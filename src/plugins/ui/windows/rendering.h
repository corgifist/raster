#pragma once

#include "common/user_interface.h"
#include "raster.h"
#include "common/user_interfaces.h"
#include "common/common.h"
#include "font/IconsFontAwesome5.h"

#include "../../../ImGui/imgui.h"
#include "../../../ImGui/imgui_stdlib.h"

namespace Raster {
    struct RenderingUI : public UserInterfaceBase {
        bool open;
        RenderingUI() : UserInterfaceBase(), open(true) {}

        void AbstractRender();
        Json AbstractSerialize();
        void AbstractLoad(Json t_data);
    };
};