#pragma once

#include "common/user_interface.h"
#include "raster.h"
#include "common/user_interfaces.h"
#include "common/common.h"
#include "utils/widgets.h"
#include "font/IconsFontAwesome5.h"
#include "common/ui_helpers.h"

#include "../../../ImGui/imgui.h"
#include "../../../ImGui/imgui_stdlib.h"
#include "../../../ImGui/imgui_node_editor.h"

namespace Raster {
    struct NodePropertiesUI : public UserInterfaceBase {
        bool open;
        NodePropertiesUI() : UserInterfaceBase(), open(true) {}

        void AbstractRender();
        Json AbstractSerialize();
        void AbstractLoad(Json t_data);
    };
};