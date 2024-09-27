#pragma once
#include "raster.h"
#include "common/common.h"

#include "../../../avcpp/av.h"
#include "../../../avcpp/ffmpeg.h"
#include "../../../avcpp/codec.h"
#include "../../../avcpp/packet.h"
#include "../../../avcpp/videorescaler.h"
#include "../../../avcpp/audioresampler.h"
#include "../../../avcpp/avutils.h"

// API2
#include "../../../avcpp/format.h"
#include "../../../avcpp/formatcontext.h"
#include "../../../avcpp/codec.h"
#include "../../../avcpp/codeccontext.h"

#include "audio/audio.h"

#include "common/audio_samples.h"

using namespace av;

// 100 audio passes
#define MAX_DECODER_LIFESPAN 100

namespace Raster {

    struct AudioDecoderContext {
        av::FormatContext formatCtx;
        av::Stream targetAudioStream;
        av::AudioDecoderContext audioDecoderCtx;
        av::AudioResampler audioResampler;
        int resamplerSamplesCount;
        SharedRawAudioSamples cachedSamples;
        bool cacheValid;

        bool wasOpened;
        int lastAudioPassID;
        bool needsSeeking;
        int health;
        int assetID;
        std::string assetPath;

        AudioDecoderContext();
    };

    using SharedDecoderContext = std::shared_ptr<AudioDecoderContext>;

    struct DecodeAudioAsset : public NodeBase {
        DecodeAudioAsset();
        
        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();

        void AbstractOnTimelineSeek();
        std::optional<float> AbstractGetContentDuration();

    private:
        SharedDecoderContext GetDecoderContext();

        av::AudioSamples PopResampler(SharedDecoderContext t_context);
        void PushMoreSamples(SharedDecoderContext t_context);
        av::AudioSamples DecodeOneFrame(SharedDecoderContext t_context);
        void FlushResampler(SharedDecoderContext t_context);
        void SeekDecoder(SharedDecoderContext t_context);

        std::unordered_map<float, SharedDecoderContext> m_decoderContexts;
        std::shared_ptr<std::mutex> m_decodingMutex;
        bool m_forceSeek;
    };
};