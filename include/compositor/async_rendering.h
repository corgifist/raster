#pragma once

#include "raster.h"
#include "compositor.h"

#include "common/common.h"
#include "gpu/gpu.h"
#include "compositor/compositor.h"
#include "common/double_buffering_index.h"

namespace Raster {
    struct AsyncRendering {
        static void* s_context;
        
        static void Initialize();
        static void Terminate();

        static void RenderingLoop();

        static Framebuffer GetFrontBuffer();
        static void SwapBuffers();
        static void AllowRendering();

    private:
        static bool m_running;
        static bool m_allowRendering;
        static std::thread m_renderingThread;
    };
};