#pragma once

#include "raster.h"
#include "common/typedefs.h"
#include "common/attribute.h"
#include "common/composition.h"
#include "common/workspace.h"
#include "common/transform2d.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_stdlib.h"

#define TRANSFORM2D_PARENT_PAYLOAD "TRANSFORM2D_PARENT_PAYLOAD"

namespace Raster {
    struct Transform2DAttribute : public AttributeBase {
    public:
        Transform2DAttribute();

        std::any AbstractInterpolate(std::any t_beginValue, std::any t_endValue, float t_percentage, float t_frame, Composition* composition);
        std::any AbstractRenderLegend(Composition* t_composition, std::any t_originalValue, bool& isItemEdited);

        void RenderKeyframes();

        void AbstractRenderDetails();

        void Load(Json t_data);

        Json AbstractSerialize();
    private:
        bool m_linkedSize;
        int m_parentAttributeID;
    };
};