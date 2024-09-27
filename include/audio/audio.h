#pragma once

#include "raster.h"

namespace Raster {

    struct AudioBackendInfo {
        std::string name, version;
    };

    struct Audio {
        static AudioBackendInfo s_backendInfo;
        static int s_samplesCount;
        static int s_globalAudioOffset;

        static void Initialize(int t_channelCount, int t_sampleRate);
        static int GetChannelCount();
        static int ClampAudioIndex(int t_index);
        static int GetSampleRate();
    };
};