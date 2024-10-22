#pragma once

#include "raster.h"
#include "gpu/gpu.h"

namespace Raster {
    using SharedRawAudioSamples = std::shared_ptr<std::vector<float>>;

    // TODO: implement ability to store multiple attached pictures
    struct AudioSamples {
        int sampleRate;
        SharedRawAudioSamples samples;
        std::vector<Texture> attachedPictures;

        AudioSamples();
    };
};