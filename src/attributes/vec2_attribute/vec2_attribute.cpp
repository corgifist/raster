#include "vec2_attribute.h"

namespace Raster {
    Vec2Attribute::Vec2Attribute() {
        AttributeBase::Initialize();

        keyframes.push_back(
            AttributeKeyframe(
                0,
                glm::vec2(0, 0)
            )
        );
    }


    std::any Vec2Attribute::AbstractInterpolate(std::any t_beginValue, std::any t_endValue, float t_percentage, float t_frame, Composition* composition) {
        glm::vec2 a = std::any_cast<glm::vec2>(t_beginValue);
        glm::vec2 b = std::any_cast<glm::vec2>(t_endValue);
        float t = t_percentage;

        return glm::mix(a, b, t);
    }

    void Vec2Attribute::RenderKeyframes() {
        for (auto& keyframe : keyframes) {
            RenderKeyframe(keyframe);
        }
    }


    std::any Vec2Attribute::AbstractRenderLegend(Composition* t_composition, std::any t_originalValue, bool& isItemEdited) {
        auto vector = std::any_cast<glm::vec2>(t_originalValue);
        ImGui::DragFloat2("##dragFloat2", glm::value_ptr(vector));
        isItemEdited = ImGui::IsItemEdited();
        return vector;
    }

    void Vec2Attribute::AbstractRenderDetails() {
        auto& project = Workspace::s_project.value();
        auto parentComposition = Workspace::GetCompositionByAttributeID(id).value();
        ImGui::PushID(id);
            auto currentValue = Get(project.GetCorrectCurrentTime() - parentComposition->beginFrame, parentComposition);
            auto vector = std::any_cast<glm::vec2>(currentValue);
                ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1, 0, 0, 1));
                ImGui::PlotVar(FormatString("%s %s %s", ICON_FA_STOPWATCH, name.c_str(), "(x)").c_str(), vector.x);

                ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0, 1, 0, 1));
                ImGui::PlotVar(FormatString("%s %s %s", ICON_FA_STOPWATCH, name.c_str(), "(y)").c_str(), vector.y);

                ImGui::PopStyleColor(2);
        ImGui::PopID();
    }


    Json Vec2Attribute::SerializeKeyframeValue(std::any t_value) {
        auto vec = std::any_cast<glm::vec2>(t_value);
        return {
            vec[0], vec[1]
        };
    }  

    std::any Vec2Attribute::LoadKeyframeValue(Json t_value) {
        return glm::vec2(t_value[0], t_value[1]);
    }

}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractAttribute SpawnAttribute() {
        return (Raster::AbstractAttribute) std::make_shared<Raster::Vec2Attribute>();
    }

    RASTER_DL_EXPORT Raster::AttributeDescription GetDescription() {
        return Raster::AttributeDescription{
            .packageName = RASTER_PACKAGED "vec2_attribute",
            .prettyName = ICON_FA_LEFT_RIGHT " Vector2"
        };
    }
}