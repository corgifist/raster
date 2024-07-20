#include "transform2d_attribute.h"

namespace Raster {
    Transform2DAttribute::Transform2DAttribute() {
        AttributeBase::Initialize();

        auto& project = Workspace::s_project.value();

        Transform2D transform;
        transform.size = {1, 1};


        keyframes.push_back(
            AttributeKeyframe(
                0,
                transform
            )
        );
    }

    std::any Transform2DAttribute::AbstractInterpolate(std::any t_beginValue, std::any t_endValue, float t_percentage, float t_frame, Composition* composition) {
        Transform2D a = std::any_cast<Transform2D>(t_beginValue);
        Transform2D b = std::any_cast<Transform2D>(t_endValue);
        float t = t_percentage;

        Transform2D result;
        result.position = glm::mix(a.position, b.position, t);
        result.size = glm::mix(a.size, b.size, t);
        result.anchor = glm::mix(a.anchor, b.anchor, t);
        result.angle = glm::mix(a.angle, b.angle, t);

        return result;
    }

    void Transform2DAttribute::RenderKeyframes() {
        for (auto& keyframe : keyframes) {
            RenderKeyframe(keyframe);
        }
    }

    void Transform2DAttribute::Load(Json t_data) {
        keyframes.clear();
        for (auto& keyframe : t_data["Keyframes"]) {
            keyframes.push_back(
                AttributeKeyframe(
                    keyframe["ID"],
                    keyframe["Timestamp"],
                    Transform2D(keyframe["Value"])
                )
            );
        }
    }

    std::any Transform2DAttribute::AbstractRenderLegend(Composition* t_composition, std::any t_originalValue, bool& isItemEdited) {
        auto& project = Workspace::s_project.value();
        Transform2D transform = std::any_cast<Transform2D>(t_originalValue);
        if (ImGui::Button(ICON_FA_UP_DOWN_LEFT_RIGHT " Edit Value", ImVec2(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x, 0))) {
            ImGui::OpenPopup(FormatString("##%iattribute", id).c_str());
        }
        if (ImGui::BeginPopup(FormatString("##%iattribute", id).c_str())) {
            if (!project.customData.contains("Transform2DAttributeData")) {
                project.customData["Transform2DAttributeData"] = {};
            }
            auto& customData = project.customData["Transform2DAttributeData"];
            auto stringID = std::to_string(id);
            if (!customData.contains(stringID)) {
                customData[stringID] = false;
            }
            bool linkedSize = customData[stringID];

            ImGui::SeparatorText(FormatString("%s Edit Value: %s", ICON_FA_UP_DOWN_LEFT_RIGHT, name.c_str()).c_str());
            
            ImGui::Text("%s Position", ICON_FA_UP_DOWN_LEFT_RIGHT);
            ImGui::SameLine();
            float cursorX = ImGui::GetCursorPosX();
            ImGui::DragFloat2("##dragPosition", glm::value_ptr(transform.position), 0.05f);
            isItemEdited = isItemEdited || ImGui::IsItemEdited();
            

            ImGui::Text("%s Size", ICON_FA_SCALE_BALANCED);
            ImGui::SameLine();
            ImGui::SetCursorPosX(cursorX);
            if (ImGui::Button(linkedSize ? ICON_FA_LINK : ICON_FA_LINK_SLASH)) {
                linkedSize = !linkedSize;
            }
            ImGui::SetItemTooltip("%s %s", ICON_FA_LINK, "Link Dimensions");
            ImGui::SameLine(0, 2.0f);
            glm::vec2 reservedSize = transform.size;
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::DragFloat2("##dragSize", glm::value_ptr(transform.size), 0.05f);
            ImGui::PopItemWidth();
            isItemEdited = isItemEdited || ImGui::IsItemEdited();
            if (linkedSize) {
                if (reservedSize.x != transform.size.x) {
                    transform.size.y = transform.size.x;
                } else if (reservedSize.y != transform.size.y) {
                    transform.size.x = transform.size.y;
                }
            }
            customData[stringID] = linkedSize;

            ImGui::Text("%s Anchor", ICON_FA_ANCHOR);
            ImGui::SameLine();
            ImGui::SetCursorPosX(cursorX);
            ImGui::DragFloat2("##dragAnchor", glm::value_ptr(transform.anchor), 0.05f);
            isItemEdited = isItemEdited || ImGui::IsItemEdited();

            ImGui::Text("%s Rotation", ICON_FA_ROTATE);
            ImGui::SameLine();
            ImGui::SetCursorPosX(cursorX);
            ImGui::DragFloat("##dragAngle", &transform.angle, 0.5f);

            isItemEdited = isItemEdited || ImGui::IsItemEdited();

            ImGui::EndPopup();
        }
        return transform;
    }

    void Transform2DAttribute::AbstractRenderDetails() {
        auto& project = Workspace::s_project.value();
        auto parentComposition = Workspace::GetCompositionByAttributeID(id).value();
    }

    Json Transform2DAttribute::AbstractSerialize() {
        Json result = {
            {"Keyframes", {}}
        };
        for (auto& keyframe : keyframes) {
            result["Keyframes"].push_back({
                {"Timestamp", keyframe.timestamp},
                {"Value", std::any_cast<Transform2D>(keyframe.value).Serialize()},
                {"ID", keyframe.id}
            });
        }
        return result;
    }
}