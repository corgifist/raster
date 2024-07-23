#pragma once

#include "raster.h"
#include "gpu/gpu.h"
#include "compositor/compositor.h"

namespace Raster {
    struct ManagedFramebuffer {
    public:
        ManagedFramebuffer();
        Framebuffer& Get(std::optional<Framebuffer> t_framebuffer);

    private:
        void EnsureResolutionConstraints();
        void InstantiateInternalFramebuffer(uint32_t width, uint32_t height);
        void DestroyInternalFramebuffer();

        Framebuffer m_internalFramebuffer;
    };
};