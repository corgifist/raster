#pragma once

#include "audio_samples.h"
#include "shared_mutex.h"

namespace Raster {
    struct AudioInfo {
        static int s_channels, s_sampleRate, s_periodSize;
        static int s_globalAudioOffset;
        static int s_audioPassID;

        static SharedMutex s_mutex;

        static float* MakeRawAudioSamples();
    };
};