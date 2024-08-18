#pragma once

#include "raster.h"
#include "ui/ui.h"
#include "common/common.h"
#include "utils/widgets.h"
#include "font/IconsFontAwesome5.h"
#include "font/font.h"

#include "../../ImGui/imgui.h"
#include "../../ImGui/imgui_stdlib.h"
#include "../../ImGui/imgui_stripes.h"

namespace Raster {
    struct AssetManagerUI : public UI {
        void Render();
    };
};