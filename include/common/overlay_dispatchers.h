#pragma once
#include "raster.h"
#include "typedefs.h"
#include "common/composition.h"

namespace Raster {
    struct OverlayDispatchers {
        static bool DispatchTransform2DValue(std::any& t_attribute, Composition* t_composition, int t_attributeID, float t_zoom, glm::vec2 t_regionSize);
    };
};