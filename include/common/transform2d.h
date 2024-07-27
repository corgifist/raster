#pragma once

#include "raster.h"
#include "common/common.h"

namespace Raster {
    struct Transform2D {
        glm::vec2 position, size, anchor;
        float angle;

        glm::mat4 parentMatrix;

        Transform2D();
        Transform2D(Json t_data);

        glm::mat4 GetTransformationMatrix();

        Json Serialize();
    };
}