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
    struct AudioMonitorUI : public UserInterfaceBase {
        bool open;
        int busID;
        ImVec2 audioMonitorChildSize;

        AudioMonitorUI() : UserInterfaceBase(), open(true), busID(-1), audioMonitorChildSize(100, 300) {}
        AudioMonitorUI(int t_busID) : UserInterfaceBase(), open(true), busID(t_busID), audioMonitorChildSize(100, 300) {}

        void AbstractRender();
        Json AbstractSerialize();
        void AbstractLoad(Json t_data);
    };
};