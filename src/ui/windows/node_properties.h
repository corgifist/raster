#pragma once

#include "raster.h"
#include "ui/ui.h"
#include "common/common.h"
#include "utils/widgets.h"
#include "font/IconsFontAwesome5.h"

#include "../../ImGui/imgui.h"
#include "../../ImGui/imgui_stdlib.h"
#include "../../ImGui/imgui_node_editor.h"

namespace Raster {
    struct NodePropertiesUI : public UI {
        bool CenteredButton(const char* label, float alignment = 0.5f);
        void Render();

        template <typename T>
        std::basic_string<T> StringLowercase(const std::basic_string<T>& s) {
            std::basic_string<T> s2 = s;
            std::transform(s2.begin(), s2.end(), s2.begin(),
                [](const T v){ return static_cast<T>(std::tolower(v)); });
            return s2;
        }
    };
};