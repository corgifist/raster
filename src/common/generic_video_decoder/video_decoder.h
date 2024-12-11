#pragma once
#include "raster.h"

#include "../../avcpp/codec.h"
#include "../../avcpp/packet.h"
#include "../../avcpp/videorescaler.h"

// API2
#include "../../avcpp/format.h"
#include "../../avcpp/formatcontext.h"
#include "../../avcpp/codec.h"
#include "../../avcpp/codeccontext.h"
#include "common/synchronized_value.h"
#include "common/audio_cache.h"
#include <cstddef>


#define MAX_GENERIC_VIDEO_DECODER_LIFESPAN 5

namespace Raster {
    struct VideoDecoder {
        av::FormatContext formatCtx;
        av::Stream targetVideoStream;
        av::VideoDecoderContext videoDecoderCtx;
        av::VideoRescaler videoRescaler;

        bool wasOpened;
        bool needsSeeking;
        int health;
        int assetID;
        std::string assetPath;

        float timeOffset;
        float framerate;
        int id;
        size_t lastLoadedFrame;
        size_t targetFrame;

        std::vector<size_t> keyframes; 

        bool currentlyDecoding;
        float percentage;

        VideoDecoder() {
            this->assetID = 0;
            this->needsSeeking = true;
            this->wasOpened = false;
            this->health = MAX_GENERIC_VIDEO_DECODER_LIFESPAN;
            this->timeOffset = 0;
            this->framerate = 0;
            this->lastLoadedFrame = -1;
            this->targetFrame = 0;
            this->currentlyDecoding = false;
            this->percentage = -1;
        }

        size_t GetCurrentKeyFrameForFrame(size_t frame);

        VideoDecoder(VideoDecoder const&) = delete;
        VideoDecoder& operator=(VideoDecoder const&) = delete;

        // returns true if this decoder uses
        // h.263 / h.264 / h.265 decoders
        bool IsUsingH264();

        void FlushResampler();
        av::Packet ReadPacket();
        av::VideoFrame DecodeOneFrame();
        av::VideoFrame DecodeOneFrameWithoutRescaling();
        void SeekDecoder(float t_second);
        std::optional<glm::vec2> GetResolution();
        std::optional<float> GetFramerate();
    };

    using SharedVideoDecoder = std::shared_ptr<VideoDecoder>;
};