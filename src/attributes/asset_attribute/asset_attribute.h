#pragma once

#include "raster.h"
#include "common/typedefs.h"
#include "common/attribute.h"
#include "common/composition.h"
#include "common/workspace.h"
#include "../../ImGui/imgui.h"
#include "../../ImGui/imgui_stdlib.h"

#define ASSET_MANAGER_DRAG_DROP_PAYLOAD "ASSET_MANAGER_DRAG_DROP_PAYLOAD"

// ASSET_ATTRIBUTE_DRAG_DROP_PAYLOAD is too long (more than 32 characters), so we're stripping the payload type name down
#define ASSET_ATTRIBUTE_DRAG_DROP_PAYLOAD "ASSETATTRIBUTEDRAGDROPPAYLOAD"

namespace Raster {
    struct AssetAttribute : public AttributeBase {
        AssetAttribute();

        std::any AbstractInterpolate(std::any t_beginValue, std::any t_endValue, float t_percentage, float t_frame, Composition* composition);
        std::any AbstractRenderLegend(Composition* t_composition, std::any t_originalValue, bool& isItemEdited);

        Json SerializeKeyframeValue(std::any t_value);
        std::any LoadKeyframeValue(Json t_value);

        void RenderKeyframes();

        void AbstractRenderDetails();
    };
};