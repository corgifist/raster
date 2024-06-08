#include "common/common.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_stdlib.h"

namespace Raster {
    void AttributeDispatchers::DispatchStringAttribute(NodeBase* t_owner, std::string t_attrbute, std::any& t_value, bool t_isAttributeExposed) {
        std::string string = std::any_cast<std::string>(t_value);
        if (t_isAttributeExposed) {
            float originalCursorX = ImGui::GetCursorPosX();
            ImGui::SetCursorPosX(originalCursorX - ImGui::CalcTextSize(ICON_FA_LINK).x - 5);
            ImGui::Text("%s ", ICON_FA_LINK);
            ImGui::SameLine();
            ImGui::SetCursorPosX(originalCursorX);
        }
        ImGui::Text("%s", t_attrbute.c_str());
        ImGui::SameLine();
        ImGui::InputText(FormatString("##%s", t_attrbute.c_str()).c_str(), &string, t_isAttributeExposed ? ImGuiInputTextFlags_ReadOnly : 0);
        t_value = string;
    }
};