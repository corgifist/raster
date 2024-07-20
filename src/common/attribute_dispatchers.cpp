#include "common/common.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_stdlib.h"

namespace Raster {
    void AttributeDispatchers::DispatchStringAttribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed) {
        std::string string = std::any_cast<std::string>(t_value);
        ImGui::Text("%s", t_attribute.c_str());
        ImGui::SameLine();
        ImGui::InputText(FormatString("##%s", t_attribute.c_str()).c_str(), &string, t_isAttributeExposed ? ImGuiInputTextFlags_ReadOnly : 0);
        t_value = string;
    }

    void AttributeDispatchers::DispatchFloatAttribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed) {
        float f = std::any_cast<float>(t_value);
        ImGui::Text("%s", t_attribute.c_str());
        ImGui::SameLine();
        ImGui::DragFloat(FormatString("##%s", t_attribute.c_str()).c_str(), &f);
        t_value = f;
    }

    void AttributeDispatchers::DispatchIntAttribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed) {
        int i = std::any_cast<int>(t_value);
        ImGui::Text("%s", t_attribute.c_str());
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_GEARS)) {
            ImGui::OpenPopup(FormatString("%i%s", t_owner->nodeID, t_attribute.c_str()).c_str());
        }
        ImGui::SameLine();
        ImGui::DragInt(FormatString("##%s", t_attribute.c_str()).c_str(), &i);
        bool openMoreAttributesPopup = false;
        if (ImGui::BeginPopup(FormatString("%i%s", t_owner->nodeID, t_attribute.c_str()).c_str())) {
            ImGui::SeparatorText(FormatString("%s %s", ICON_FA_GEARS, Localization::GetString("INSERT_ATTRIBUTE_ID").c_str()).c_str());
            auto parentComposition = Workspace::GetCompositionByNodeID(t_owner->nodeID).value();
            static std::string attributeFilter = "";
            ImGui::InputText("##attributeFilter", &attributeFilter);
            ImGui::SetItemTooltip("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str());
            for (auto& attribute : parentComposition->attributes) {
                if (!attributeFilter.empty() && attribute->name.find(attributeFilter) == std::string::npos) continue;
                ImGui::PushID(attribute->id);
                if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_LINK, attribute->name.c_str()).c_str())) {
                    i = attribute->id;
                }
                ImGui::PopID();
            }
            ImGui::Separator();
            if (ImGui::MenuItem(FormatString("%s %s %s", ICON_FA_GLOBE, ICON_FA_LINK, Localization::GetString("MORE_ATTRIBUTES").c_str()).c_str())) {
                openMoreAttributesPopup = true;
            }
            ImGui::EndPopup();
        }
        if (openMoreAttributesPopup) {
            ImGui::OpenPopup(FormatString("%i%smoreAttributes", t_owner->nodeID, t_attribute.c_str()).c_str());
        }
        if (ImGui::BeginPopup(FormatString("%i%smoreAttributes", t_owner->nodeID, t_attribute.c_str()).c_str())) {
            ImGui::SeparatorText(FormatString("%s %s %s", ICON_FA_GLOBE, ICON_FA_LINK, Localization::GetString("INSERT_EXTERNAL_ATTRIBUTE_ID").c_str()).c_str());
            static std::string attributeFilter = "";
            ImGui::InputText("##attributeFilter", &attributeFilter);
            ImGui::SetItemTooltip("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str());

            auto& project = Workspace::s_project.value();
            for (auto& composition : project.compositions) {
                if (composition.attributes.empty()) continue;
                bool skip = !attributeFilter.empty();
                for (auto& attribute : composition.attributes) {
                    if (!attributeFilter.empty() && attribute->name.find(attributeFilter) != std::string::npos) {
                        skip = false;
                        break;
                    }
                }
                if (skip) continue;
                ImGui::PushID(composition.id);
                if (ImGui::TreeNode(FormatString("%s %s", ICON_FA_LAYER_GROUP, composition.name.c_str()).c_str())) {
                    for (auto& attribute : composition.attributes) {
                        ImGui::PushID(attribute->id);
                        if (ImGui::MenuItem(FormatString("%s %s %s", ICON_FA_GLOBE, ICON_FA_LINK, attribute->name.c_str()).c_str())) {
                            i = attribute->id;
                        }
                        ImGui::PopID();
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
            ImGui::EndPopup();
        }
        t_value = i;
    }

    void AttributeDispatchers::DispatchVec4Attribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed) {
        static std::unordered_map<std::string, bool> s_interpretAsColor;
        std::string id = FormatString("##%s%i", t_attribute.c_str(), t_owner->nodeID);
        if (s_interpretAsColor.find(id) == s_interpretAsColor.end()) {
            s_interpretAsColor[id] = false;
        }
        bool& interpretAsColor = s_interpretAsColor[id];

        glm::vec4 v = std::any_cast<glm::vec4>(t_value);
        ImGui::Text("%s", t_attribute.c_str());
        ImGui::SameLine();
        if (ImGui::Button(interpretAsColor ? ICON_FA_DROPLET : ICON_FA_EXPAND)) {
            interpretAsColor = !interpretAsColor;
        }
        ImGui::SetItemTooltip("%s %s", interpretAsColor ? ICON_FA_DROPLET : ICON_FA_EXPAND, Localization::GetString("VECTOR_INTERPRETATION_MODE").c_str());
        ImGui::SameLine();
        if (!interpretAsColor) {
            ImGui::DragFloat4(FormatString("##%s", t_attribute.c_str()).c_str(), glm::value_ptr(v));
        } else {
            ImGui::PushItemWidth(200);
                ImGui::ColorPicker4("##colorPreview", glm::value_ptr(v), ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoSidePreview);
            ImGui::PopItemWidth();
        }

        t_value = v;
    }
};