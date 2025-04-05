#pragma once

#include "common/user_interface.h"
#include "raster.h"
#include "common/user_interfaces.h"
#include "common/common.h"
#include "font/IconsFontAwesome5.h"

#include "../ImGui/imgui.h"
#include "../ImGui/imgui_internal.h"
#include "../ImGui/imgui_stdlib.h"
#include "../ImGui/stb_sprintf.h"

namespace Raster {
    struct DockspaceUI : public UserInterfaceBase {
        void RenderAboutWindow();
        void RenderNewProjectPopup();

        void RenderPreferencesModal();

        void RenderLayoutsModal();

        void AbstractRender();
        Json AbstractSerialize();
        void AbstractLoad(Json t_data);
    };
};