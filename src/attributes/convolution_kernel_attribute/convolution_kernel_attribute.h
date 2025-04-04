#pragma once

#include "common/bezier_curve.h"
#include "raster.h"
#include "common/typedefs.h"
#include "common/attribute.h"
#include "common/composition.h"
#include "common/workspace.h"
#include "../../ImGui/imgui.h"
#include "common/common.h"
#include "common/convolution_kernel.h"

namespace Raster {
    struct ConvolutionKernelAttribute : public AttributeBase {
        ConvolutionKernelAttribute();

        bool interpretAsColor;

        std::any AbstractInterpolate(std::any t_beginValue, std::any t_endValue, float t_percentage, float t_frame, Composition* composition);
        std::any AbstractRenderLegend(Composition* t_composition, std::any t_originalValue, bool& isItemEdited);

        void RenderKeyframes();

        void AbstractRenderDetails();

        Json SerializeKeyframeValue(std::any t_value);
        std::any LoadKeyframeValue(Json t_value);

    };
};