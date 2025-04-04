#pragma once

#include "raster.h"

namespace Raster {

    struct Transform2D;

    struct Transform2D {
        glm::vec2 position, size, anchor;
        float angle;
        float scale;

        std::shared_ptr<Transform2D> parentTransform;

        Transform2D();
        Transform2D(Json t_data);

        glm::mat4 GetTransformationMatrix();
        glm::mat4 GetParentMatrix();

        glm::vec2 DecomposePosition();
        glm::vec2 DecomposeSize();
        float DecomposeRotation();

        Json Serialize();
    };
}