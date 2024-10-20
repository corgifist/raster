#pragma once

#include "raster.h"
#include "common/typedefs.h"
#include "gpu/gpu.h"

namespace Raster {
    struct SamplerSettings {
        TextureWrappingMode wrappingMode;
        TextureFilteringMode filteringMode;

        SamplerSettings();
        SamplerSettings(Json t_data);

        Json Serialize();
    };
};