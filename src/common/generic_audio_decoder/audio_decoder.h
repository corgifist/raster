#pragma once
#include "raster.h"

#include "../../avcpp/codec.h"
#include "../../avcpp/packet.h"
#include "../../avcpp/audioresampler.h"

// API2
#include "../../avcpp/format.h"
#include "../../avcpp/formatcontext.h"
#include "../../avcpp/codec.h"
#include "../../avcpp/codeccontext.h"
#include "common/synchronized_value.h"
#include "common/audio_cache.h"
#include <rubberband/RubberBandStretcher.h>


#define MAX_GENERIC_AUDIO_DECODER_LIFESPAN 5

namespace Raster {
    struct AudioDecoder {
        av::FormatContext formatCtx;
        av::Stream targetAudioStream;
        av::AudioDecoderContext audioDecoderCtx;
        av::AudioResampler audioResampler;
        bool cacheValid;
        SynchronizedValue<AudioCache> cache;
        std::shared_ptr<RubberBand::RubberBandStretcher> stretcher;
        int stretcherSampleRate;

        bool wasOpened;
        int lastAudioPassID;
        bool needsSeeking;
        int health;
        int assetID;
        std::string assetPath;

        float timeOffset;
        int id;

        AudioDecoder() {
            this->lastAudioPassID = INT_MIN;
            this->assetID = 0;
            this->needsSeeking = true;
            this->wasOpened = false;
            this->health = MAX_GENERIC_AUDIO_DECODER_LIFESPAN;
            this->cacheValid = false;
            this->timeOffset = 0;
            this->stretcher = nullptr;
            this->stretcherSampleRate = 0;
        }

        AudioDecoder(AudioDecoder const&) = delete;
        AudioDecoder& operator=(AudioDecoder const&) = delete;

        void FlushResampler();
        av::AudioSamples DecodeOneFrame();
        av::AudioSamples PopResampler();
        bool PushMoreSamples();
        void SeekDecoder(float t_second);
    };

    using SharedAudioDecoder = std::shared_ptr<AudioDecoder>;
};