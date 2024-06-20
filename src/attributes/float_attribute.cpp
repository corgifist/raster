#include "float_attribute.h"

namespace Raster {
    FloatAttribute::FloatAttribute() {
        AttributeBase::Initialize();
    }

    std::any FloatAttribute::Get(float t_frame, Composition* composition) {
        return 1.0f;
    }

    void FloatAttribute::RenderKeyframes() {

    }

    void FloatAttribute::Load(Json t_data) {

    }

    void FloatAttribute::RenderLegend(Composition* t_composition) {
        auto& project = Workspace::s_project.value();
        float currentFrame = project.currentFrame - t_composition->beginFrame;
        auto currentValue = Get(project.currentFrame - t_composition->beginFrame, t_composition);
        float fCurrentValue = std::any_cast<float>(currentValue);
        ImGui::PushID(id);
            bool shouldAddKeyframe = ImGui::Button(ICON_FA_PLUS);
            ImGui::SameLine();
            ImGui::Text("%s %s", ICON_FA_LINK, name.c_str()); 
            ImGui::SameLine();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::DragFloat("##floatDrag", &fCurrentValue);
            ImGui::PopItemWidth();
        ImGui::PopID();
    }

    Json FloatAttribute::AbstractSerialize() {
        return {};
    }
}