#include "vec3_attribute.h"

namespace Raster {
    Vec3Attribute::Vec3Attribute() {
        AttributeBase::Initialize();

        keyframes.push_back(
            AttributeKeyframe(
                0,
                glm::vec3(0, 0, 0)
            )
        );
    }


    std::any Vec3Attribute::AbstractInterpolate(std::any t_beginValue, std::any t_endValue, float t_percentage, float t_frame, Composition* composition) {
        glm::vec3 a = std::any_cast<glm::vec3>(t_beginValue);
        glm::vec3 b = std::any_cast<glm::vec3>(t_endValue);
        float t = t_percentage;

        return glm::mix(a, b, t);
    }

    void Vec3Attribute::RenderKeyframes() {
        for (auto& keyframe : keyframes) {
            RenderKeyframe(keyframe);
        }
    }


    std::any Vec3Attribute::AbstractRenderLegend(Composition* t_composition, std::any t_originalValue, bool& isItemEdited) {
        auto vector = std::any_cast<glm::vec3>(t_originalValue);
        ImGui::DragFloat3("##dragFloat3", glm::value_ptr(vector));
        isItemEdited = ImGui::IsItemEdited();
        return vector;
    }

    void Vec3Attribute::AbstractRenderDetails() {
        auto& project = Workspace::s_project.value();
        auto parentComposition = Workspace::GetCompositionByAttributeID(id).value();
        ImGui::PushID(id);
            auto currentValue = Get(project.currentFrame - parentComposition->beginFrame, parentComposition);
            auto vector = std::any_cast<glm::vec3>(currentValue);
                ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1, 0, 0, 1));
                ImGui::PlotVar(FormatString("%s %s %s", ICON_FA_STOPWATCH, name.c_str(), "(x)").c_str(), vector.x);

                ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0, 1, 0, 1));
                ImGui::PlotVar(FormatString("%s %s %s", ICON_FA_STOPWATCH, name.c_str(), "(y)").c_str(), vector.y);

                ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0, 0, 1, 1));
                ImGui::PlotVar(FormatString("%s %s %s", ICON_FA_STOPWATCH, name.c_str(), "(z)").c_str(), vector.z);

                ImGui::PopStyleColor(3);
        ImGui::PopID();
    }


    Json Vec3Attribute::SerializeKeyframeValue(std::any t_value) {
        auto vec = std::any_cast<glm::vec3>(t_value);
        return {
            vec[0], vec[1], vec[2]
        };
    }  

    std::any Vec3Attribute::LoadKeyframeValue(Json t_value) {
        return glm::vec3(t_value[0], t_value[1], t_value[2]);
    }

}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractAttribute SpawnAttribute() {
        return (Raster::AbstractAttribute) std::make_shared<Raster::Vec3Attribute>();
    }

    RASTER_DL_EXPORT Raster::AttributeDescription GetDescription() {
        return Raster::AttributeDescription{
            .packageName = RASTER_PACKAGED "vec3_attribute",
            .prettyName = ICON_FA_UP_RIGHT_AND_DOWN_LEFT_FROM_CENTER " Vector3"
        };
    }
}