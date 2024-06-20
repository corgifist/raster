#pragma once

#include "raster.h"
#include "common/typedefs.h"
#include "common/attribute.h"
#include "common/composition.h"
#include "common/workspace.h"
#include "../ImGui/imgui.h"

namespace Raster {
    struct FloatAttribute : public AttributeBase {
        FloatAttribute();

        std::any Get(float t_frame, Composition* composition);
        void RenderKeyframes();
        void RenderLegend(Composition* t_composition);

        void Load(Json t_data);

        Json AbstractSerialize();
    };
};