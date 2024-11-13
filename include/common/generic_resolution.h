#pragma once

#include "raster.h"

namespace Raster {
    struct GenericResolution {
        glm::vec2 rawResolution;
        bool useRawResolution;
        bool useProjectAspectResolution;
        bool useRawResolutionAsReference;
        float aspectRatio;

        GenericResolution();
        GenericResolution(Json t_data);

        glm::vec2 CalculateResolution();

        Json Serialize();
    };
};