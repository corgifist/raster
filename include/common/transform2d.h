#pragma once

#include "raster.h"
#include "common/common.h"

namespace Raster {
    struct Transform2D {
        glm::vec2 position, size, anchor;
        float angle;

        Transform2D();
        Transform2D(Json t_data);

        glm::mat4 GetTransformationMatrix();
        glm::vec2 GetTransformedPosition();
        glm::vec2 GetTransformedSize();

        Json Serialize();
    };
}