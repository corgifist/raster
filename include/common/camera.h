#pragma once

#include "raster.h"

namespace Raster {
    struct Camera {
        glm::vec3 position;
        glm::vec3 rotation;
        glm::vec3 anchor;
        float perspNear, perspFar;
        bool customF;
        float f;
        float fSize, fLength;
        bool persp;
        float orthoWidth;
        bool enabled;

        Camera() : position(0, 0, 0), rotation(0, 0, 0), anchor(0, 0, 0), perspNear(0.001), perspFar(1000.0f), customF(false), f(45.0f), fSize(36), fLength(50), persp(true), orthoWidth(1), enabled(true) {}
        Camera(Json t_data);

        glm::mat4 GetProjectionMatrix();
        glm::mat4 GetTransformationMatrix();

        float GetF();

        Json Serialize();
    };
};