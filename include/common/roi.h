#pragma once

#include "raster.h"

namespace Raster {
    struct ROI {
        glm::vec2 upperLeft;
        glm::vec2 bottomRight;

        ROI() : upperLeft(1, -1), bottomRight(-1, 1) {}
        ROI(glm::vec2 t_upperLeft, glm::vec2 t_bottomRight) : upperLeft(t_upperLeft), bottomRight(t_bottomRight) {}
        ROI(Json t_data);

        Json Serialize();
    };
};