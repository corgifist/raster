#pragma once

#include "raster.h"
#include "gpu/gpu.h"
#include "compositor/compositor.h"
#include "double_buffered_framebuffer.h"

namespace Raster {
    struct GeometryFramebuffer {
    public:
        GeometryFramebuffer();
        ~GeometryFramebuffer();

        Framebuffer& Get(std::optional<Framebuffer> t_framebuffer);
        void Destroy();

    private:
        void EnsureResolutionConstraints(std::optional<Framebuffer> t_framebuffer);
        void InstantiateInternalFramebuffer(uint32_t width, uint32_t height);
        void DestroyInternalFramebuffer();

        DoubleBufferedFramebuffer m_internalFramebuffer;
    };
};