#pragma once

#include "raster.h"
#include "common/common.h"
#include "gpu/gpu.h"

#define RASTER_BLENDING_PLACEHOLDER "__GENERATED_CODE_GOES_HERE__"
#define RASTER_BLENDING_FUNCTIONS_PLACEHOLDER "__GENERATED_FUNCTIONS_GO_HERE__"

namespace Raster {
    struct BlendingMode {
        std::string name, codename, formula, functions, icon;

        BlendingMode();
        BlendingMode(Json mode);

        Json Serialize();
    };

    struct Blending {
        std::vector<BlendingMode> modes;
        std::optional<Pipeline> pipelineCandidate;
        std::optional<Framebuffer> framebufferCandidate;

        Blending();
        Blending(Json json);

        Framebuffer PerformBlending(BlendingMode& mode, Texture base, Texture blend, float opacity);

        void GenerateBlendingPipeline();
        void EnsureResolutionConstraints(Texture& texture);

        std::optional<int> GetModeIndexByCodeName(std::string t_codename);
        std::optional<BlendingMode> GetModeByCodeName(std::string t_codename);

        Json Serialize();
    };
};