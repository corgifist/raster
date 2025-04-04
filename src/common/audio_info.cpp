#include "common/audio_info.h"
#include "common/audio_memory_management.h"

namespace Raster {
    int AudioInfo::s_channels = 2;
    int AudioInfo::s_sampleRate = 48000;
    int AudioInfo::s_periodSize = 4096;

    int AudioInfo::s_globalAudioOffset = 0;

    int AudioInfo::s_audioPassID = 1;

    SharedMutex AudioInfo::s_mutex;

    float* AudioInfo::MakeRawAudioSamples() {
        return (float*) AudioMemoryManagement::Allocate(s_periodSize * s_channels * sizeof(float));
    }
};