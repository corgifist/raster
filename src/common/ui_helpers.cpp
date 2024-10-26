#include "common/ui_helpers.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_stdlib.h"

namespace Raster {
    void UIHelpers::SelectAttribute(Composition* t_composition, int& t_attributeID, std::string t_headerText, std::string* t_customAttributeFilter) {
        int reservedID = t_attributeID;
        bool openMoreAttributesPopup = false;
        if (ImGui::BeginPopup(FormatString("##attributeSelector").c_str())) {
            ImGui::SeparatorText(FormatString("%s %s", ICON_FA_GEARS, Localization::GetString("INSERT_ATTRIBUTE_ID").c_str()).c_str());
            auto parentComposition = t_composition;
            static std::string defaultAttributeFilter = "";
            std::string& attributeFilter = t_customAttributeFilter ? *t_customAttributeFilter : defaultAttributeFilter;
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputTextWithHint("##attributeFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_BY_NAME_OR_ID").c_str()).c_str(), &attributeFilter);
            ImGui::PopItemWidth();

            if (ImGui::BeginChild("##attributesChild", ImVec2(ImGui::GetContentRegionAvail().x, RASTER_PREFERRED_POPUP_HEIGHT))) {
                bool mustShowByID = false;
                bool hasCandidates = false;
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
                    hasCandidates = true;
                    if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_LINK, attribute->name.c_str()).c_str())) {
                        t_attributeID = attribute->id;
                    }
                    ImGui::PopID();
                }
                if (!hasCandidates) {
                    std::string nothingToShowText = Localization::GetString("NOTHING_TO_SHOW");
                    ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2 - ImGui::CalcTextSize(nothingToShowText.c_str()).x);
                    ImGui::Text("%s", nothingToShowText.c_str());
                }
            }
            ImGui::EndChild();

            ImGui::Separator();
            if (ImGui::MenuItem(FormatString("%s %s %s", ICON_FA_GLOBE, ICON_FA_LINK, Localization::GetString("MORE_ATTRIBUTES").c_str()).c_str())) {
                openMoreAttributesPopup = true;
            }
            ImGui::EndPopup();
        }
        if (openMoreAttributesPopup) {
            ImGui::OpenPopup("##moreAttributes");
        }
        if (ImGui::BeginPopup("##moreAttributes")) {
            ImGui::SeparatorText(FormatString("%s %s %s", ICON_FA_GLOBE, ICON_FA_LINK, Localization::GetString("INSERT_EXTERNAL_ATTRIBUTE_ID").c_str()).c_str());
            static std::string defaultAttributeFilter = "";
            std::string& attributeFilter = t_customAttributeFilter ? *t_customAttributeFilter : defaultAttributeFilter;
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputTextWithHint("##attributeFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_BY_NAME_OR_ID").c_str()).c_str(), &attributeFilter);
            ImGui::PopItemWidth();
            ImGui::SetItemTooltip("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str());

            if (ImGui::BeginChild("##externalAttributesChild", ImVec2(ImGui::GetContentRegionAvail().x, RASTER_PREFERRED_POPUP_HEIGHT))) {
                auto& project = Workspace::s_project.value();
                bool hasCandidates = true;
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
                            if (t_composition->id == composition.id) {
                                attributeIcon = ICON_FA_LINK;
                            }
                            hasCandidates = true;
                            if (ImGui::MenuItem(FormatString("%s %s", attributeIcon.c_str(), attribute->name.c_str()).c_str())) {
                                t_attributeID = attribute->id;
                            }
                            ImGui::PopID();
                        }
                        ImGui::TreePop();
                    }
                    ImGui::PopID();
                }
                if (!hasCandidates) {
                    std::string nothingToShowText = Localization::GetString("NOTHING_TO_SHOW");
                    ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2 - ImGui::CalcTextSize(nothingToShowText.c_str()).x);
                    ImGui::Text("%s", nothingToShowText.c_str());
                }
                ImGui::EndChild();
            }
            ImGui::EndPopup();
        }
    }

    void UIHelpers::OpenSelectAttributePopup() {
        ImGui::OpenPopup("##attributeSelector");
    }
    
    void UIHelpers::SelectAsset(int& t_assetID, std::string t_headerText, std::string* t_customAttributeFilter) {
        if (Workspace::IsProjectLoaded()) {
            auto& project = Workspace::GetProject();
            if (ImGui::BeginPopup("##selectAsset")) {
                ImGui::SeparatorText(FormatString("%s %s", ICON_FA_GEARS, Localization::GetString("INSERT_ASSET_ID").c_str()).c_str());
                static std::string s_assetFilter = "";
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputTextWithHint("##assetFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &s_assetFilter);
                ImGui::PopItemWidth();
                if (ImGui::BeginChild("##assetsChild", ImVec2(ImGui::GetContentRegionAvail().x, RASTER_PREFERRED_POPUP_HEIGHT))) {
                    bool hasCandidates = false;
                    for (auto& asset : project.assets) {
                        auto description = Assets::GetAssetImplementation(asset->packageName).value();
                        if (!s_assetFilter.empty() && LowerCase(asset->name).find(LowerCase(s_assetFilter)) == std::string::npos) continue;
                        hasCandidates = true;
                        if (ImGui::MenuItem(FormatString("%s %s", description.description.icon.c_str(), asset->name.c_str()).c_str())) {
                            t_assetID = asset->id;
                        }
                    }
                    if (!hasCandidates) {
                        std::string nothingToShowText = Localization::GetString("NOTHING_TO_SHOW");
                        ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2 - ImGui::CalcTextSize(nothingToShowText.c_str()).x);
                        ImGui::Text("%s", nothingToShowText.c_str());
                    }
                }
                ImGui::EndChild();
                ImGui::EndPopup();
            }
        }
    }

    void UIHelpers::OpenSelectAssetPopup() {
        ImGui::OpenPopup("##selectAsset");
    }

    struct CustomTreeNodeData {
        bool opened;
        bool hovered;

        CustomTreeNodeData() : opened(false), hovered(false) {}
    };

    bool UIHelpers::CustomTreeNode(std::string t_id) {
        static std::unordered_map<ImGuiID, CustomTreeNodeData> s_data;
        auto id = ImGui::GetID(t_id.c_str());
        if (s_data.find(id) == s_data.end()) {
            s_data[id] = CustomTreeNodeData();
        }

        auto& data = s_data[id];

        ImVec4 textColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
        if (data.hovered) ImGui::PushStyleColor(ImGuiCol_Text, textColor);
        ImGui::Text("%s %s", data.opened ? ICON_FA_ANGLE_DOWN : ICON_FA_ANGLE_RIGHT, t_id.c_str());
        if (data.hovered) ImGui::PopStyleColor();
        data.hovered = ImGui::IsItemHovered();

        if (ImGui::IsItemClicked()) data.opened = !data.opened;
        return data.opened;
    }
};