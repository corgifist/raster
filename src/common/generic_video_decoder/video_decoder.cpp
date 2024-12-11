#include "video_decoder.h"
#include "common/workspace.h"
#include <cerrno>
#include <libavutil/error.h>
#include <system_error>

namespace Raster {

    bool VideoDecoder::IsUsingH264() {
        if (std::string(videoDecoderCtx.codec().longName()).find("263") != std::string::npos) return true;
        if (std::string(videoDecoderCtx.codec().longName()).find("264") != std::string::npos) return true;
        if (std::string(videoDecoderCtx.codec().longName()).find("265") != std::string::npos) return true;
        return false;
    }

    size_t VideoDecoder::GetCurrentKeyFrameForFrame(size_t frame) {
        if (frame < 0) return 0;
        for (int i = 0; i < keyframes.size(); i++) {
            int keyframe = keyframes[i];
            if (frame == keyframe) return keyframe;
            if (keyframe > frame) {
                return keyframes[glm::clamp(i - 1, 0, (int) keyframes.size() )];
            }
        }
        return 0;
    }

    av::VideoFrame VideoDecoder::DecodeOneFrame() {
        auto decodedFrame = DecodeOneFrameWithoutRescaling();
        if (!decodedFrame) return av::VideoFrame();
        auto rescaledFrame = videoRescaler.rescale(decodedFrame);
        if (!rescaledFrame) return av::VideoFrame();
        return rescaledFrame;
    }

    av::Packet VideoDecoder::ReadPacket() {
        while (true) {
            av::Packet pkt = formatCtx.readPacket();
            if (!pkt) return av::Packet();
            if (pkt.streamIndex() != targetVideoStream.index()) continue;
            return pkt;
        }
        return av::Packet();
    }

    av::VideoFrame VideoDecoder::DecodeOneFrameWithoutRescaling() {
        std::error_code ec;
        av::VideoFrame result;
        while (true) {
            av::Packet pkt = ReadPacket();
            if (!pkt) break;
            result = videoDecoderCtx.decode(pkt, ec);
            if (result && result.pts().timestamp() == av::NoPts) { 
                result.setPts(pkt.pts());
            }
            if (result && result.width() > 0 && result.height() > 0 && !result.isNull()) break;
            if (ec.value() == AVERROR(EAGAIN)) continue;
            break;
        }
        return result;
    }


    void VideoDecoder::SeekDecoder(float t_second) {
        if (!videoDecoderCtx.isOpened()) return;
        auto project = Workspace::GetProject();
        float currentTime = t_second + timeOffset / project.framerate;
        auto rational = targetVideoStream.timeBase().getValue();
        std::string codecName = videoDecoderCtx.codec().longName();
        bool needsReconstruction = IsUsingH264();
        avcodec_flush_buffers(videoDecoderCtx.raw());
        formatCtx.seek((int64_t) (currentTime) * (int64_t)rational.den / (int64_t) rational.num, targetVideoStream.index(), AVSEEK_FLAG_BACKWARD);
        needsSeeking = false;

        DecodeOneFrameWithoutRescaling();
    }

    std::optional<glm::vec2> VideoDecoder::GetResolution() {
        if (videoDecoderCtx.isOpened()) {
            return glm::vec2(videoDecoderCtx.width(), videoDecoderCtx.height());
        }
        return std::nullopt;
    }

    std::optional<float> VideoDecoder::GetFramerate() {
        return framerate;
    }
};