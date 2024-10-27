#include "common/audio_info.h"

namespace Raster {
    int AudioInfo::s_channels = 2;
    int AudioInfo::s_sampleRate = 48000;
    int AudioInfo::s_periodSize = 4096;

    int AudioInfo::s_globalAudioOffset = 0;

    int AudioInfo::s_audioPassID = 1;

    SharedRawAudioSamples AudioInfo::MakeRawAudioSamples() {
        return std::make_shared<std::vector<float>>(s_periodSize * s_channels);
    }
};