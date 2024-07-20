#pragma once
#include "raster.h"
#include "typedefs.h"

namespace Raster {
    struct Project {
        std::string name, description;
        float framerate;
        float currentFrame;

        glm::vec2 preferredResolution;
        glm::vec4 backgroundColor;
        
        std::vector<Composition> compositions;

        Json customData;

        Project();
        Project(Json data);

        float GetProjectLength();
        std::string FormatFrameToTime(float frame);

        glm::mat4 GetProjectionMatrix();

        Json Serialize();
    };
};