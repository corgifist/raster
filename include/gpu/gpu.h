#pragma once

#include "raster.h"

namespace Raster {
    struct GPUInfo {
        std::string renderer;
        std::string version;

        void* display;
    };

    struct GPU {
        static GPUInfo info;

        static void Initialize();
        static bool MustTerminate();
        static void BeginFrame();
        static void EndFrame();

        static void Terminate();

        static void* GetImGuiContext();
    };
}