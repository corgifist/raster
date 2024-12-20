#pragma once

#include "raster.h"
#include "ui/ui.h"

struct ImFont;

namespace Raster {

    struct App {
        static std::vector<AbstractUI> s_windows;

        static void Start();
        static void WriterThread();
        static void InitializeInternals();
        static void RenderLoop();
        static void Terminate();
    };
};