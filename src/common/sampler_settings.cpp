#include "common/sampler_settings.h"

namespace Raster {
    SamplerSettings::SamplerSettings() {
        this->filteringMode = TextureFilteringMode::Linear;
        this->wrappingMode = TextureWrappingMode::Repeat;
    }
};