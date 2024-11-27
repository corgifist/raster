#pragma once

#include "raster.h"

namespace Raster {

    enum class AudioPerformanceProfile {
        Conservative, LowLatency
    };

    struct AudioDiscretizationOptions {
        int desiredSampleRate;
        int desiredChannelsCount;
        AudioPerformanceProfile performanceProfile;

        AudioDiscretizationOptions() : desiredSampleRate(44100), 
                                       desiredChannelsCount(2), 
                                       performanceProfile(AudioPerformanceProfile::LowLatency) {}
        
        AudioDiscretizationOptions(Json t_data);

        Json Serialize();
    };
};