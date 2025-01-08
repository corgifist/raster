#pragma once

#include "raster.h"
#include "common.h"
#include "audio_samples.h"
#include "audio_cache.h"
#include "shared_mutex.h"

namespace Raster {

    // default audio decoder used by Raster 
    // used by `Decode Audio Data` under the hood
    //
    // field waveformDecoderContexts is only used by WaveformManager to calculate audio waveforms
    // and should not be used by anybody else
    //
    struct GenericAudioDecoder {
        int assetID;
        std::shared_ptr<std::unordered_map<float, int>> decoderContexts;
        std::shared_ptr<std::unordered_map<float, int>> waveformDecoderContexts;
        std::shared_ptr<float> seekTarget;

        GenericAudioDecoder();

        std::optional<AudioSamples> DecodeSamples(int t_audioPassID, ContextData t_contextData);
        std::optional<AudioSamples> GetCachedSamples();
        void Seek(float t_second);
        std::optional<float> GetContentDuration();

        void Destroy();

    private:
        SharedMutex m_decodingMutex;
    };
};