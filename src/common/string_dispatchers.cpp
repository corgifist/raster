#include "common/common.h"
#include "gpu/gpu.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_stdlib.h"

namespace Raster {

    static ImVec2 FitRectInRect(ImVec2 dst, ImVec2 src) {
        float scale = std::min(dst.x / src.x, dst.y / src.y);
        return ImVec2{src.x * scale, src.y * scale};
    }

    void StringDispatchers::DispatchStringValue(std::any& t_attribute) {
        ImGui::Text("%s %s: '%s'", ICON_FA_QUOTE_LEFT, Localization::GetString("VALUE").c_str(), std::any_cast<std::string>(t_attribute).c_str());
    }

    void StringDispatchers::DispatchTextureValue(std::any& t_attribute) {
        auto texture = std::any_cast<Texture>(t_attribute);
        ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - 64);
        ImGui::Image(texture.handle, FitRectInRect(ImVec2(128, 128), ImVec2(texture.width, texture.height)));
        
        auto footerText = FormatString("%ix%i; %s", (int) texture.width, (int) texture.height, texture.PrecisionToString().c_str());
        ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - ImGui::CalcTextSize(footerText.c_str()).x / 2.0f);
        ImGui::Text(footerText.c_str());
    }

    void StringDispatchers::DispatchFloatValue(std::any& t_attribute) {
        ImGui::Text("%s %s: %0.2f", ICON_FA_CIRCLE_INFO, Localization::GetString("VALUE").c_str(), std::any_cast<float>(t_attribute));
    }
};