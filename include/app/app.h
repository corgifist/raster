#pragma once

#include "raster.h"

namespace Raster{
    struct App {
        static void Initialize();
        static void RenderLoop();
        static void Terminate();
    };
};