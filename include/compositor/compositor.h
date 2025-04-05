#pragma once

#include "raster.h"
#include "common/common.h"
#include "gpu/gpu.h"
#include "common/workspace.h"
#include "common/composition.h"
#include "compositor/blending.h"
#include "double_buffered_framebuffer.h"
#include "common/synchronized_value.h"
#include "common/composition_mask.h"

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
        std::vector<CompositionMask> masks;
    };

    struct Compositor {
        static std::optional<DoubleBufferedFramebuffer> primaryFramebuffer;
        static float previewResolutionScale;
        static DoubleBufferedValue<std::unordered_map<int, RenderableBundle>> s_bundles;
        static SynchronizedValue<std::vector<CompositorTarget>> s_targets;
        static Blending s_blending;
        static Pipeline s_pipeline;
        static TexturePrecision s_colorPrecision;

        static void Initialize();

        static void ResizePrimaryFramebuffer(glm::vec2 t_resolution);

        static Framebuffer GenerateCompatibleFramebuffer(glm::vec2 t_resolution, std::optional<TexturePrecision> t_precision = std::nullopt);
        static DoubleBufferedFramebuffer GenerateCompatibleDoubleBufferedFramebuffer(glm::vec2 t_resolution, std::optional<TexturePrecision> t_precision = std::nullopt);

        static void EnsureResolutionConstraints();
        static void EnsureResolutionConstraintsForFramebuffer(Framebuffer& t_fbo);
        static void EnsureResolutionConstraintsForFramebuffer(DoubleBufferedFramebuffer& t_fbo);

        static void PerformManualComposition(std::vector<CompositorTarget> t_targets, Framebuffer& t_fbo, std::optional<glm::vec4> t_backgroundColor = std::nullopt, std::optional<Blending*> t_blending = std::nullopt, std::optional<Pipeline> s_pipeline = std::nullopt);
        static Framebuffer PerformComposition(std::vector<int> t_allowedCompositions = {});

        static glm::vec2 GetRequiredResolution();
    };
};