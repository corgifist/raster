#pragma once

#include "raster.h"
#include "synchronized_value.h"

namespace Raster {
    struct DoubleBufferingIndex {
        static SynchronizedValue<int> s_index;
    };
};