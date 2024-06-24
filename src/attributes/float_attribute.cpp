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

    std::any FloatAttribute::Get(float t_frame, Composition* t_composition) {
        this->composition = composition;
        SortKeyframes();
        auto& project = Workspace::s_project.value();

        int targetKeyframeIndex = -1;
        int keyframesLength = keyframes.size();
        float renderViewTime = project.currentFrame - composition->beginFrame;

        for (int i = 0; i < keyframesLength; i++) {
            float keyframeTimestamp = keyframes.at(i).timestamp;
            if (renderViewTime <= keyframeTimestamp) {
                targetKeyframeIndex = i;
                break;
            }
        }

        if (targetKeyframeIndex == -1) {
            return keyframes.back().value;
        }

        if (targetKeyframeIndex == 0) {
            return keyframes.front().value;
        }

        float keyframeTimestamp = keyframes.at(targetKeyframeIndex).timestamp;
        float interpolationPercentage = 0;
        if (targetKeyframeIndex == 1) {
            interpolationPercentage = renderViewTime / keyframeTimestamp;
        } else {
            float previousFrame = keyframes.at(targetKeyframeIndex - 1).timestamp;
            interpolationPercentage = (renderViewTime - previousFrame) / (keyframeTimestamp - previousFrame);
        }

        auto& beginKeyframeValue = keyframes.at(targetKeyframeIndex - 1).value;
        auto& endkeyframeValue = keyframes.at(targetKeyframeIndex).value;

        float beginValue = std::any_cast<float>(beginKeyframeValue);
        float endValue = std::any_cast<float>(endkeyframeValue);

        return beginValue + interpolationPercentage * (endValue - beginValue);
    }

    void FloatAttribute::RenderKeyframes() {
        for (auto& keyframe : keyframes) {

            RenderKeyframe(keyframe);
        }
    }

    void FloatAttribute::Load(Json t_data) {

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
        return {};
    }
}