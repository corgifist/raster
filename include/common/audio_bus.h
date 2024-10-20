#pragma once

#include "raster.h"
#include "randomizer.h"

namespace Raster {
    struct AudioBus {
        int id;
        int redirectID; // negative if not redirecting, positive if redirecting to some audio bus
        bool main;
        std::string name;
        std::vector<float> samples;
        uint32_t colorMark;

        AudioBus();
        AudioBus(Json t_data);

        Json Serialize();
    };
};