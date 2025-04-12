#include "audio_decoder.h"
#include "common/audio_info.h"
#include "common/workspace.h"

namespace Raster {
    void AudioDecoder::FlushResampler() {
        std::error_code ec;
        audioResampler.pop(audioResampler.delay(), ec);
        audioResampler.pop(0, ec);
    }

    av::AudioSamples AudioDecoder::DecodeOneFrame() {
        while (true) {
            av::Packet pkt = formatCtx.readPacket();
            if (!pkt) break;
            if (pkt.streamIndex() != targetAudioStream.index()) continue;

            av::AudioSamples decodedSamples = audioDecoderCtx.decode(pkt);
            return decodedSamples;
        }
        return av::AudioSamples();
    }

    bool AudioDecoder::PushMoreSamples() {
        auto decodedSamples = DecodeOneFrame();
        if (decodedSamples) {
            audioResampler.push(decodedSamples);
            return true;
        }
        return false;
    }

    av::AudioSamples AudioDecoder::PopResampler() {
        auto result = av::AudioSamples();
        while (!(result = audioResampler.pop(AudioInfo::s_periodSize))) {
            if (!PushMoreSamples()) return av::AudioSamples();
        }

        return result;
    }

    void AudioDecoder::SeekDecoder(float t_second) {
        if (!audioDecoderCtx.isOpened()) return;
        float currentTime = t_second + timeOffset / Workspace::GetProject().framerate;
        auto rational = targetAudioStream.timeBase().getValue();
        formatCtx.seek((int64_t) (currentTime) * (int64_t)rational.den / (int64_t) rational.num, targetAudioStream.index(), AVSEEK_FLAG_ANY);
        avcodec_flush_buffers(audioDecoderCtx.raw());
        if (stretcher) stretcher->reset();
        cacheValid = false;
        needsSeeking = false;
        FlushResampler();
        av::AudioSamples seekSamples;
        while ((seekSamples = DecodeOneFrame())) {
            if (seekSamples.pts().seconds() >= currentTime) break;
        }  
    }
};