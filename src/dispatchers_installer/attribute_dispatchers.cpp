#include "common/common.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_stdlib.h"
#include "../ImGui/imgui_drag.h"
#include "attribute_dispatchers.h"
#include "overlay_dispatchers.h"
#include "common/transform2d.h"
#include "../ImGui/imgui_stripes.h"

namespace Raster {

    static glm::vec2 FitRectInRect(glm::vec2 dst, glm::vec2 src) {
        float scale = std::min(dst.x / src.x, dst.y / src.y);
        return glm::vec2{src.x * scale, src.y * scale};
    }

    void AttributeDispatchers::DispatchStringAttribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed) {
        std::string string = std::any_cast<std::string>(t_value);
        ImGui::Text("%s", t_attribute.c_str());
        ImGui::SameLine();
        ImGui::InputText(FormatString("##%s", t_attribute.c_str()).c_str(), &string, 0);
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
            ImGui::InputTextWithHint("##attributeFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_BY_NAME_OR_ATTRIBUTE_ID").c_str()).c_str(), &attributeFilter);
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
                } else {
                    if (!attributeFilter.empty() && std::to_string(attribute->id) != ReplaceString(attributeFilter, " ", "")) continue;
                }
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
                        if (!mustShowByID && LowerCase(attribute->name).find(ReplaceString(LowerCase(attributeFilter), " ", "")) == std::string::npos) continue;
                        ImGui::PushID(attribute->id);
                        std::string attributeIcon = ICON_FA_GLOBE " " ICON_FA_LINK;
                        if (Workspace::GetCompositionByNodeID(t_owner->nodeID).value()->id == composition.id) {
                            attributeIcon = ICON_FA_LINK;
                        }
                        if (ImGui::MenuItem(FormatString("%s %s", attributeIcon.c_str(), attribute->name.c_str()).c_str())) {
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
        auto& project = Workspace::GetProject();
        if (!project.customData.contains("Vec4AttributeDispatcherData")) {
            project.customData["Vec4AttributeDispatcherData"] = {
                {"Placeholder", false}
            };
        }
        auto& interpretAsColorDict = project.customData["Vec4AttributeDispatcherData"];
        std::string id = FormatString("##%s%i", t_attribute.c_str(), t_owner->nodeID);
        if (!interpretAsColorDict.contains(id)) {
            interpretAsColorDict[id] = false;
        }
        bool interpretAsColor = interpretAsColorDict[id];

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

        interpretAsColorDict[id] = interpretAsColor;
    }

    void AttributeDispatchers::DispatchVec3Attribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed) {
        auto& project = Workspace::GetProject();
        if (!project.customData.contains("Vec3AttributeDispatcherData")) {
            project.customData["Vec3AttributeDispatcherData"] = {
                {"Placeholder", false}
            };
        }
        auto& interpretAsColorDict = project.customData["Vec3AttributeDispatcherData"];
        std::string id = FormatString("##%s%i", t_attribute.c_str(), t_owner->nodeID);
        if (!interpretAsColorDict.contains(id)) {
            interpretAsColorDict[id] = false;
        }
        bool interpretAsColor = interpretAsColorDict[id];

        auto v = std::any_cast<glm::vec3>(t_value);
        ImGui::Text("%s", t_attribute.c_str());
        ImGui::SameLine();
        if (ImGui::Button(interpretAsColor ? ICON_FA_DROPLET : ICON_FA_EXPAND)) {
            interpretAsColor = !interpretAsColor;
        }
        ImGui::SetItemTooltip("%s %s", interpretAsColor ? ICON_FA_DROPLET : ICON_FA_EXPAND, Localization::GetString("VECTOR_INTERPRETATION_MODE").c_str());
        ImGui::SameLine();
        if (!interpretAsColor) {
            ImGui::DragFloat3(FormatString("##%s", t_attribute.c_str()).c_str(), glm::value_ptr(v));
        } else {
            ImGui::PushItemWidth(200);
                ImGui::ColorPicker3("##colorPreview", glm::value_ptr(v), ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoSidePreview);
            ImGui::PopItemWidth();
        }

        t_value = v;

        interpretAsColorDict[id] = interpretAsColor;
    }

    void AttributeDispatchers::DispatchVec2Attribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed) {
        auto& project = Workspace::GetProject();

        if (!project.customData.contains("Vec2AttributeDispatcherData")) {
            project.customData["Vec4AttributeDispatcherData"] = {
                {"Placeholder", false}
            };
        }
        auto& linkedSizeDict = project.customData["Vec2AttributeDispatcherData"];
        std::string id = FormatString("##%s%i", t_attribute.c_str(), t_owner->nodeID);
        if (!linkedSizeDict.contains(id)) {
            linkedSizeDict[id] = false;
        }
        bool linkedSize = linkedSizeDict[id];

        auto v = std::any_cast<glm::vec2>(t_value);
        auto reservedVector = v;
        ImGui::Text("%s", t_attribute.c_str());
        ImGui::SameLine();
        if (ImGui::Button(linkedSize ? ICON_FA_LINK : ICON_FA_LINK_SLASH)) {
            linkedSize = !linkedSize;
        }
        ImGui::SameLine();
        ImGui::DragFloat2(FormatString("##%s", t_attribute.c_str()).c_str(), glm::value_ptr(v));
        if (linkedSize) {
            if (reservedVector.x != v.x) {
                v.y = v.x;
            } else if (reservedVector.y != v.y) {
                v.x = v.y;
            }
        }

        linkedSizeDict[id] = linkedSize;

        t_value = v;

    }

    void AttributeDispatchers::DispatchTransform2DAttribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed) {
        auto fitSize = FitRectInRect({ImGui::GetWindowSize().x, 256}, Workspace::s_project.value().preferredResolution);
        ImVec2 iFitSize = ImVec2(fitSize.x, fitSize.y);
        ImGui::Text("%s", t_attribute.c_str());
        ImGui::SameLine();
        ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - iFitSize.x / 2.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::BeginChild("##transformContainer", iFitSize, ImGuiChildFlags_Border);
            ImGui::Stripes(ImVec4(0.05f, 0.05f, 0.05f, 1), ImVec4(0.1f, 0.1f, 0.1f, 1), 40, 20, iFitSize);
            OverlayDispatchers::s_attributeName = t_attribute;
            OverlayDispatchers::DispatchTransform2DValue(t_value, Workspace::GetCompositionByNodeID(t_owner->nodeID).value(), t_owner->nodeID, 1.0f, fitSize);
        ImGui::EndChild();
        ImGui::PopStyleVar();

        auto transform = std::any_cast<Transform2D>(t_value);

        auto& project = Workspace::s_project.value();
        if (!project.customData.contains("Transform2DAttributeData")) {
            project.customData["Transform2DAttributeData"] = {};
        }
        auto& customData = project.customData["Transform2DAttributeData"];
        auto stringID = std::to_string(t_owner->nodeID) + t_attribute;
        if (!customData.contains(stringID)) {
            customData[stringID] = false;
        }
        bool linkedSize = customData[stringID];
        
        ImGui::BeginChild("##transformDrags", ImVec2(ImGui::GetWindowSize().x, 0));
            ImGui::Text("%s Position", ICON_FA_UP_DOWN_LEFT_RIGHT);
            ImGui::SameLine();
            float cursorX = ImGui::GetCursorPosX();
            ImGui::DragFloat2("##dragPosition", glm::value_ptr(transform.position), 0.05f);
            ImGui::SameLine(0, 0);
            float dragWidth = ImGui::GetCursorPosX() - cursorX;
            ImGui::NewLine();

            ImGui::Text("%s Size", ICON_FA_SCALE_BALANCED);
            ImGui::SameLine();
            ImGui::SetCursorPosX(cursorX);
            if (ImGui::Button(linkedSize ? ICON_FA_LINK : ICON_FA_LINK_SLASH)) {
                linkedSize = !linkedSize;
            }
            ImGui::SameLine(0, 0);
            float buttonCursor = ImGui::GetCursorPosX() - cursorX;
            ImGui::NewLine();
            ImGui::SetItemTooltip("%s %s", ICON_FA_LINK, "Link Dimensions");
            ImGui::SameLine(0, 2.0f);
            glm::vec2 reservedSize = transform.size;
            ImGui::PushItemWidth(dragWidth - buttonCursor - 2);
                ImGui::DragFloat2("##dragSize", glm::value_ptr(transform.size), 0.05f);
            ImGui::PopItemWidth();
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

            ImGui::Text("%s Rotation", ICON_FA_ROTATE);
            ImGui::SameLine();
            ImGui::SetCursorPosX(cursorX);
            ImGui::DragFloat("##dragAngle", &transform.angle, 0.5f);
        ImGui::EndChild();

        t_value = transform;
    }

    void AttributeDispatchers::DispatchSamplerSettingsAttribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed) {
        auto samplerSettings = std::any_cast<SamplerSettings>(t_value);
        static std::vector<TextureWrappingMode> wrappingModes = {
            TextureWrappingMode::Repeat,
            TextureWrappingMode::MirroredRepeat,
            TextureWrappingMode::ClampToEdge, 
            TextureWrappingMode::ClampToBorder
        };
        int selectedWrappingMode = 0;
        for (auto& mode : wrappingModes) {
            if (mode == samplerSettings.wrappingMode) break;
            selectedWrappingMode++;
        }

        std::vector<std::string> wrappingStringModes;
        for (auto& mode : wrappingModes) {
            wrappingStringModes.push_back(GPU::TextureWrappingModeToString(mode));
        }

        std::vector<const char*> wrappingRawStringModes;
        for (auto& stringMode : wrappingStringModes) {
            wrappingRawStringModes.push_back(stringMode.c_str());
        }

        ImGui::Text("%s %s: ", ICON_FA_IMAGE, Localization::GetString("WRAPPING_MODE").c_str());
        ImGui::SameLine();
        ImGui::Combo("##wrappingModes", &selectedWrappingMode, wrappingRawStringModes.data(), wrappingRawStringModes.size());

        samplerSettings.wrappingMode = wrappingModes[selectedWrappingMode];

        static std::vector<TextureFilteringMode> filteringModes = {
            TextureFilteringMode::Linear,
            TextureFilteringMode::Nearest
        };

        int selectedFilteringMode = 0;
        for (auto& mode : filteringModes) {
            if (samplerSettings.filteringMode == mode) break;
            selectedFilteringMode++;
        }

        std::vector<std::string> filteringStringModes;
        for (auto& mode : filteringModes) {
            filteringStringModes.push_back(GPU::TextureFilteringModeToString(mode));
        }
        
        std::vector<const char*> filteringRawStringModes;
        for (auto& stringMode : filteringStringModes) {
            filteringRawStringModes.push_back(stringMode.c_str());
        }

        ImGui::Text("%s %s: ", ICON_FA_IMAGE, Localization::GetString("FILTERING_MODE").c_str());
        ImGui::SameLine();
        ImGui::Combo("##filteringMode", &selectedFilteringMode, filteringRawStringModes.data(), filteringRawStringModes.size());
        samplerSettings.filteringMode = filteringModes[selectedFilteringMode];

        t_value = samplerSettings;
    }

    void AttributeDispatchers::DispatchBoolAttribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed) {
        bool value = std::any_cast<bool>(t_value);
        ImGui::Text("%s", t_attribute.c_str());
        ImGui::SameLine();
        ImGui::Checkbox(FormatString("##%s", t_attribute.c_str()).c_str(), &value);
        t_value = value;
    }
};