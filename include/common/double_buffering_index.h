#pragma once

#include "raster.h"
namespace Raster {
    struct DoubleBufferingIndex {
        static int s_index;
        static std::mutex s_mutex;
    };
};