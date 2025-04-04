#pragma once

#include "raster.h"
#include "common/user_interface.h"
#include "common/common.h"
#include "utils/widgets.h"
#include "font/IconsFontAwesome5.h"
#include "font/font.h"

#include "../../../ImGui/imgui.h"
#include "../../../ImGui/imgui_stdlib.h"
#include "../../../ImGui/imgui_stripes.h"

namespace Raster {
    struct EasingEditorUI : public UserInterfaceBase {
        bool open;
        EasingEditorUI() : UserInterfaceBase(), open(true) {}

        void AbstractRender();
        Json AbstractSerialize();
        void AbstractLoad(Json t_data);
    };
};