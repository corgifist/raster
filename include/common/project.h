#pragma once
#include "raster.h"
#include "typedefs.h"
#include "assets.h"

namespace Raster {
    struct Project {
        std::string path;
        std::string name, description;
        float framerate;
        float currentFrame;
        float timeTravelOffset;
        bool playing, looping;

        glm::vec2 preferredResolution;
        glm::vec4 backgroundColor;
        
        std::vector<Composition> compositions;
        std::vector<AbstractAsset> assets;

        std::vector<int> selectedCompositions;
        std::vector<int> selectedNodes;
        std::vector<int> selectedAttributes;
        std::vector<int> selectedKeyframes;
        std::vector<int> selectedAssets;

        Json customData;

        Project();
        Project(Json data);

        float GetProjectLength();
        float GetCurrentTime();

        void TimeTravel(float t_offset);
        void ResetTimeTravel();
        std::string FormatFrameToTime(float frame);

        glm::mat4 GetProjectionMatrix(bool inverted = false);

        Json Serialize();
    };
};