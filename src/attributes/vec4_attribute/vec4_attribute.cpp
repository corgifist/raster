#include "vec4_attribute.h"

namespace Raster {
    Vec4Attribute::Vec4Attribute() {
        AttributeBase::Initialize();

        this->interpretAsColor = false;

        keyframes.push_back(
            AttributeKeyframe(
                0,
                glm::vec4(0, 0, 0, 0)
            )
        );
    }


    std::any Vec4Attribute::AbstractInterpolate(std::any t_beginValue, std::any t_endValue, float t_percentage, float t_frame, Composition* composition) {
        glm::vec4 a = std::any_cast<glm::vec4>(t_beginValue);
        glm::vec4 b = std::any_cast<glm::vec4>(t_endValue);
        float t = t_percentage;

        return glm::mix(a, b, t);
    }

    void Vec4Attribute::RenderKeyframes() {
        for (auto& keyframe : keyframes) {
            RenderKeyframe(keyframe);
        }
    }


    std::any Vec4Attribute::AbstractRenderLegend(Composition* t_composition, std::any t_originalValue, bool& isItemEdited) {
        auto vector = std::any_cast<glm::vec4>(t_originalValue);
        float vectorPtr[4] = {
            vector.x,
            vector.y,
            vector.z,
            vector.w
        };
        if (!interpretAsColor) {
            ImGui::DragFloat4("##dragVector4", vectorPtr);
        } else {
            ImGui::ColorEdit4("##colorVec4", vectorPtr);
        }
        isItemEdited = ImGui::IsItemEdited();
        return glm::vec4(
            vectorPtr[0], vectorPtr[1], vectorPtr[2], vectorPtr[3]
        );
    }

    void Vec4Attribute::AbstractRenderDetails() {
        auto& project = Workspace::s_project.value();
        auto parentComposition = Workspace::GetCompositionByAttributeID(id).value();
        ImGui::PushID(id);
            auto currentValue = Get(project.GetCorrectCurrentTime() - parentComposition->beginFrame, parentComposition);
            auto vector = std::any_cast<glm::vec4>(currentValue);
            if (!interpretAsColor) {
                ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1, 0, 0, 1));
                ImGui::PlotVar(FormatString("%s %s %s", ICON_FA_STOPWATCH, name.c_str(), "(x)").c_str(), vector.x);

                ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0, 1, 0, 1));
                ImGui::PlotVar(FormatString("%s %s %s", ICON_FA_STOPWATCH, name.c_str(), "(y)").c_str(), vector.y);

                ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(0, 0, 1, 1));
                ImGui::PlotVar(FormatString("%s %s %s", ICON_FA_STOPWATCH, name.c_str(), "(z)").c_str(), vector.z);

                ImGui::PushStyleColor(ImGuiCol_PlotLines, ImVec4(1, 1, 0, 1));
                ImGui::PlotVar(FormatString("%s %s %s", ICON_FA_STOPWATCH, name.c_str(), "(w)").c_str(), vector.w);
                ImGui::PopStyleColor(4);
            } else {
                float vectorPtr[4] = {
                    vector.x,
                    vector.y,
                    vector.z,
                    vector.w
                };
                ImGui::PushItemWidth(200);
                    ImGui::ColorPicker4("##colorPreview", vectorPtr, ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);
                ImGui::PopItemWidth();
            }
        ImGui::PopID();
    }

    void Vec4Attribute::AbstractRenderPopup() {
        if (ImGui::BeginMenu(ICON_FA_CIRCLE_QUESTION " Interpret as")) {
            ImGui::SeparatorText(ICON_FA_CIRCLE_QUESTION " Interpret as");
            if (ImGui::MenuItem(FormatString("%s %s Vector", !interpretAsColor ? ICON_FA_CHECK : "", ICON_FA_EXPAND).c_str())) {
                interpretAsColor = false;
            }
            if (ImGui::MenuItem(FormatString("%s %s Color", interpretAsColor ? ICON_FA_CHECK : "", ICON_FA_DROPLET).c_str())) {
                interpretAsColor = true;
            }
            ImGui::EndMenu();
        }

        if (interpretAsColor && ImGui::MenuItem(ICON_FA_DROPLET " Set Alpha Channel to 255")) {
            for (auto& keyframe : keyframes) {
                auto vector = std::any_cast<glm::vec4>(keyframe.value);
                vector.a = 1.0f;
                keyframe.value = vector;
            }
        }
        ImGui::Separator();
    }

    Json Vec4Attribute::SerializeKeyframeValue(std::any t_value) {
        auto vec = std::any_cast<glm::vec4>(t_value);
        return {
            vec[0], vec[1], vec[2], vec[3]
        };
    }  

    std::any Vec4Attribute::LoadKeyframeValue(Json t_value) {
        return glm::vec4(t_value[0], t_value[1], t_value[2], t_value[3]);
    }

}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractAttribute SpawnAttribute() {
        return (Raster::AbstractAttribute) std::make_shared<Raster::Vec4Attribute>();
    }

    RASTER_DL_EXPORT Raster::AttributeDescription GetDescription() {
        return Raster::AttributeDescription{
            .packageName = RASTER_PACKAGED "vec4_attribute",
            .prettyName = ICON_FA_EXPAND " Vector4"
        };
    }
}