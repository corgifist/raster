#pragma once

#include "raster.h"
#include "common/user_interface.h"
#include "common/common.h"
#include "font/IconsFontAwesome5.h"
#include "font/font.h"

#include "../../../ImGui/imgui.h"
#include "../../../ImGui/imgui_internal.h"
#include "../../../ImGui/imgui_stdlib.h"
#include "../../../ImGui/stb_sprintf.h"

namespace Raster {
    struct AudioBusesUI : public UserInterfaceBase {

        bool open;
        AudioBusesUI() : UserInterfaceBase(), open(true) {}

        void AbstractRender();
        Json AbstractSerialize();
        void AbstractLoad(Json t_data);
    };
};