#pragma once

#include "raster.h"

namespace Raster {
    struct AudioMemoryManagement {
        static void Initialize(size_t t_bytes);
        static void* Allocate(size_t t_bytes);
        static void Reset();
        static void Terminate();
    };
};