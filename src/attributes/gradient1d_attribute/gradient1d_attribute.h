#pragma once

#include "raster.h"
#include "common/typedefs.h"
#include "common/attribute.h"
#include "common/composition.h"
#include "common/workspace.h"
#include "common/gradient_1d.h"
#include "../../ImGui/imgui.h"
#include "../../ImGui/imgui_stdlib.h"



namespace Raster {

    struct Gradient1DAttribute : public AttributeBase {
    public:
        Gradient1DAttribute();

        std::any AbstractInterpolate(std::any t_beginValue, std::any t_endValue, float t_percentage, float t_frame, Composition* composition);
        std::any AbstractRenderLegend(Composition* t_composition, std::any t_originalValue, bool& isItemEdited);

        void RenderKeyframes();

        void AbstractRenderDetails();

        Json SerializeKeyframeValue(std::any t_value);
        std::any LoadKeyframeValue(Json t_value);
    private:
        Gradient1D m_persistentGradient;
        bool m_gradientHovered;
    };
};