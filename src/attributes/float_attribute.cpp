#include "float_attribute.h"

namespace Raster {
    FloatAttribute::FloatAttribute() {
        AttributeBase::Initialize();

        keyframes.push_back(
            AttributeKeyframe(
                0,
                0.0f
            )
        );
    }

    std::any FloatAttribute::AbstractInterpolate(std::any t_beginValue, std::any t_endValue, float t_percentage, float t_frame, Composition* composition) {
        float a = std::any_cast<float>(t_beginValue);
        float b = std::any_cast<float>(t_endValue);
        float t = t_percentage;

        return a + t * (b - a);
    }

    void FloatAttribute::RenderKeyframes() {
        for (auto& keyframe : keyframes) {
            RenderKeyframe(keyframe);
        }
    }

    void FloatAttribute::Load(Json t_data) {
        keyframes.clear();
        for (auto& keyframe : t_data["Keyframes"]) {
            keyframes.push_back(
                AttributeKeyframe(
                    keyframe["ID"],
                    keyframe["Timestamp"],
                    keyframe["Value"].get<float>()
                )
            );
        }
    }

    void FloatAttribute::RenderLegend(Composition* t_composition) {
        this->composition = t_composition;
        auto& project = Workspace::s_project.value();
        float currentFrame = project.currentFrame - t_composition->beginFrame;
        auto currentValue = Get(currentFrame, t_composition);
        float fCurrentValue = std::any_cast<float>(currentValue);
        ImGui::PushID(id);
            bool buttonPressed = ImGui::Button(KeyframeExists(currentFrame) ? ICON_FA_TRASH_CAN : ICON_FA_PLUS);
            bool shouldAddKeyframe = buttonPressed;
            ImGui::SameLine();
            ImGui::Text("%s %s", ICON_FA_LINK, name.c_str()); 
            ImGui::SameLine();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x);
                ImGui::DragFloat("##floatDrag", &fCurrentValue);
                shouldAddKeyframe = shouldAddKeyframe || ImGui::IsItemEdited();
                currentValue = fCurrentValue;
            ImGui::PopItemWidth();
        ImGui::PopID();

        if (shouldAddKeyframe && !KeyframeExists(currentFrame)) {
            keyframes.push_back(
                AttributeKeyframe(
                    currentFrame,
                    currentValue
                )
            );
        } else if (shouldAddKeyframe && !buttonPressed) {
            auto keyframeCandidate = GetKeyframeByTimestamp(currentFrame);
            if (keyframeCandidate.has_value()) {
                auto* keyframe = keyframeCandidate.value();
                keyframe->value = currentValue;
            }
        } else if (shouldAddKeyframe && buttonPressed) {
            auto indexCandidate = GetKeyframeIndexByTimestamp(currentFrame);
            if (indexCandidate.has_value()) {
                keyframes.erase(keyframes.begin() + indexCandidate.value());
            }
        }
    }

    Json FloatAttribute::AbstractSerialize() {
        Json result = {
            {"Keyframes", {}}
        };
        for (auto& keyframe : keyframes) {
            result["Keyframes"].push_back({
                {"Timestamp", keyframe.timestamp},
                {"Value", std::any_cast<float>(keyframe.value)},
                {"ID", keyframe.id}
            });
        }
        return result;
    }
}