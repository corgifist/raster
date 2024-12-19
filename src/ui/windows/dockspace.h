#pragma once

#include "raster.h"
#include "ui/ui.h"
#include "common/common.h"
#include "font/IconsFontAwesome5.h"

#include "../../ImGui/imgui.h"
#include "../../ImGui/imgui_internal.h"
#include "../../ImGui/imgui_stdlib.h"
#include "../../ImGui/stb_sprintf.h"

namespace Raster {
    struct DockspaceUI : public UI {

        void RenderAboutWindow();
        void RenderNewProjectPopup();

        void RenderPreferencesModal();

        void Render();
    };
};