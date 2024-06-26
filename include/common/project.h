#pragma once
#include "raster.h"
#include "typedefs.h"

namespace Raster {
    struct Project {
        std::string name, description;
        float framerate;
        float currentFrame;
        
        std::vector<Composition> compositions;

        Project();
        Project(Json data);

        uint64_t GetProjectLength();
        std::string FormatFrameToTime(uint64_t frame);

        Json Serialize();
    };
};