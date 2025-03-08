#include "elastic_easing.h"
#include "font/IconsFontAwesome5.h"
#include "font/font.h"
#include "../../ImGui/imgui.h"
#include "../../ImGui/imgui_bezier.h"
#include "common/localization.h"
#include "../../ImGui/imgui_stripes.h"
#include <cmath>

namespace Raster {
    ElasticEasing::ElasticEasing() {
        EasingBase::Initialize();

        this->m_amplitude = 1.0f;
        this->m_speed = 5.0f;
    }

    float ElasticEasing::BaseGet(float x) {
        return std::sin(x * m_speed * M_PI) * 0.5 * m_amplitude * (1 - x);
    }

    float ElasticEasing::Get(float x) {
        return (float) BaseGet(x) + x;
    }

    void ElasticEasing::AbstractLoad(Json t_data) {
        this->m_amplitude = t_data["Amplitude"];
        this->m_speed = t_data["Speed"];
    }

    Json ElasticEasing::AbstractSerialize() {
        return {
            {"Amplitude", m_amplitude},
            {"Speed", m_speed}
        };
    }

    void ElasticEasing::AbstractRenderDetails() {
        ImVec2 bezierEditorSize(230, 230);

        ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - bezierEditorSize.x / 2.0f);
        ImGui::BeginChild("##bezierEditor", bezierEditorSize);
            ImGui::Stripes(ImVec4(0.05f, 0.05f, 0.05f, 1), ImVec4(0.1f, 0.1f, 0.1f, 1), 40, 14, bezierEditorSize);
            ImGui::SetCursorPos(ImGui::GetWindowSize() / 2.0f - bezierEditorSize / 2.0f);
            static float placeholder[] = {0, 0, 1, 1};
            std::vector<float> ease;
            for (int i = 0; i < 256; i++) {
                ease.push_back(glm::clamp(BaseGet((float) i / 256.0f) + 0.5f, 0.0f, 1.0f));
            }
            ImGui::Bezier("Elastic Curve", placeholder, bezierEditorSize.x, false, ease, false);
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
        return (Raster::AbstractEasing) std::make_shared<Raster::ElasticEasing>();
    }

    Raster::EasingDescription GetDescription() {
        return Raster::EasingDescription{
            .prettyName = "Elastic Easing",
            .packageName = RASTER_PACKAGED "elastic_easing"
        };
    }
}