#include "random_easing.h"
#include "font/IconsFontAwesome5.h"
#include "font/font.h"
#include "../../ImGui/imgui.h"
#include "../../ImGui/imgui_bezier.h"
#include "common/localization.h"
#include "../../ImGui/imgui_stripes.h"
#include <cmath>

namespace Raster {
    RandomEasing::RandomEasing() {
        EasingBase::Initialize();

        this->m_seed1 = 1;
        this->m_seed2 = 2;
        this->m_seed3 = 3;
        this->m_amplitude = 1;
    }

    float noise1(float seed1,float seed2){
        using namespace glm;
        return(
        fract(seed1+12.34567*
        fract(100.*(abs(seed1*0.91)+seed2+94.68)*
        fract((abs(seed2*0.41)+45.46)*
        fract((abs(seed2)+757.21)*
        fract(seed1*0.0171))))))
        * 1.0038 - 0.00185;
    }
    
    //2 seeds
    float noise2(float seed1,float seed2){
        using namespace glm;
        float buff1 = abs(seed1+100.94) + 1000.;
        float buff2 = abs(seed2+100.73) + 1000.;
        buff1 = (buff1*fract(buff2*fract(buff1*fract(buff2*0.63))));
        buff2 = (buff2*fract(buff2*fract(buff1+buff2*fract(seed1*0.79))));
        buff1 = noise1(buff1, buff2);
        return(buff1 * 1.0038 - 0.00185);
    }
    
    //3 seeds
    float noise2(float seed1,float seed2,float seed3){
        using namespace glm;
        float buff1 = abs(seed1+100.81) + 1000.3;
        float buff2 = abs(seed2+100.45) + 1000.2;
        float buff3 = abs(noise1(seed1, seed2)+seed3) + 1000.1;
        buff1 = (buff3*fract(buff2*fract(buff1*fract(buff2*0.146))));
        buff2 = (buff2*fract(buff2*fract(buff1+buff2*fract(buff3*0.52))));
        buff1 = noise1(buff1, buff2);
        return(buff1);
    }
    
    //3 seeds hard
    float noise3(float seed1,float seed2,float seed3){
        using namespace glm;
        float buff1 = abs(seed1+100.813) + 1000.314;
        float buff2 = abs(seed2+100.453) + 1000.213;
        float buff3 = abs(noise1(buff2, buff1)+seed3) + 1000.17;
        buff1 = (buff3*fract(buff2*fract(buff1*fract(buff2*0.14619))));
        buff2 = (buff2*fract(buff2*fract(buff1+buff2*fract(buff3*0.5215))));
        buff1 = noise2(noise1(seed2,buff1), noise1(seed1,buff2), noise1(seed3,buff3));
        return(buff1);
    }

    float RandomEasing::Get(float x) {
        float random = noise3(m_seed1 + x, m_seed2 - x, m_seed3 * x) * m_amplitude;
#define E0 0.01f
        float fadePercentage = (1.0f / E0) * std::max(glm::smoothstep(0.0f, 1.0f, x) - 1.0f + E0, 0.0f);
        return glm::mix(random, 1.0f, fadePercentage);
    }

    void RandomEasing::AbstractLoad(Json t_data) {
        this->m_seed1 = t_data["Seed1"];
        this->m_seed2 = t_data["Seed2"];
        this->m_seed3 = t_data["Seed3"];
        if (t_data.contains("Amplitude"))  m_amplitude = t_data["Amplitude"];
    }

    Json RandomEasing::AbstractSerialize() {
        return {
            {"Seed1", m_seed1},
            {"Seed2", m_seed2},
            {"Seed3", m_seed3},
            {"Amplitude", m_amplitude}
        };
    }

    void RandomEasing::AbstractRenderDetails() {
        ImVec2 bezierEditorSize(230, 230);

        ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - bezierEditorSize.x / 2.0f);
        ImGui::BeginChild("##bezierEditor", bezierEditorSize);
            ImGui::Stripes(ImVec4(0.05f, 0.05f, 0.05f, 1), ImVec4(0.1f, 0.1f, 0.1f, 1), 40, 14, bezierEditorSize);
            ImGui::SetCursorPos(ImGui::GetWindowSize() / 2.0f - bezierEditorSize / 2.0f);
            static float placeholder[] = {0, 0, 1, 1};
            std::vector<float> ease;
            for (int i = 0; i < 256; i++) {
                ease.push_back(glm::clamp(Get((float) i / 256.0f), 0.0f, 1.0f));
            }
            ImGui::Bezier("Random Curve", placeholder, bezierEditorSize.x, false, ease, false);
        ImGui::EndChild();    

        static float pointsChildWidth = 150;
        ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - pointsChildWidth / 2.0f);
        ImGui::BeginChild("##dragsChild", ImVec2(bezierEditorSize.x, 0), ImGuiChildFlags_AutoResizeY);
            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s Seed1", ICON_FA_BEZIER_CURVE);
            ImGui::SameLine();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::DragFloat("##seed1", &m_seed1);
            ImGui::PopItemWidth();

            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s Seed2", ICON_FA_BEZIER_CURVE);
            ImGui::SameLine();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::DragFloat("##seed2", &m_seed2);
            ImGui::PopItemWidth();

            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s Seed3", ICON_FA_BEZIER_CURVE);
            ImGui::SameLine();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::DragFloat("##seed3", &m_seed3);
            ImGui::PopItemWidth();

            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s Amplitude", ICON_FA_BEZIER_CURVE);
            ImGui::SameLine();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::SliderFloat("##amplitudeSlider", &m_amplitude, 0, 2);
            ImGui::PopItemWidth();
            pointsChildWidth = ImGui::GetWindowSize().x;
        ImGui::EndChild();

    }
};

extern "C" {
    Raster::AbstractEasing SpawnEasing() {
        return (Raster::AbstractEasing) std::make_shared<Raster::RandomEasing>();
    }

    Raster::EasingDescription GetDescription() {
        return Raster::EasingDescription{
            .prettyName = "Random Easing",
            .packageName = RASTER_PACKAGED "random_easing"
        };
    }
}