#include "common/common.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_stdlib.h"
#include "../ImGui/imgui_drag.h"
#include "attribute_dispatchers.h"
#include "overlay_dispatchers.h"
#include "common/transform2d.h"
#include "../ImGui/imgui_stripes.h"
#include "common/dispatchers.h"
#include "common/ui_helpers.h"

namespace Raster {

    static glm::vec2 FitRectInRect(glm::vec2 dst, glm::vec2 src) {
        float scale = std::min(dst.x / src.x, dst.y / src.y);
        return glm::vec2{src.x * scale, src.y * scale};
    }

    struct ParsedMetadata {
        bool isSlider;
        float sliderMin, sliderMax;
        float sliderStep;
        std::string sliderFormat;
        float base;

        ParsedMetadata() {
            this->isSlider = false;
            this->sliderFormat = "%0.2f";
            this->base = 1.0f;
        }
    };

    static ParsedMetadata ParseMetadata(std::optional<std::vector<std::any>> t_metadata) {
        bool isSlider = false;
        float sliderMin, sliderMax;
        std::string format = "";
        float step = FLT_MAX;
        float base = 1.0f;
        if (t_metadata.has_value()) {
            auto& metadata = t_metadata.value();
            for (auto& info : metadata) {
                if (info.type() == typeid(SliderRangeMetadata)) {
                    auto sliderInfo = std::any_cast<SliderRangeMetadata>(info);
                    isSlider = true;
                    sliderMin = sliderInfo.min;
                    sliderMax = sliderInfo.max;
                }
                if (info.type() == typeid(FormatStringMetadata)) {
                    auto formatString = std::any_cast<FormatStringMetadata>(info);
                    format = " " + formatString.format;
                }
                if (info.type() == typeid(SliderStepMetadata)) {
                    step = std::any_cast<SliderStepMetadata>(info).step;
                }
                if (info.type() == typeid(SliderBaseMetadata)) {
                    base = std::any_cast<SliderBaseMetadata>(info).base;
                }
            }
        }

        ParsedMetadata result;
        result.isSlider = isSlider;
        result.sliderMin = sliderMin;
        result.sliderMax = sliderMax;
        result.sliderStep = step == FLT_MAX ? 1.0f : step;
        result.sliderFormat = format;
        result.base = base;
        return result;
    }

    static std::string GetMetadataFormat(ParsedMetadata t_metadata, std::string t_type = "0.2f") {
        return FormatString("%%%s%s", t_type.c_str(), ReplaceString(t_metadata.sliderFormat, "\\%", "%%").c_str());
    }

