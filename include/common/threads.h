#pragma once

#include "raster.h"

namespace Raster {
    struct Threads {
        static std::thread::id s_audioThreadID;
    };
};