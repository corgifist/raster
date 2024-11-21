#include "decode_audio_asset.h"
#include "common/asset_id.h"

#include "common/audio_info.h"

namespace Raster {

    AudioDecoderContext::AudioDecoderContext() {
        this->lastAudioPassID = -1;
        this->assetID = 0;
        this->needsSeeking = true;
        this->wasOpened = false;
        this->health = MAX_DECODER_LIFESPAN;
        this->cacheValid = false;
        this->cachedSamples = AudioInfo::MakeRawAudioSamples();
        this->resamplerSamplesCount = 0;
    }

    DecodeAudioAsset::DecodeAudioAsset() {
        NodeBase::Initialize();

        SetupAttribute("Asset", AssetID());
        SetupAttribute("Volume", 1.0f);

        this->m_forceSeek = false;

        AddOutputPin("Samples");

        MakePinPersistent("Samples");
    }

    DecodeAudioAsset::~DecodeAudioAsset() {
        DestroyPersistentPins();
    }

    AbstractPinMap DecodeAudioAsset::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};
        SharedLockGuard guard(m_decodingMutex);
        auto assetIDCandidate = GetAttribute<int>("Asset", t_contextData);
        auto volumeCandidate = GetAttribute<float>("Volume", t_contextData);

        auto& project = Workspace::GetProject();
        if (t_contextData.find("AUDIO_PASS") == t_contextData.end() || (t_contextData.find("AUDIO_PASS") != t_contextData.end() && !RASTER_GET_CONTEXT_VALUE(t_contextData, "ALLOW_MEDIA_DECODING", bool))) {
            auto decoderContext = GetDecoderContext();
            decoderContext->cache.Lock();
            auto cacheCandidate = decoderContext->cache.GetReference().GetCachedSamples();
            if (cacheCandidate.has_value()) {
                TryAppendAbstractPinMap(result, "Samples", cacheCandidate.value());
            }
            decoderContext->cache.Unlock();
            return result;
        }
        auto audioPassID = std::any_cast<int>(t_contextData["AUDIO_PASS_ID"]);

        auto decoderContext = GetDecoderContext();
        if (decoderContext->lastAudioPassID + 1 != audioPassID) {
            decoderContext->needsSeeking = true;
            decoderContext->cacheValid = false;
        }
        if (decoderContext->lastAudioPassID != audioPassID) decoderContext->cacheValid = false;
        if (decoderContext->cacheValid) {
            AudioSamples samples;
            samples.sampleRate = AudioInfo::s_sampleRate;
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
                    decoderContext->audioResampler.init(av::ChannelLayout(AudioInfo::s_channels).layout(), AudioInfo::s_sampleRate, AV_SAMPLE_FMT_FLT,
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
            decoderContext->cache.Lock();
            decoderContext->cache.GetReference().SetCachedSamples(samples);
            decoderContext->cache.Unlock();

            decoderContext->cacheValid = true;
            decoderContext->lastAudioPassID = audioPassID;

            TryAppendAbstractPinMap(result, "Samples", samples);
        }


        return result;
    }

    SharedDecoderContext DecodeAudioAsset::GetDecoderContext() {
        std::vector<float> deadDecoders;
        m_decoderContexts.Lock();
        for (auto& context : m_decoderContexts.GetReference()) {
            context.second->health--;
            if (context.second->health < 0) {
                deadDecoders.push_back(context.first);
            }
        }

        for (auto& deadDecoder : deadDecoders) {
            if (m_decoderContexts.GetReference().find(deadDecoder) != m_decoderContexts.GetReference().end())
                m_decoderContexts.GetReference().erase(deadDecoder);
        }

        auto& project = Workspace::GetProject();
        auto currentOffset = project.GetTimeTravelOffset();
        if (m_decoderContexts.GetReference().find(currentOffset) != m_decoderContexts.GetReference().end()) {
            m_decoderContexts.GetReference()[currentOffset]->health = MAX_DECODER_LIFESPAN;
            auto result = m_decoderContexts.GetReference()[currentOffset];
            m_decoderContexts.Unlock();
            return result;
        }

        m_decoderContexts.GetReference()[currentOffset] = std::make_shared<AudioDecoderContext>();
        auto result = m_decoderContexts.GetReference()[currentOffset];
        m_decoderContexts.Unlock();
        return result;
    }

    av::AudioSamples DecodeAudioAsset::PopResampler(SharedDecoderContext t_context) {
        if (t_context->resamplerSamplesCount < AudioInfo::s_periodSize) {
            PushMoreSamples(t_context);
            return PopResampler(t_context);
        }
        av::AudioSamples result = t_context->audioResampler.pop(AudioInfo::s_periodSize);
        t_context->resamplerSamplesCount -= AudioInfo::s_periodSize;
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
        SharedLockGuard guard(m_decodingMutex);
        auto& project = Workspace::GetProject();
        m_decoderContexts.Lock();
        for (auto& decodingPair : m_decoderContexts.GetReference()) {
            decodingPair.second->needsSeeking = true;
            decodingPair.second->cacheValid = false;
        }
        m_decoderContexts.Unlock();
    }

    std::optional<float> DecodeAudioAsset::AbstractGetContentDuration() {
        m_decoderContexts.Lock();
        if (m_decoderContexts.GetReference().find(0) != m_decoderContexts.GetReference().end()) {
            auto& decoderContext = m_decoderContexts.GetReference()[0];
            auto& project = Workspace::GetProject();
            if (decoderContext->formatCtx.isOpened()) {
                m_decoderContexts.Unlock();
                return decoderContext->formatCtx.duration().seconds() * project.framerate;
            }
        }
        m_decoderContexts.Unlock();
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
        return "Decode Audio";
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