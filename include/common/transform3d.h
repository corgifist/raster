#pragma once

#pragma once

#include "raster.h"

namespace Raster {

    struct Transform3D;

    struct Transform3D {
        glm::vec3 position, size, rotation;

        std::shared_ptr<Transform3D> parentTransform;

        Transform3D();
        Transform3D(Json t_data);

        glm::mat4 GetTransformationMatrix();
        glm::mat4 GetParentMatrix();

        glm::vec3 DecomposePosition();
        glm::vec3 DecomposeSize();
        glm::vec3 DecomposeRotation();

        Json Serialize();
    };
}