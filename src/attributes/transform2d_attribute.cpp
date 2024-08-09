#include "transform2d_attribute.h"
#include "common/workspace.h"

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

        this->m_parentAttributeID = -1;
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

        auto parentAttributeCandidate = Workspace::GetAttributeByAttributeID(m_parentAttributeID);
        if (parentAttributeCandidate.has_value()) {
            auto& project = Workspace::GetProject();
            auto& parentAttribute = parentAttributeCandidate.value();
            auto parentComposition = Workspace::GetCompositionByAttributeID(parentAttribute->id).value();
            auto dynamicValue = parentAttribute->Get(project.currentFrame - parentComposition->beginFrame, parentComposition);
            if (dynamicValue.type() == typeid(Transform2D)) {
                auto transform = std::any_cast<Transform2D>(dynamicValue);
                result.parentTransform = std::make_shared<Transform2D>(transform);
            }
        }

        return result;
    }

    void Transform2DAttribute::RenderKeyframes() {
        for (auto& keyframe : keyframes) {
            RenderKeyframe(keyframe);
        }
    }

    void Transform2DAttribute::Load(Json t_data) {
        keyframes.clear();
        m_linkedSize = t_data["LinkedSize"];
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
        auto parentAttributeCandidate = Workspace::GetAttributeByAttributeID(m_parentAttributeID);
        std::string buttonText = FormatString("%s %s | %s %s", ICON_FA_UP_DOWN_LEFT_RIGHT, Localization::GetString("EDIT_VALUE").c_str(), ICON_FA_SITEMAP, parentAttributeCandidate.has_value() ? parentAttributeCandidate.value()->name.c_str() : Localization::GetString("NO_PARENT").c_str());
        if (ImGui::Button(m_linkedSize ? ICON_FA_LINK : ICON_FA_LINK_SLASH)) {
            m_linkedSize = !m_linkedSize;
        }
        ImGui::SetItemTooltip("%s %s", ICON_FA_LINK, "Link Dimensions");
        ImGui::SameLine(0, 2);
        if (ImGui::Button(buttonText.c_str(), ImVec2(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x, 0))) {
            ImGui::OpenPopup(FormatString("##%iattribute", id).c_str());
        }
        ImGui::SetItemTooltip("%s %s", ICON_FA_ARROW_POINTER, Localization::GetString("RIGHT_CLICK_FOR_CONTEXT_MENU").c_str());

        std::string popupID = FormatString("##transform2DParentAttribute%i", id);

        if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
            ImGui::OpenPopup(popupID.c_str());
        }

        bool openMoreAttributesPopup = false;
        if (ImGui::BeginPopup(popupID.c_str())) {
            std::string buttonText = FormatString("%s %s", ICON_FA_UP_DOWN_LEFT_RIGHT, name.c_str());
            if (parentAttributeCandidate.has_value()) {
                buttonText += FormatString(" | %s %s", ICON_FA_SITEMAP, parentAttributeCandidate.value()->name.c_str());
            }
            ImGui::SeparatorText(buttonText.c_str());
            if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_LIST, Localization::GetString("SELECT_PARENT_ATTRIBUTE").c_str()).c_str())) {
                ImGui::SeparatorText(FormatString("%s %s", ICON_FA_LIST, Localization::GetString("SELECT_PARENT_ATTRIBUTE").c_str()).c_str());
                static std::string attributeFilter = "";
                ImGui::InputTextWithHint("##attributeFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_BY_NAME_OR_ATTRIBUTE_ID").c_str()).c_str(), &attributeFilter);
                auto parentComposition = Workspace::GetCompositionByAttributeID(id).value();
                bool oneCandidateWasDisplayed = false;
                bool mustShowByID = false;
                for (auto& attribute : parentComposition->attributes) {
                    if (std::to_string(attribute->id) == ReplaceString(attributeFilter, " ", "")) {
                        mustShowByID = true;
                        break;
                    }
                }
                for (auto& attribute : parentComposition->attributes) {
                    if (!mustShowByID) {
                        if (!attributeFilter.empty() && LowerCase(ReplaceString(attribute->name, " ", "")).find(LowerCase(ReplaceString(attributeFilter, " ", ""))) == std::string::npos) continue;
                        if (attribute->id == id) continue;
                    } else {
                        if (!attributeFilter.empty() && std::to_string(attribute->id) != ReplaceString(attributeFilter, " ", "")) continue;
                    }
                    ImGui::PushID(attribute->id);
                    if (ImGui::MenuItem(FormatString("%s%s %s", m_parentAttributeID == attribute->id ? ICON_FA_CHECK " " : "", ICON_FA_LINK, attribute->name.c_str()).c_str())) {
                        m_parentAttributeID = attribute->id;
                    }
                    oneCandidateWasDisplayed = true;
                    ImGui::PopID();
                }
                if (!oneCandidateWasDisplayed) {
                    ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - ImGui::CalcTextSize(Localization::GetString("NOTHING_TO_SHOW").c_str()).x / 2.0f);
                    ImGui::Text("%s", Localization::GetString("NOTHING_TO_SHOW").c_str());
                }
                ImGui::Separator();
                if (ImGui::MenuItem(FormatString("%s %s %s", ICON_FA_GLOBE, ICON_FA_LINK, Localization::GetString("MORE_ATTRIBUTES").c_str()).c_str())) {
                    openMoreAttributesPopup = true;
                }
                ImGui::EndMenu();
            }
            if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_XMARK, Localization::GetString("REMOVE_PARENT_ATTRIBUTE").c_str()).c_str())) {
                m_parentAttributeID = -1;
            }
            ImGui::EndPopup();
        }

        if (openMoreAttributesPopup) {
            ImGui::OpenPopup(FormatString("%imoreAttributes", id).c_str());
        }
        if (ImGui::BeginPopup(FormatString("%imoreAttributes", id).c_str())) {
            ImGui::SeparatorText(FormatString("%s %s %s", ICON_FA_GLOBE, ICON_FA_LINK, Localization::GetString("SELECT_EXTERNAL_PARENT_ATTRIBUTE").c_str()).c_str());
            static std::string attributeFilter = "";
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputTextWithHint("##attributeFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_BY_NAME_OR_ATTRIBUTE_ID").c_str()).c_str(), &attributeFilter);
            ImGui::PopItemWidth();

            auto& project = Workspace::s_project.value();
            for (auto& composition : project.compositions) {
                if (composition.attributes.empty()) continue;
                bool skip = !attributeFilter.empty();
                bool mustShowByID = false;
                for (auto& attribute : composition.attributes) {
                    if (std::to_string(attribute->id) == ReplaceString(attributeFilter, " ", "")) {
                        mustShowByID = true;
                        skip = false;
                        break;
                    }
                    if (!attributeFilter.empty() && attribute->name.find(attributeFilter) != std::string::npos) {
                        skip = false;
                        break;
                    }
                }
                if (skip) continue;
                ImGui::PushID(composition.id);
                if (ImGui::TreeNode(FormatString("%s %s", ICON_FA_LAYER_GROUP, composition.name.c_str()).c_str())) {
                    for (auto& attribute : composition.attributes) {
                        if (mustShowByID && ReplaceString(attributeFilter, " ", "") != std::to_string(attribute->id)) continue;
                        ImGui::PushID(attribute->id);
                        std::string attributeIcon = ICON_FA_GLOBE " " ICON_FA_LINK;
                        if (Workspace::GetCompositionByAttributeID(id).value()->id == composition.id) {
                            attributeIcon = ICON_FA_LINK;
                        }
                        if (ImGui::MenuItem(FormatString("%s %s", attributeIcon.c_str(), attribute->name.c_str()).c_str())) {
                            m_parentAttributeID = attribute->id;
                        }
                        ImGui::PopID();
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(TRANSFORM2D_PARENT_PAYLOAD)) {
                m_parentAttributeID = *((int*) payload->Data);
            }
            ImGui::EndDragDropTarget();
        }
        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload(TRANSFORM2D_PARENT_PAYLOAD, &id, sizeof(id));
            ImGui::Text("%s %s: %s", ICON_FA_SITEMAP, Localization::GetString("SET_AS_PARENT_ATTRIBUTE").c_str(), name.c_str());
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginPopup(FormatString("##%iattribute", id).c_str())) {
            bool& linkedSize = m_linkedSize;

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
            {"Keyframes", {}},
            {"LinkedSize", m_linkedSize}
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