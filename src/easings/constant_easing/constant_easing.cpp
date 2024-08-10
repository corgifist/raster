#include "constant_easing.h"
#include "font/font.h"
#include "../../ImGui/imgui.h"
#include "../../ImGui/imgui_bezier.h"
#include "common/localization.h"
#include "../../ImGui/imgui_stripes.h"

namespace Raster {
    ConstantEasing::ConstantEasing() {
        EasingBase::Initialize();

        this->m_constant = 0.0f;
    }

    float ConstantEasing::Get(float t_percentage) {
        return m_constant;
    }

    void ConstantEasing::AbstractLoad(Json t_data) {
        this->m_constant = t_data;
    }

    Json ConstantEasing::AbstractSerialize() {
        return m_constant;
    }

    void ConstantEasing::AbstractRenderDetails() {
        ImVec2 bezierEditorSize(230, 230);

        ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - bezierEditorSize.x / 2.0f);
        ImGui::BeginChild("##bezierEditor", bezierEditorSize);
            ImGui::Stripes(ImVec4(0.05f, 0.05f, 0.05f, 1), ImVec4(0.1f, 0.1f, 0.1f, 1), 40, 14, bezierEditorSize);
            ImGui::SetCursorPos(ImGui::GetWindowSize() / 2.0f - bezierEditorSize / 2.0f);
            static float placeholder[] = {0, 0, 1, 1};
            ImGui::Bezier("Constant Curve", placeholder, bezierEditorSize.x, false, std::vector<float>{m_constant, m_constant}, false);
        ImGui::EndChild();    

        static float pointsChildWidth = 150;
        ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - pointsChildWidth / 2.0f);
        ImGui::BeginChild("##dragsChild", ImVec2(bezierEditorSize.x, 0), ImGuiChildFlags_AutoResizeY);
            ImGui::Text("%s Constant", ICON_FA_BEZIER_CURVE);
            ImGui::SameLine();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::SliderFloat("##constantSlider", &m_constant, 0, 1);
            ImGui::PopItemWidth();
            pointsChildWidth = ImGui::GetWindowSize().x;
        ImGui::EndChild();

    }
};

extern "C" {
    Raster::AbstractEasing SpawnEasing() {
        return (Raster::AbstractEasing) std::make_shared<Raster::ConstantEasing>();
    }

    Raster::EasingDescription GetDescription() {
        return Raster::EasingDescription{
            .prettyName = "Constant Easing",
            .packageName = RASTER_PACKAGED "constant_easing"
        };
    }
}