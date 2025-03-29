#include "common/generic_audio_decoder.h"

#include "audio_decoder.h"
#include "common/audio_info.h"
#include "common/audio_samples.h"
#include "audio_decoders.h"
#include "common/thread_unique_value.h"
#include "common/typedefs.h"
#include "raster.h"
#include <rubberband/RubberBandStretcher.h>

#define FAST_ENGINE_FLAGS RubberBand::RubberBandStretcher::OptionEngineFaster  \
                    | RubberBand::RubberBandStretcher::OptionProcessRealTime  \
                    | RubberBand::RubberBandStretcher::OptionPitchHighSpeed \
                    | RubberBand::RubberBandStretcher::OptionThreadingAuto \
                    | RubberBand::RubberBandStretcher::OptionWindowShort

#define QUALITY_ENGINE_FLAGS RubberBand::RubberBandStretcher::OptionProcessRealTime  \
                        | RubberBand::RubberBandStretcher::OptionPitchHighConsistency \
                        | RubberBand::RubberBandStretcher::OptionChannelsTogether \
                        | RubberBand::RubberBandStretcher::OptionFormantPreserved \
                        | RubberBand::RubberBandStretcher::OptionWindowLong \
                        | RubberBand::RubberBandStretcher::OptionThreadingAuto 

namespace Raster {
    GenericAudioDecoder::GenericAudioDecoder() {
        this->assetID = 0;
        this->decoderContexts = std::make_shared<std::unordered_map<float, int>>();
        this->waveformDecoderContexts = std::make_shared<std::unordered_map<float, int>>();
        this->seekTarget = std::make_shared<float>();
        *this->seekTarget = 0;
        this->speed = std::make_shared<float>();
        *this->speed = 1.0f;
        this->pitch = std::make_shared<float>();
        *this->pitch = 1.0f;
    }

    static SharedAudioDecoder AllocateDecoderContext(int t_decoderID) {
        if (AudioDecoders::s_decoders.find(t_decoderID) == AudioDecoders::s_decoders.end()) {
            auto result = std::make_shared<AudioDecoder>();
            AudioDecoders::s_decoders[t_decoderID] = result;
            return result;
        }
        return AudioDecoders::GetDecoder(t_decoderID);
    }

    static std::shared_ptr<std::unordered_map<float, int>> GetSuitableDecoderContexts(GenericAudioDecoder* t_decoder, ContextData t_contextData) {
        return RASTER_GET_CONTEXT_VALUE(t_contextData, "WAVEFORM_PASS", bool) ? t_decoder->waveformDecoderContexts : t_decoder->decoderContexts;
    }

    static SharedAudioDecoder GetDecoderContext(GenericAudioDecoder* t_decoder, ContextData t_contextData) {
        std::vector<float> deadDecoders;
        for (auto& context : *GetSuitableDecoderContexts(t_decoder, t_contextData)) {
            auto decoder = AllocateDecoderContext(context.second);
            decoder->health--;
            if (decoder->health < 0) {
                deadDecoders.push_back(context.first);
            }
        }

        for (auto& deadDecoder : deadDecoders) {
            auto correspondingDecoderID = (*GetSuitableDecoderContexts(t_decoder, t_contextData))[deadDecoder];
            auto decoderIterator = AudioDecoders::s_decoders.find(correspondingDecoderID);
            if (decoderIterator != AudioDecoders::s_decoders.end()) {
                AudioDecoders::s_decoders.erase(correspondingDecoderID);
                GetSuitableDecoderContexts(t_decoder, t_contextData)->erase(deadDecoder);
            }
        }

        auto& project = Workspace::GetProject();
        auto currentOffset = project.GetTimeTravelOffset();
        if (GetSuitableDecoderContexts(t_decoder, t_contextData)->find(currentOffset) != GetSuitableDecoderContexts(t_decoder, t_contextData)->end()) {
            auto allocatedDecoder = AllocateDecoderContext((*GetSuitableDecoderContexts(t_decoder, t_contextData))[currentOffset]);
            allocatedDecoder->health = MAX_GENERIC_AUDIO_DECODER_LIFESPAN;
            allocatedDecoder->timeOffset = currentOffset;
            allocatedDecoder->id = (*GetSuitableDecoderContexts(t_decoder, t_contextData))[currentOffset];
            return allocatedDecoder;
        }
        
        int newDecoderID = Randomizer::GetRandomInteger();
        auto newDecoder = AllocateDecoderContext(newDecoderID);
        (*GetSuitableDecoderContexts(t_decoder, t_contextData))[currentOffset] = newDecoderID;
        newDecoder->health = MAX_GENERIC_AUDIO_DECODER_LIFESPAN;
        newDecoder->timeOffset = currentOffset;
        newDecoder->id = newDecoderID;
        return newDecoder;
    }


