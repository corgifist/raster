#include "bounce_easing.h"
#include "font/IconsFontAwesome5.h"
#include "font/font.h"
#include "../../ImGui/imgui.h"
#include "../../ImGui/imgui_bezier.h"
#include "common/localization.h"
#include "../../ImGui/imgui_stripes.h"
#include <cmath>

namespace Raster {
    BounceEasing::BounceEasing() {
        EasingBase::Initialize();

        this->m_amplitude = 1.0f;
        this->m_speed = 1.0f;
    }

    float BounceEasing::Get(float x) {
        float result = std::clamp(1 - std::abs((float) (pow(M_E, -x/m_amplitude) * cos(x * m_speed))), 0.0f, 1.0f);
        return result;
    }

    void BounceEasing::AbstractLoad(Json t_data) {
        this->m_amplitude = t_data["Amplitude"];
        this->m_speed = t_data["Speed"];
    }

    Json BounceEasing::AbstractSerialize() {
        return {
            {"Amplitude", m_amplitude},
            {"Speed", m_speed}
        };
    }

    void BounceEasing::AbstractRenderDetails() {
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
            ImGui::Bezier("Constant Curve", placeholder, bezierEditorSize.x, false, ease, false);
        ImGui::EndChild();    

        static float pointsChildWidth = 150;
        ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - pointsChildWidth / 2.0f);
        ImGui::BeginChild("##dragsChild", ImVec2(bezierEditorSize.x, 0), ImGuiChildFlags_AutoResizeY);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s Amplitude", ICON_FA_BEZIER_CURVE);
            ImGui::SameLine();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::SliderFloat("##amplitudeSlider", &m_amplitude, 0.01, 2);
            ImGui::PopItemWidth();

            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s Speed", ICON_FA_BEZIER_CURVE);
            ImGui::SameLine();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::SliderFloat("##speedSlider", &m_speed, 0.01f, 50.0f);
            ImGui::PopItemWidth();
            pointsChildWidth = ImGui::GetWindowSize().x;
        ImGui::EndChild();

    }
};

extern "C" {
    Raster::AbstractEasing SpawnEasing() {
        return (Raster::AbstractEasing) std::make_shared<Raster::BounceEasing>();
    }

    Raster::EasingDescription GetDescription() {
        return Raster::EasingDescription{
            .prettyName = "Bounce Easing",
            .packageName = RASTER_PACKAGED "bounce_easing"
        };
    }
}