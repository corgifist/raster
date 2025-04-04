#pragma once

#include "raster.h"
#include "gpu/gpu.h"
#include "common/double_buffered_value.h"

namespace Raster {
    struct DoubleBufferedFramebuffer {
        uint32_t width, height;

        DoubleBufferedFramebuffer();
        DoubleBufferedFramebuffer(int width, int height, TexturePrecision precision = TexturePrecision::Usual);

        Framebuffer& Get();
        Framebuffer& GetWithOffset(int offset = 0);

        Framebuffer& GetFrontFramebuffer();
        Framebuffer& GetFrontFramebufferWithoutSwapping();

        void SwapBuffers();
        void Destroy();
        private:
            int m_index;
            Framebuffer m_front, m_back;
    };
};