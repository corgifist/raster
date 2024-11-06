#include "common/generic_audio_decoder.h"

#include "../avcpp/av.h"
#include "../avcpp/ffmpeg.h"
#include "../avcpp/codec.h"
#include "../avcpp/packet.h"
#include "../avcpp/videorescaler.h"
#include "../avcpp/audioresampler.h"
#include "../avcpp/avutils.h"

// API2
#include "../avcpp/format.h"
#include "../avcpp/formatcontext.h"
#include "../avcpp/codec.h"
#include "../avcpp/codeccontext.h"

#include "common/audio_info.h"
#include "common/threads.h"

namespace Raster {
    
    struct GenericAudioDecoderContext {
        av::FormatContext formatCtx;
        av::Stream targetAudioStream;
        av::AudioDecoderContext audioDecoderCtx;
        av::AudioResampler audioResampler;
        int resamplerSamplesCount;
        SharedRawAudioSamples cachedSamples;
        bool cacheValid;
        AudioCache cache;

        bool wasOpened;
        int lastAudioPassID;
        bool needsSeeking;
        int health;
        int assetID;
        std::string assetPath;

        float timeOffset;
        int id;

        GenericAudioDecoderContext() {
            this->lastAudioPassID = INT_MIN;
            this->assetID = 0;
            this->needsSeeking = true;
            this->wasOpened = false;
            this->health = MAX_GENERIC_AUDIO_DECODER_LIFESPAN;
            this->cacheValid = false;
            this->cachedSamples = AudioInfo::MakeRawAudioSamples();
            this->resamplerSamplesCount = 0;
            this->timeOffset = 0;
        }
    };

    using SharedAudioDecoderContext = std::shared_ptr<GenericAudioDecoderContext>;

    static std::unordered_map<int, SharedAudioDecoderContext> s_decoders;

    GenericAudioDecoder::GenericAudioDecoder() {
        this->assetID = 0;
        this->decoderContexts = std::make_shared<std::unordered_map<float, int>>();
        this->seekTarget = std::make_shared<float>();
    }

    static SharedAudioDecoderContext AllocateDecoderContext(int t_decoderID) {
        if (s_decoders.find(t_decoderID) != s_decoders.end()) {
            return s_decoders[t_decoderID];
        }
        s_decoders[t_decoderID] = std::make_shared<GenericAudioDecoderContext>();
        return s_decoders[t_decoderID];
    }

    static SharedAudioDecoderContext GetDecoderContext(GenericAudioDecoder* t_decoder) {
        std::vector<float> deadDecoders;
        for (auto& context : *t_decoder->decoderContexts) {
            auto decoder = AllocateDecoderContext(context.second);
            decoder->health--;
            if (decoder->health < 0) {
                deadDecoders.push_back(context.first);
            }
        }

        for (auto& deadDecoder : deadDecoders) {
            auto correspondingDecoderID = (*t_decoder->decoderContexts)[deadDecoder];
            auto decoderIterator = s_decoders.find(correspondingDecoderID);
            if (decoderIterator != s_decoders.end()) {
                s_decoders.erase(correspondingDecoderID);
                t_decoder->decoderContexts->erase(deadDecoder);
            }
        }

        auto& project = Workspace::GetProject();
        auto currentOffset = project.GetTimeTravelOffset();
        if (t_decoder->decoderContexts->find(currentOffset) != t_decoder->decoderContexts->end()) {
            auto allocatedDecoder = AllocateDecoderContext((*t_decoder->decoderContexts)[currentOffset]);
            allocatedDecoder->health = MAX_GENERIC_AUDIO_DECODER_LIFESPAN;
            allocatedDecoder->timeOffset = currentOffset;
            allocatedDecoder->id = (*t_decoder->decoderContexts)[currentOffset];
            return allocatedDecoder;
        }
        
        int newDecoderID = Randomizer::GetRandomInteger();
        auto newDecoder = AllocateDecoderContext(newDecoderID);
        (*t_decoder->decoderContexts)[currentOffset] = newDecoderID;
        newDecoder->health = MAX_GENERIC_AUDIO_DECODER_LIFESPAN;
        newDecoder->timeOffset = currentOffset;
        newDecoder->id = newDecoderID;
        return newDecoder;
    }

    static void FlushResampler(GenericAudioDecoder* t_ga, int t_decoderID) {
        auto decoder = AllocateDecoderContext(t_decoderID);
        if (decoder->resamplerSamplesCount > 0) decoder->audioResampler.pop(decoder->resamplerSamplesCount);
        decoder->audioResampler.pop(0);
        decoder->resamplerSamplesCount = 0;
    }

    static av::AudioSamples DecodeOneFrame(GenericAudioDecoder* t_ga, int t_decoderID) {
        auto decoder = AllocateDecoderContext(t_decoderID);
        while (true) {
            av::Packet pkt = decoder->formatCtx.readPacket();
            if (!pkt) break;
            if (pkt.streamIndex() != decoder->targetAudioStream.index()) continue;

            av::AudioSamples decodedSamples = decoder->audioDecoderCtx.decode(pkt);
            return decodedSamples;
        }
        return av::AudioSamples();
    }

