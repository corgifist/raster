#pragma once

#include "raster.h"
#include "common/typedefs.h"
#include "common/attribute.h"
#include "common/composition.h"
#include "common/workspace.h"
#include "../../ImGui/imgui.h"
#include "common/common.h"

namespace Raster {
    struct FolderAttribute : public AttributeBase {
        std::vector<AbstractAttribute> attributes;

        FolderAttribute();

        std::any AbstractInterpolate(std::any t_beginValue, std::any t_endValue, float t_percentage, float t_frame, Composition* composition);
        std::any AbstractRenderLegend(Composition* t_composition, std::any t_originalValue, bool& isItemEdited);

        void RenderKeyframes();

        std::optional<std::vector<AbstractAttribute>*> AbstractGetChildAttributes();

        void AbstractRenderDetails();

        Json SerializeKeyframeValue(std::any t_value);
        std::any LoadKeyframeValue(Json t_value);

        Json AbstractSerialize();
        void Load(Json t_data);

    };
};