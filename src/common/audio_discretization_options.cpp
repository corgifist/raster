#include "common/audio_discretization_options.h"

namespace Raster {
    AudioDiscretizationOptions::AudioDiscretizationOptions(Json t_data) {
        this->desiredChannelsCount = t_data["ChannelsCount"];
        this->desiredSampleRate = t_data["SampleRate"];
        this->performanceProfile = static_cast<AudioPerformanceProfile>(t_data["PerformanceProfile"].get<int>());
    }

    Json AudioDiscretizationOptions::Serialize() {
        return {
            {"ChannelsCount", desiredChannelsCount},
            {"SampleRate", desiredSampleRate},
            {"PerformanceProfile", static_cast<int>(performanceProfile)}
        };
    }
};