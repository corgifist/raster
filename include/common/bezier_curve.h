#pragma once

#include "raster.h"

namespace Raster {
    struct BezierCurve {
        std::vector<glm::vec2> points;
        bool smoothCurve;

        BezierCurve();
        BezierCurve(Json t_data);

        glm::vec2 Get(float t_percentage);

        Json Serialize();
    };
};