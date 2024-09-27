#pragma once

#include "raster.h"
#include "gpu/gpu.h"

namespace Raster {
    using SharedRawAudioSamples = std::shared_ptr<std::vector<float>>;

    struct AudioSamples {
        int sampleRate;
        SharedRawAudioSamples samples;
        std::optional<Texture> attachedPicture;

        AudioSamples();
    };
};