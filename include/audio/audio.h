#pragma once

#include "raster.h"
#include "common/audio_samples.h"

namespace Raster {

    struct AudioBackendInfo {
        std::string name, version;
    };

    struct Audio {
        static AudioBackendInfo s_backendInfo;
        static std::mutex s_audioMutex;

        static void Initialize(int t_channelCount, int t_sampleRate);
    };
};