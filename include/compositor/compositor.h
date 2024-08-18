#pragma once

#include "raster.h"
#include "common/common.h"
#include "gpu/gpu.h"
#include "common/workspace.h"
#include "common/composition.h"
#include "compositor/blending.h"

namespace Raster {

    struct RenderableBundle {
        Framebuffer primaryFramebuffer;
        std::unordered_map<std::string, Framebuffer> attachments;
    };

    struct CompositorTarget {
        Texture colorAttachment, uvAttachment;
        float opacity;
        std::string blendMode;
        int compositionID;
    };

    struct Compositor {
        static std::optional<Framebuffer> primaryFramebuffer;
        static float previewResolutionScale;
        static std::unordered_map<int, RenderableBundle> s_bundles;
        static std::vector<CompositorTarget> s_targets;
        static Blending s_blending;
        static Pipeline s_pipeline;

        static void Initialize();

        static void ResizePrimaryFramebuffer(glm::vec2 t_resolution);

        static Framebuffer GenerateCompatibleFramebuffer(glm::vec2 t_resolution);

        static void EnsureResolutionConstraints();
        static void EnsureResolutionConstraintsForFramebuffer(Framebuffer& t_fbo);

        static void PerformManualComposition(std::vector<CompositorTarget> t_targets);
        static void PerformComposition(std::vector<int> t_allowedCompositions = {});

        static glm::vec2 GetRequiredResolution();
    };
};