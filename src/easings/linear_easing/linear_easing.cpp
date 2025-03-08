#include "linear_easing.h"
#include "font/IconsFontAwesome5.h"
#include "font/font.h"
#include "../../ImGui/imgui.h"
#include "../../ImGui/imgui_bezier.h"
#include "common/localization.h"
#include "../../ImGui/imgui_stripes.h"
#include <cmath>

namespace Raster {
    LinearEasing::LinearEasing() {
        EasingBase::Initialize();

        this->m_slope = 1.0f;
        this->m_flip = 0.0f;
        this->m_shift = 0.0f;
    }

    float LinearEasing::Get(float x) {
        return std::clamp((float) std::abs(m_shift - ((x - m_flip) / m_slope)), 0.0f, 1.0f);
    }

    void LinearEasing::AbstractLoad(Json t_data) {
        this->m_slope = t_data["Slope"];
        this->m_flip = t_data["Flip"];
        this->m_shift = t_data["Shift"];
    }

    Json LinearEasing::AbstractSerialize() {
        return {
            {"Slope", m_slope},
            {"Flip", m_flip},
            {"Shift", m_shift}
        };
    }

    void LinearEasing::AbstractRenderDetails() {
        ImVec2 bezierEditorSize(230, 230);

        ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - bezierEditorSize.x / 2.0f);
        ImGui::BeginChild("##bezierEditor", bezierEditorSize);
            ImGui::Stripes(ImVec4(0.05f, 0.05f, 0.05f, 1), ImVec4(0.1f, 0.1f, 0.1f, 1), 40, 14, bezierEditorSize);
            ImGui::SetCursorPos(ImGui::GetWindowSize() / 2.0f - bezierEditorSize / 2.0f);
            static float placeholder[] = {0, 0, 1, 1};
            std::vector<float> ease;
            for (int i = 0; i < 256; i++) {
                ease.push_back(Get((float) i / 256.0f));
            }
            ImGui::Bezier("Linear Curve", placeholder, bezierEditorSize.x, false, ease, false);
        ImGui::EndChild();    

        static float pointsChildWidth = 150;
        ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - pointsChildWidth / 2.0f);
        ImGui::BeginChild("##dragsChild", ImVec2(bezierEditorSize.x, 0), ImGuiChildFlags_AutoResizeY);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s Slope", ICON_FA_BEZIER_CURVE);
            ImGui::SameLine();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::SliderFloat("##slopeSlider", &m_slope, 0.0, 5);
            ImGui::PopItemWidth();

            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s Flip", ICON_FA_BEZIER_CURVE);
            ImGui::SameLine();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::SliderFloat("##flipSlider", &m_flip, 0.0f, 1.0f);
            ImGui::PopItemWidth();

            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s Shift", ICON_FA_BEZIER_CURVE);
            ImGui::SameLine();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::DragFloat("##shiftSlider", &m_shift, 0.01f);
            ImGui::PopItemWidth();

            pointsChildWidth = ImGui::GetWindowSize().x;
        ImGui::EndChild();

    }
};

extern "C" {
    Raster::AbstractEasing SpawnEasing() {
        return (Raster::AbstractEasing) std::make_shared<Raster::LinearEasing>();
    }

    Raster::EasingDescription GetDescription() {
        return Raster::EasingDescription{
            .prettyName = "Linear Easing",
            .packageName = RASTER_PACKAGED "linear_easing"
        };
    }
}