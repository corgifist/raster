#include "font/IconsFontAwesome5.h"
#include "line2d_attribute.h"
#include "common/dispatchers.h"

namespace Raster {
    Line2DAttribute::Line2DAttribute() {
        AttributeBase::Initialize();

        this->interpretAsColor = true;

        keyframes.push_back(
            AttributeKeyframe(
                0,
                Line2D()
            )
        );
    }


    std::any Line2DAttribute::AbstractInterpolate(std::any t_beginValue, std::any t_endValue, float t_percentage, float t_frame, Composition* composition) {
        auto a = std::any_cast<Line2D>(t_beginValue);
        auto b = std::any_cast<Line2D>(t_endValue);
        float t = t_percentage;

        Line2D mixedLine;
        mixedLine.begin = glm::mix(a.begin, b.begin, t);
        mixedLine.end = glm::mix(a.end, b.end, t);

        return mixedLine;
    }

    void Line2DAttribute::RenderKeyframes() {
        for (auto& keyframe : keyframes) {
            RenderKeyframe(keyframe);
        }
    }


    std::any Line2DAttribute::AbstractRenderLegend(Composition* t_composition, std::any t_originalValue, bool& isItemEdited) {
        auto line = std::any_cast<Line2D>(t_originalValue);

        auto buttonText = FormatString("%s P0: (%0.2f; %0.2f); P1: (%0.2f, %0.2f)", ICON_FA_LINES_LEANING, line.begin.x, line.begin.y, line.end.x, line.end.y);
        if (ImGui::Button(buttonText.c_str(), ImVec2(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x, 0))) {
            ImGui::OpenPopup("##editLine");
        }

        if (ImGui::BeginPopup("##editLine")) {
            ImGui::SeparatorText(buttonText.c_str());
            ImGui::AlignTextToFramePadding();
            ImGui::Text(ICON_FA_UP_DOWN_LEFT_RIGHT " P0 ");
            ImGui::SameLine();
            ImGui::DragFloat2("##p0Drag", glm::value_ptr(line.begin), 0.01f, 0.0f, 0.0f, "%0.2f");
            isItemEdited = isItemEdited || ImGui::IsItemEdited();

            ImGui::AlignTextToFramePadding();
            ImGui::Text(ICON_FA_UP_DOWN_LEFT_RIGHT " P1 ");
            ImGui::SameLine();
            ImGui::DragFloat2("##p1Drag", glm::value_ptr(line.end), 0.01f, 0.0f, 0.0f, "%0.2f");
            isItemEdited = isItemEdited || ImGui::IsItemEdited();
            ImGui::EndPopup();
        }

        return line;
    }

    void Line2DAttribute::AbstractRenderDetails() {
        auto& project = Workspace::s_project.value();
        auto parentComposition = Workspace::GetCompositionByAttributeID(id).value();
        ImGui::PushID(id);
            auto currentValue = Get(project.currentFrame - parentComposition->beginFrame, parentComposition);
            Dispatchers::DispatchString(currentValue);
        ImGui::PopID();
    }
    Json Line2DAttribute::SerializeKeyframeValue(std::any t_value) {
        return std::any_cast<Line2D>(t_value).Serialize();
    }  

    std::any Line2DAttribute::LoadKeyframeValue(Json t_value) {
        return Line2D(t_value);
    }

}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractAttribute SpawnAttribute() {
        return (Raster::AbstractAttribute) std::make_shared<Raster::Line2DAttribute>();
    }

    RASTER_DL_EXPORT Raster::AttributeDescription GetDescription() {
        return Raster::AttributeDescription{
            .packageName = RASTER_PACKAGED "line2d_attribute",
            .prettyName = ICON_FA_LINES_LEANING " Line2D"
        };
    }
}