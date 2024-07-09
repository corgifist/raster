#pragma once

#include "raster.h"
#include "common/common.h"
#include "gpu/gpu.h"
#include "common/workspace.h"

namespace Raster {
    struct Compositor {
        static std::optional<Framebuffer> primaryFramebuffer;
        static float previewResolutionScale;

        static void ResizePrimaryFramebuffer(glm::vec2 t_resolution);

        static Framebuffer GenerateCompatibleFramebuffer(glm::vec2 t_resolution);

        static void EnsureResolutionConstraints();
        static glm::vec2 GetRequiredResolution();
    };
};