#pragma once

#include "raster.h"
#include "common/common.h"
#include "gpu/gpu.h"
#include "common/workspace.h"

namespace Raster {

    struct RenderableBundle {
        Framebuffer primaryFramebuffer;
        std::unordered_map<std::string, Framebuffer> attachments;
    };

    struct CompositorTarget {
        Texture colorAttachment, uvAttachment;
    };

    struct Compositor {
        static std::optional<Framebuffer> primaryFramebuffer;
        static float previewResolutionScale;
        static std::unordered_map<int, RenderableBundle> s_bundles;
        static std::vector<CompositorTarget> s_targets;
        static Pipeline s_pipeline;

        static void Initialize();

        static void ResizePrimaryFramebuffer(glm::vec2 t_resolution);

        static Framebuffer GenerateCompatibleFramebuffer(glm::vec2 t_resolution);

        static void EnsureResolutionConstraints();

        static void PerformComposition();

        static glm::vec2 GetRequiredResolution();
    };
};