    void AttributeDispatchers::DispatchStringAttribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed, std::vector<std::any> t_metadata) {
        std::string string = std::any_cast<std::string>(t_value);
        ImGui::Text("%s", t_attribute.c_str());
        ImGui::SameLine();
        ImGui::InputText(FormatString("##%s", t_attribute.c_str()).c_str(), &string, 0);
        t_value = string;
    }

    void AttributeDispatchers::DispatchFloatAttribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed, std::vector<std::any> t_metadata) {
        float f = std::any_cast<float>(t_value);
        ImGui::Text("%s", t_attribute.c_str());
        ImGui::SameLine();
        auto metadata = ParseMetadata(t_metadata);
        std::string formattedAttribute = FormatString("##%s", t_attribute.c_str());
        f *= metadata.base;
        if (!metadata.isSlider) {
            ImGui::DragFloat(formattedAttribute.c_str(), &f, metadata.sliderStep, 0, 0, GetMetadataFormat(metadata).c_str());
        } else {
            ImGui::SliderFloat(formattedAttribute.c_str(), &f, metadata.sliderMin, metadata.sliderMax, GetMetadataFormat(metadata).c_str());
        }
        f /= metadata.base;
        t_value = f;
    }

    void AttributeDispatchers::DispatchIntAttribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed, std::vector<std::any> t_metadata) {
        auto& project = Workspace::GetProject();
        auto composition = Workspace::GetCompositionByNodeID(t_owner->nodeID).value();
        int i = std::any_cast<int>(t_value);
        ImGui::Text("%s", t_attribute.c_str());
        ImGui::SameLine();
        if (ImGui::Button(ICON_FA_GEARS)) {
            ImGui::OpenPopup(FormatString("##insertValue%i%s", t_owner->nodeID, t_attribute.c_str()).c_str());
        }
        ImGui::SameLine();

        auto metadata = ParseMetadata(t_metadata);
        std::string formattedAttribute = FormatString("##%s", t_attribute.c_str());
        if (!metadata.isSlider) {
            ImGui::DragInt(formattedAttribute.c_str(), &i, metadata.sliderStep, 0, 0, GetMetadataFormat(metadata, "i").c_str());
        } else {
            ImGui::SliderInt(formattedAttribute.c_str(), &i, metadata.sliderMin, metadata.sliderMax, GetMetadataFormat(metadata, "i").c_str());
        }

        bool openAssetIDPopup = false;
        bool openAttributesPopup = false;
        if (ImGui::BeginPopup(FormatString("##insertValue%i%s", t_owner->nodeID, t_attribute.c_str()).c_str())) {
            ImGui::SeparatorText(FormatString("%s %s", ICON_FA_GEARS, Localization::GetString("INSERT_VALUE").c_str()).c_str());
            if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_GEARS, Localization::GetString("INSERT_ASSET_ID").c_str()).c_str())) {
                openAssetIDPopup = true;
            }
            if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_GEARS, Localization::GetString("INSERT_ATTRIBUTE_ID").c_str()).c_str())) {
                openAttributesPopup = true;
            }
            ImGui::EndPopup();
        }

        if (openAssetIDPopup) {
            ImGui::OpenPopup(FormatString("##selectAsset%i%s", t_owner->nodeID, t_attribute.c_str()).c_str());
        }

        static std::string attributesSearchFilter = "";
        auto attributeCandidate = Workspace::GetAttributeByAttributeID(i);
        ImGui::PushID(t_owner->nodeID);
        ImGui::PushID(composition->id);
            if (openAttributesPopup) {
                UIHelpers::OpenSelectAttributePopup();
            }

            UIHelpers::SelectAttribute(composition, i, attributeCandidate.has_value() ? attributeCandidate.value()->name : "", &attributesSearchFilter);
        ImGui::PopID();
        ImGui::PopID();

        static std::string assetsSearchFilter = "";
        auto assetCandidate = Workspace::GetAssetByAssetID(i);
        ImGui::PushID(t_owner->nodeID);
        ImGui::PushID(composition->id);
            if (openAssetIDPopup) {
                UIHelpers::OpenSelectAssetPopup();
            }

            UIHelpers::SelectAsset(i, assetCandidate.has_value() ? assetCandidate.value()->name : "", &assetsSearchFilter);
        ImGui::PopID();
        ImGui::PopID();


        t_value = i;
    }

    void AttributeDispatchers::DispatchVec4Attribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed, std::vector<std::any> t_metadata) {
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
        auto metadata = ParseMetadata(t_metadata);
        v *= metadata.base;
        if (!interpretAsColor) {
            std::string formattedAttribute = FormatString("##%s", t_attribute.c_str());
            if (!metadata.isSlider) {
                ImGui::DragFloat4(formattedAttribute.c_str(), glm::value_ptr(v), metadata.sliderStep, 0, 0, GetMetadataFormat(metadata).c_str());
            } else {
                ImGui::SliderFloat4(formattedAttribute.c_str(), glm::value_ptr(v), metadata.sliderMin, metadata.sliderMax, GetMetadataFormat(metadata).c_str());
            }
        } else {
            ImGui::PushItemWidth(200);
                ImGui::ColorPicker4("##colorPreview", glm::value_ptr(v), ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoSidePreview);
            ImGui::PopItemWidth();
        }
        v /= metadata.base;
        t_value = v;

        interpretAsColorDict[id] = interpretAsColor;
    }

    void AttributeDispatchers::DispatchVec3Attribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed, std::vector<std::any> t_metadata) {
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
        auto metadata = ParseMetadata(t_metadata);
        v *= metadata.base;
        if (!interpretAsColor) {
            std::string formattedAttribute = FormatString("##%s", t_attribute.c_str());
            if (!metadata.isSlider) {
                ImGui::DragFloat4(formattedAttribute.c_str(), glm::value_ptr(v), metadata.sliderStep, 0, 0, GetMetadataFormat(metadata).c_str());
            } else {
                ImGui::SliderFloat4(formattedAttribute.c_str(), glm::value_ptr(v), metadata.sliderMin, metadata.sliderMax, GetMetadataFormat(metadata).c_str());
            }
        } else {
            ImGui::PushItemWidth(200);
                ImGui::ColorPicker3("##colorPreview", glm::value_ptr(v), ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoSidePreview);
            ImGui::PopItemWidth();
        }
        v /= metadata.base;
        t_value = v;

        interpretAsColorDict[id] = interpretAsColor;
    }

    void AttributeDispatchers::DispatchVec2Attribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed, std::vector<std::any> t_metadata) {
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
        auto metadata = ParseMetadata(t_metadata);
        v *= metadata.base;
        std::string formattedAttribute = FormatString("##%s", t_attribute.c_str());
        if (!metadata.isSlider) {
            ImGui::DragFloat2(formattedAttribute.c_str(), glm::value_ptr(v), metadata.sliderStep, 0, 0, GetMetadataFormat(metadata).c_str());
        } else {
            ImGui::SliderFloat2(formattedAttribute.c_str(), glm::value_ptr(v), metadata.sliderMin, metadata.sliderMax, GetMetadataFormat(metadata).c_str());
        }
        v /= metadata.base;
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

    void AttributeDispatchers::DispatchTransform2DAttribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed, std::vector<std::any> t_metadata) {
        auto fitSize = FitRectInRect({ImGui::GetWindowSize().x, 256}, Workspace::s_project.value().preferredResolution);
        ImVec2 iFitSize = ImVec2(fitSize.x, fitSize.y);
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

    void AttributeDispatchers::DispatchSamplerSettingsAttribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed, std::vector<std::any> t_metadata) {
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

    void AttributeDispatchers::DispatchBoolAttribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed, std::vector<std::any> t_metadata) {
        bool value = std::any_cast<bool>(t_value);
        ImGui::Text("%s", t_attribute.c_str());
        ImGui::SameLine();
        ImGui::Checkbox(FormatString("##%s", t_attribute.c_str()).c_str(), &value);
        t_value = value;
    }
};