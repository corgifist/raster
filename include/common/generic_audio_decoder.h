#pragma once

#include "raster.h"
#include "common.h"
#include "audio_samples.h"
#include "audio_cache.h"
#include "shared_mutex.h"

#define MAX_GENERIC_AUDIO_DECODER_LIFESPAN 5

namespace Raster {

    // default audio decoder used by Raster 
    // used by `Decode Audio Data` under the hood
    struct GenericAudioDecoder {
        int assetID;
        std::shared_ptr<std::unordered_map<float, int>> decoderContexts;
        std::shared_ptr<float> seekTarget;

        GenericAudioDecoder();

        std::optional<AudioSamples> DecodeSamples();
        void Seek(float t_second);
        std::optional<float> GetContentDuration();

    private:
        SharedMutex m_decodingMutex;
    };
};