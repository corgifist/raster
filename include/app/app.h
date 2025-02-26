#pragma once

#include "raster.h"

struct ImFont;

namespace Raster {

    struct App {
        static void Start();
        static void WriterThread();
        static void InitializeInternals();
        static void RenderLoop();
        static void Terminate();
    };
};