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
                    t_attributeID = attribute->id;
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
            ImGui::OpenPopup("##moreAttributes");
        }
        if (ImGui::BeginPopup("##moreAttributes")) {
            ImGui::SeparatorText(FormatString("%s %s %s", ICON_FA_GLOBE, ICON_FA_LINK, Localization::GetString("INSERT_EXTERNAL_ATTRIBUTE_ID").c_str()).c_str());
            static std::string defaultAttributeFilter = "";
            std::string& attributeFilter = t_customAttributeFilter ? *t_customAttributeFilter : defaultAttributeFilter;
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
                        if (t_composition->id == composition.id) {
                            attributeIcon = ICON_FA_LINK;
                        }
                        if (ImGui::MenuItem(FormatString("%s %s", attributeIcon.c_str(), attribute->name.c_str()).c_str())) {
                            t_attributeID = attribute->id;
                        }
                        ImGui::PopID();
                    }
                    ImGui::TreePop();
                }
                ImGui::PopID();
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
                for (auto& asset : project.assets) {
                    auto description = Assets::GetAssetImplementation(asset->packageName).value();
                    if (!s_assetFilter.empty() && LowerCase(asset->name).find(LowerCase(s_assetFilter)) == std::string::npos) continue;
                    if (ImGui::MenuItem(FormatString("%s %s", description.description.icon.c_str(), asset->name.c_str()).c_str())) {
                        t_assetID = asset->id;
                    }
                }
                ImGui::EndPopup();
            }
        }
    }

    void UIHelpers::OpenSelectAssetPopup() {
        ImGui::OpenPopup("##selectAsset");
    }

};