    std::optional<AudioSamples> GenericAudioDecoder::GetCachedSamples() {
        SharedLockGuard guard(m_decodingMutex);
        auto& project = Workspace::GetProject();

        auto decoder = GetDecoderContext(this, {});
        if (decoder->cacheValid) return decoder->cache.Get().GetCachedSamples();
        return std::nullopt;
    }

    std::optional<AudioSamples> GenericAudioDecoder::DecodeSamples(int audioPassID, ContextData t_contextData) {
        SharedLockGuard guard(m_decodingMutex);
        auto& project = Workspace::GetProject();

        auto decoder = GetDecoderContext(this, t_contextData);

        if (decoder->lastAudioPassID + 1 != audioPassID) {
            decoder->needsSeeking = true;
            decoder->cacheValid = false;
        }
        if (decoder->lastAudioPassID != audioPassID) decoder->cacheValid = false;
        if (decoder->cacheValid && !RASTER_GET_CONTEXT_VALUE(t_contextData, "WAVEFORM_PASS", bool)) {
            decoder->cache.Lock();
            auto cachedSamples = decoder->cache.GetReference().GetCachedSamples();
            decoder->cache.Unlock();
            decoder->lastAudioPassID = audioPassID;
            if (cachedSamples) {
                return *cachedSamples;
            } 
        }

        if (decoder->needsSeeking && decoder->formatCtx.isOpened() && decoder->audioDecoderCtx.isOpened() && !RASTER_GET_CONTEXT_VALUE(t_contextData, "WAVEFORM_PASS", bool)) {
            decoder->SeekDecoder(*seekTarget);
            if (decoder->stretcher) decoder->stretcher->reset();
        }

        if (RASTER_GET_CONTEXT_VALUE(t_contextData, "WAVEFORM_FIRST_PASS", bool)) {
            decoder->SeekDecoder(0.0);
            if (decoder->stretcher) decoder->stretcher->reset();
        }

        auto assetCandidate = Workspace::GetAssetByAssetID(assetID);
        std::optional<std::string> assetPathCandidate;
        std::optional<Texture> attachedPicCandidate;
        if (assetCandidate.has_value()) {
            assetPathCandidate = assetCandidate.value()->Serialize()["Data"]["RelativePath"];
            attachedPicCandidate = assetCandidate.value()->GetPreviewTexture();
        }

        if (assetPathCandidate.has_value() && assetPathCandidate.value() != decoder->assetPath) {
            auto& assetPath = assetPathCandidate.value();
            // print("opening format context");    

            decoder->formatCtx.close();
            decoder->formatCtx.openInput(FormatString("%s/%s", project.path.c_str(), assetPath.c_str()));
            decoder->formatCtx.findStreamInfo();

            bool streamWasFound = false;

            for (int i = 0; i < decoder->formatCtx.streamsCount(); i++) {
                auto stream = decoder->formatCtx.stream(i);
                if (stream.isAudio()) {
                    decoder->targetAudioStream = stream;
                    streamWasFound = true;
                    break;
                }
            }

            if (streamWasFound) {
                if (decoder->audioDecoderCtx.isOpened()) {
                    decoder->audioDecoderCtx.close();
                }
                decoder->audioDecoderCtx = av::AudioDecoderContext(decoder->targetAudioStream);
                decoder->audioDecoderCtx.open(av::Codec());
                decoder->audioDecoderCtx.setRefCountedFrames(true);
                decoder->audioResampler.init(av::ChannelLayout(AudioInfo::s_channels).layout(), AudioInfo::s_sampleRate, AV_SAMPLE_FMT_FLT,
                                                        decoder->audioDecoderCtx.channelLayout(), decoder->audioDecoderCtx.sampleRate(), decoder->audioDecoderCtx.sampleFormat());
                decoder->needsSeeking = true;
            }

            decoder->wasOpened = true;
            decoder->assetPath = assetPath;
        }

        if (decoder->formatCtx.isOpened() && decoder->audioDecoderCtx.isOpened()) {
            if (decoder->audioResampler.dstChannelLayout() != av::ChannelLayout(AudioInfo::s_channels).layout() || 
                    decoder->audioResampler.dstSampleRate() != AudioInfo::s_sampleRate) {
                decoder->audioResampler.init(av::ChannelLayout(AudioInfo::s_channels).layout(), AudioInfo::s_sampleRate, AV_SAMPLE_FMT_FLT,
                                                        decoder->audioDecoderCtx.channelLayout(), decoder->audioDecoderCtx.sampleRate(), decoder->audioDecoderCtx.sampleFormat());
            }
            decoder->lastAudioPassID = audioPassID;
            bool mustRebuildStretcher = false;
            if (!decoder->stretcher) mustRebuildStretcher = true;
            if (decoder->stretcher && (decoder->stretcher->getChannelCount() != AudioInfo::s_channels || decoder->stretcherSampleRate != AudioInfo::s_sampleRate)) {
                mustRebuildStretcher = true;
            }
            if (mustRebuildStretcher) {
                decoder->stretcher = std::make_shared<RubberBand::RubberBandStretcher>(AudioInfo::s_sampleRate, AudioInfo::s_channels, FAST_ENGINE_FLAGS);
                decoder->stretcherSampleRate = AudioInfo::s_sampleRate;
            }
            decoder->stretcher->setPitchScale(*pitch);
            decoder->stretcher->setTimeRatio(1.0f / *speed);
            while (decoder->stretcher->available() < AudioInfo::s_periodSize) {
                auto resampledSamples = decoder->PopResampler();
                if (!resampledSamples) return std::nullopt;
                static ThreadUniqueValue<SharedRawDeinterleavedAudioSamples> s_deinterleavedSamples;
                auto& m_deinterleavedSamples = s_deinterleavedSamples.Get();
                if (!m_deinterleavedSamples) m_deinterleavedSamples = MakeDeinterleavedAudioSamples(AudioInfo::s_periodSize, AudioInfo::s_channels);
                ValidateDeinterleavedAudioSamples(m_deinterleavedSamples, AudioInfo::s_periodSize, AudioInfo::s_channels);
                DeinterleaveAudioSamples(std::make_shared<std::vector<float>>((float*) resampledSamples.data(), (float*) (resampledSamples.data()) + (AudioInfo::s_channels * AudioInfo::s_periodSize)), m_deinterleavedSamples, AudioInfo::s_periodSize, AudioInfo::s_channels);

                std::vector<float*> planarBuffersData;
                for (int i = 0; i < m_deinterleavedSamples->size(); i++) {
                    planarBuffersData.push_back(m_deinterleavedSamples->at(i).data());
                }

                decoder->stretcher->process(planarBuffersData.data(), AudioInfo::s_periodSize, false);
            }
            static ThreadUniqueValue<SharedRawDeinterleavedAudioSamples> s_deinterleavedSamples;
            auto& m_deinterleavedSamples = s_deinterleavedSamples.Get();
            if (!m_deinterleavedSamples) m_deinterleavedSamples = MakeDeinterleavedAudioSamples(AudioInfo::s_periodSize, AudioInfo::s_channels);
            ValidateDeinterleavedAudioSamples(m_deinterleavedSamples, AudioInfo::s_periodSize, AudioInfo::s_channels);

            std::vector<float*> planarBuffersData;
            for (int i = 0; i < m_deinterleavedSamples->size(); i++) {
                planarBuffersData.push_back(m_deinterleavedSamples->at(i).data());
            }
            if (decoder->stretcher->available() >= AudioInfo::s_periodSize) {
                decoder->stretcher->retrieve(planarBuffersData.data(), AudioInfo::s_periodSize);

                SharedRawInterleavedAudioSamples resultInterleavedBuffer = MakeInterleavedAudioSamples(AudioInfo::s_periodSize, AudioInfo::s_channels);
                InterleaveAudioSamples(m_deinterleavedSamples, resultInterleavedBuffer, AudioInfo::s_periodSize, AudioInfo::s_channels);
                auto samplesPtr = resultInterleavedBuffer->data();
                SharedRawAudioSamples allocatedSamples = AudioInfo::MakeRawAudioSamples();
                memcpy(allocatedSamples, samplesPtr, sizeof(float) * AudioInfo::s_channels * AudioInfo::s_periodSize);

                AudioSamples samples;
                samples.sampleRate = AudioInfo::s_channels;
                samples.samples = allocatedSamples;
                if (attachedPicCandidate.has_value()) {
                    samples.attachedPictures.push_back(attachedPicCandidate.value());
                }
                decoder->cache.Lock();
                decoder->cache.GetReference().SetCachedSamples(samples);
                decoder->cache.Unlock();

                decoder->cacheValid = true;
                return samples;
            }
        }

        return std::nullopt;
    }

    void GenericAudioDecoder::Seek(float t_second) {
        *seekTarget = t_second;
        for (auto& decodingPair : *decoderContexts) {
            auto decoder = AllocateDecoderContext(decodingPair.second);
            decoder->needsSeeking = true;
            decoder->cacheValid = false;
        }
    }

    void GenericAudioDecoder::Destroy() {
        for (auto& context : *decoderContexts) {
            if (AudioDecoders::DecoderExists(context.second)) {
                AudioDecoders::DestroyDecoder(context.second);
            }
        }
        for (auto& context : *waveformDecoderContexts) {
            if (AudioDecoders::DecoderExists(context.second)) {
                AudioDecoders::DestroyDecoder(context.second);
            }
        }
    }

    std::optional<float> GenericAudioDecoder::GetContentDuration() {
        if (decoderContexts->find(0) != decoderContexts->end()) {
            auto decoder = AllocateDecoderContext(decoderContexts->at(0));
            if (decoder->formatCtx.isOpened()) {
                return decoder->formatCtx.duration().seconds() * Workspace::GetProject().framerate;
            }
        }
        return std::nullopt;
    }

};