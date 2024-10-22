#include "decode_audio_asset.h"

namespace Raster {

    AudioDecoderContext::AudioDecoderContext() {
        this->lastAudioPassID = -1;
        this->assetID = 0;
        this->needsSeeking = true;
        this->wasOpened = false;
        this->health = MAX_DECODER_LIFESPAN;
        this->cacheValid = false;
        this->cachedSamples = std::make_shared<std::vector<float>>(4096 * Audio::GetChannelCount());
        this->resamplerSamplesCount = 0;
    }

    DecodeAudioAsset::DecodeAudioAsset() {
        NodeBase::Initialize();

        SetupAttribute("Asset", 0);
        SetupAttribute("Volume", 1.0f);

        this->m_forceSeek = false;
        this->m_decodingMutex = std::make_shared<std::mutex>();

        AddOutputPin("Samples");

        MakePinPersistent("Samples");
    }

    DecodeAudioAsset::~DecodeAudioAsset() {
        DestroyPersistentPins();
    }

    AbstractPinMap DecodeAudioAsset::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};
        std::lock_guard<std::mutex> decodingGuard(*m_decodingMutex);
        auto contextData = GetContextData();
        auto assetIDCandidate = GetAttribute<int>("Asset");
        auto volumeCandidate = GetAttribute<float>("Volume");

        auto& project = Workspace::GetProject();
        if (contextData.find("AUDIO_PASS") == contextData.end()) {
            auto decoderContext = GetDecoderContext();
            auto cacheCandidate = decoderContext->cache.GetCachedSamples();
            if (cacheCandidate.has_value()) {
                TryAppendAbstractPinMap(result, "Samples", cacheCandidate.value());
            }
            return result;
        }
        if (contextData.find("AUDIO_PASS") == contextData.end() || !project.playing) return {};
        auto audioPassID = std::any_cast<int>(contextData["AUDIO_PASS_ID"]);

        auto decoderContext = GetDecoderContext();
        if (decoderContext->lastAudioPassID + 1 != audioPassID) {
            decoderContext->needsSeeking = true;
            decoderContext->cacheValid = false;
        }
        if (decoderContext->lastAudioPassID != audioPassID) decoderContext->cacheValid = false;
        if (decoderContext->cacheValid) {
            AudioSamples samples;
            samples.sampleRate = Audio::GetSampleRate();
            samples.samples = decoderContext->cachedSamples;
            decoderContext->lastAudioPassID = audioPassID;
            TryAppendAbstractPinMap(result, "Samples", samples);
            return result;
        }

        if (decoderContext->needsSeeking && decoderContext->formatCtx.isOpened()) {
            SeekDecoder(decoderContext);
        }

        std::optional<std::string> assetPathCandidate;
        std::optional<Texture> attachedPicCandidate;
        if (assetIDCandidate.has_value()) {
            auto& assetID = assetIDCandidate.value();
            auto assetCandidate = Workspace::GetAssetByAssetID(assetID);
            if (assetCandidate.has_value()) {
                assetPathCandidate = assetCandidate.value()->Serialize()["Data"]["RelativePath"];
                attachedPicCandidate = assetCandidate.value()->GetPreviewTexture();
            }

            if (assetPathCandidate.has_value() && assetPathCandidate.value() != decoderContext->assetPath /* && !std::filesystem::is_directory(assetPathCandidate.value()) && std::filesystem::exists(assetPathCandidate.value()) */) {
                auto& assetPath = assetPathCandidate.value();

                decoderContext->formatCtx.close();
                decoderContext->formatCtx.openInput(FormatString("%s/%s", project.path.c_str(), assetPath.c_str()));
                decoderContext->formatCtx.findStreamInfo();

                bool streamWasFound = false;
                for (int i = 0; i < decoderContext->formatCtx.streamsCount(); i++) {
                    auto stream = decoderContext->formatCtx.stream(i);
                    if (stream.isAudio()) {
                        decoderContext->targetAudioStream = stream;
                        streamWasFound = true;
                        break;
                    }
                }

                if (streamWasFound) {
                    if (decoderContext->audioDecoderCtx.isOpened()) {
                        decoderContext->audioDecoderCtx.close();
                    }
                    decoderContext->audioDecoderCtx = av::AudioDecoderContext(decoderContext->targetAudioStream);
                    decoderContext->audioDecoderCtx.open(Codec());
                    decoderContext->audioResampler.init(av::ChannelLayout(Audio::GetChannelCount()).layout(), Audio::GetSampleRate(), AV_SAMPLE_FMT_FLT,
                                                            decoderContext->audioDecoderCtx.channelLayout(), decoderContext->audioDecoderCtx.sampleRate(), decoderContext->audioDecoderCtx.sampleFormat());
                    decoderContext->needsSeeking = true;
                }

                decoderContext->wasOpened = true;
                decoderContext->assetPath = assetPath;
            }
        }



        if (decoderContext->formatCtx.isOpened() && decoderContext->audioDecoderCtx.isOpened()) {
            auto resampledSamples = PopResampler(decoderContext);
            auto samplesPtr = resampledSamples.data();
            memcpy(decoderContext->cachedSamples->data(), samplesPtr, resampledSamples.sampleFormat().bytesPerSample() * resampledSamples.channelsCount() * resampledSamples.samplesCount());
            if (volumeCandidate.has_value()) {
                auto& volume = volumeCandidate.value();
                auto rawSamplesPtr = decoderContext->cachedSamples->data();
                for (int i = 0; i < decoderContext->cachedSamples->size(); i++) {
                    rawSamplesPtr[i] *= volume; 
                }
            } 

            AudioSamples samples;
            samples.sampleRate = resampledSamples.sampleRate();
            samples.samples = decoderContext->cachedSamples;
            if (attachedPicCandidate.has_value()) {
                samples.attachedPictures.push_back(attachedPicCandidate.value());
            }
            decoderContext->cache.SetCachedSamples(samples);

            decoderContext->cacheValid = true;
            decoderContext->lastAudioPassID = audioPassID;

            TryAppendAbstractPinMap(result, "Samples", samples);
        }


        return result;
    }

    SharedDecoderContext DecodeAudioAsset::GetDecoderContext() {
        std::vector<float> deadDecoders;
        for (auto& context : m_decoderContexts) {
            context.second->health--;
            if (context.second->health < 0) {
                deadDecoders.push_back(context.first);
            }
        }

        for (auto& deadDecoder : deadDecoders) {
            m_decoderContexts.erase(deadDecoder);
        }

        auto& project = Workspace::GetProject();
        auto currentOffset = project.GetTimeTravelOffset();
        if (m_decoderContexts.find(currentOffset) != m_decoderContexts.end()) {
            m_decoderContexts[currentOffset]->health = MAX_DECODER_LIFESPAN;
            return m_decoderContexts[currentOffset];
        }

        m_decoderContexts[currentOffset] = std::make_shared<AudioDecoderContext>();
        return m_decoderContexts[currentOffset];
    }

    av::AudioSamples DecodeAudioAsset::PopResampler(SharedDecoderContext t_context) {
        if (t_context->resamplerSamplesCount < Audio::s_samplesCount) {
            PushMoreSamples(t_context);
            return PopResampler(t_context);
        }
        av::AudioSamples result = t_context->audioResampler.pop(Audio::s_samplesCount);
        t_context->resamplerSamplesCount -= Audio::s_samplesCount;
        return result;
    }

    void DecodeAudioAsset::PushMoreSamples(SharedDecoderContext t_context) {
        av::AudioSamples decodedSamples = DecodeOneFrame(t_context);
        if (decodedSamples) {
            t_context->audioResampler.push(decodedSamples);
        t_context->resamplerSamplesCount += decodedSamples.samplesCount();  
        }
    }

    av::AudioSamples DecodeAudioAsset::DecodeOneFrame(SharedDecoderContext t_context) {
        while (true) {
            av::Packet pkt = t_context->formatCtx.readPacket();
            if (!pkt) break;
            if (pkt.streamIndex() != t_context->targetAudioStream.index()) continue;

            av::AudioSamples decodedSamples = t_context->audioDecoderCtx.decode(pkt);
            return decodedSamples;
        }
        return av::AudioSamples();
    }

    void DecodeAudioAsset::FlushResampler(SharedDecoderContext t_context) {
        if (t_context->resamplerSamplesCount > 0) t_context->audioResampler.pop(t_context->resamplerSamplesCount);
        t_context->audioResampler.pop(0);
        t_context->resamplerSamplesCount = 0;
    }

    void DecodeAudioAsset::SeekDecoder(SharedDecoderContext t_context) {
        if (!t_context->audioDecoderCtx.isOpened()) return;
        auto& project = Workspace::GetProject();
        auto composition = Workspace::GetCompositionByNodeID(nodeID).value();
        float currentTime = (project.currentFrame - composition->beginFrame + project.GetTimeTravelOffset()) / project.framerate;
        auto rational = t_context->audioDecoderCtx.timeBase().getValue();
        avcodec_flush_buffers(t_context->audioDecoderCtx.raw());
        t_context->formatCtx.seek((int64_t) (currentTime) * (int64_t)rational.den / (int64_t) rational.num, t_context->targetAudioStream.index(), AVSEEK_FLAG_ANY);
        t_context->cacheValid = false;
        t_context->needsSeeking = false;
        FlushResampler(t_context);
        DecodeOneFrame(t_context);
    }

    void DecodeAudioAsset::AbstractOnTimelineSeek() {
        std::lock_guard<std::mutex> decodingGuard(*m_decodingMutex);
        auto& project = Workspace::GetProject();
        for (auto& decodingPair : m_decoderContexts) {
            decodingPair.second->needsSeeking = true;
            decodingPair.second->cacheValid = false;
        }
    }

    std::optional<float> DecodeAudioAsset::AbstractGetContentDuration() {
        if (m_decoderContexts.find(0) != m_decoderContexts.end()) {
            auto& decoderContext = m_decoderContexts[0];
            auto& project = Workspace::GetProject();
            if (decoderContext->formatCtx.isOpened()) {
                return decoderContext->formatCtx.duration().seconds() * project.framerate;
            }
        }
        return std::nullopt;
    }

    void DecodeAudioAsset::AbstractRenderProperties() {
        RenderAttributeProperty("Asset");
        RenderAttributeProperty("Volume", {
            FormatStringMetadata("%"),
            SliderRangeMetadata(0, 150),
            SliderBaseMetadata(100)
        });
    }

    void DecodeAudioAsset::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);   
    }

    Json DecodeAudioAsset::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool DecodeAudioAsset::AbstractDetailsAvailable() {
        return false;
    }

    std::string DecodeAudioAsset::AbstractHeader() {
        return "Decode Audio Asset";
    }

    std::string DecodeAudioAsset::Icon() {
        return ICON_FA_WAVE_SQUARE;
    }

    std::optional<std::string> DecodeAudioAsset::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::DecodeAudioAsset>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Decode Audio Asset",
            .packageName = RASTER_PACKAGED "decode_audio_asset",
            .category = Raster::DefaultNodeCategories::s_audio
        };
    }
}