    static void PushMoreSamples(GenericAudioDecoder* t_ga, int t_decoderID) {
        auto decoder = AllocateDecoderContext(t_decoderID);
        auto decodedSamples = DecodeOneFrame(t_ga, t_decoderID);
        if (decodedSamples) {
            decoder->audioResampler.push(decodedSamples);
            decoder->resamplerSamplesCount += decodedSamples.samplesCount();
        }
    }

    static av::AudioSamples PopResampler(GenericAudioDecoder* t_ga, int t_decoderID) {
        auto decoder = AllocateDecoderContext(t_decoderID);
        if (decoder->resamplerSamplesCount < AudioInfo::s_periodSize) {
            PushMoreSamples(t_ga, t_decoderID);
            return PopResampler(t_ga, t_decoderID);
        }

        auto result = decoder->audioResampler.pop(AudioInfo::s_periodSize);
        decoder->resamplerSamplesCount -= AudioInfo::s_periodSize;
        return result;
    }

    static void SeekDecoder(GenericAudioDecoder* t_ga, int t_decoderID, float t_second) {
        auto decoder = AllocateDecoderContext(t_decoderID);
        if (!decoder->audioDecoderCtx.isOpened()) return;
        auto& project = Workspace::GetProject();
        float currentTime = t_second + decoder->timeOffset / project.framerate;
        auto rational = decoder->audioDecoderCtx.timeBase().getValue();
        avcodec_flush_buffers(decoder->audioDecoderCtx.raw());
        decoder->formatCtx.seek((int64_t) (currentTime) * (int64_t)rational.den / (int64_t) rational.num, decoder->targetAudioStream.index(), AVSEEK_FLAG_ANY);
        decoder->cacheValid = false;
        decoder->needsSeeking = false;
        FlushResampler(t_ga, t_decoderID);
        DecodeOneFrame(t_ga, t_decoderID);
    }

    std::optional<AudioSamples> GenericAudioDecoder::GetCachedSamples() {
        SharedLockGuard guard(m_decodingMutex);
        auto& project = Workspace::GetProject();

        auto decoder = GetDecoderContext(this);
        if (decoder->cacheValid) return decoder->cache.GetCachedSamples();
        return std::nullopt;
    }

    std::optional<AudioSamples> GenericAudioDecoder::DecodeSamples() {
        SharedLockGuard guard(m_decodingMutex);
        auto& project = Workspace::GetProject();

        auto decoder = GetDecoderContext(this);
        auto audioPassID = AudioInfo::s_audioPassID;

        if (decoder->lastAudioPassID + 1 != audioPassID) {
            decoder->needsSeeking = true;
            decoder->cacheValid = false;
        }
        if (decoder->lastAudioPassID != audioPassID) decoder->cacheValid = false;
        if (decoder->cacheValid) {
            AudioSamples samples;
            samples.sampleRate = AudioInfo::s_sampleRate;
            samples.samples = decoder->cachedSamples;
            decoder->lastAudioPassID = audioPassID;
            return samples;
        }

        if (decoder->needsSeeking && decoder->formatCtx.isOpened()) {
            SeekDecoder(this, decoder->id, *seekTarget);
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
            print("opening format context");    

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
                decoder->audioResampler.init(av::ChannelLayout(AudioInfo::s_channels).layout(), AudioInfo::s_sampleRate, AV_SAMPLE_FMT_FLT,
                                                        decoder->audioDecoderCtx.channelLayout(), decoder->audioDecoderCtx.sampleRate(), decoder->audioDecoderCtx.sampleFormat());
                decoder->needsSeeking = true;
            }

            decoder->wasOpened = true;
            decoder->assetPath = assetPath;
        }

        if (decoder->formatCtx.isOpened() && decoder->audioDecoderCtx.isOpened()) {
            auto resampledSamples = PopResampler(this, decoder->id);
            auto samplesPtr = resampledSamples.data();
            memcpy(decoder->cachedSamples->data(), samplesPtr, resampledSamples.sampleFormat().bytesPerSample() * resampledSamples.channelsCount() * resampledSamples.samplesCount());

            AudioSamples samples;
            samples.sampleRate = resampledSamples.sampleRate();
            samples.samples = decoder->cachedSamples;
            if (attachedPicCandidate.has_value()) {
                samples.attachedPictures.push_back(attachedPicCandidate.value());
            }
            decoder->cache.SetCachedSamples(samples);

            decoder->cacheValid = true;
            decoder->lastAudioPassID = audioPassID;
            return samples;
        }

        return std::nullopt;
    }

    void GenericAudioDecoder::Seek(float t_second) {
        SharedLockGuard decodingGuard(m_decodingMutex);
        *seekTarget = t_second;
        for (auto& decodingPair : *decoderContexts) {
            auto decoder = AllocateDecoderContext(decodingPair.second);
            decoder->needsSeeking = true;
            decoder->cacheValid = false;
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