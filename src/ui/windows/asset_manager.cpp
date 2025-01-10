#include "asset_manager.h"
#include "common/ui_shared.h"

#define ASSET_MANAGER_DRAG_DROP_PAYLOAD "ASSET_MANAGER_DRAG_DROP_PAYLOAD"

namespace Raster {

    enum class AssetPreviewType {
        Table, Grid, List
    };

    static std::unordered_map<AssetPreviewType, const char*> s_assetPreviewIconMap = {
        {AssetPreviewType::Table, ICON_FA_TABLE},
        {AssetPreviewType::Grid, ICON_FA_BORDER_ALL},
        {AssetPreviewType::List, ICON_FA_LIST}
    };

    static std::unordered_map<AssetPreviewType, std::string> s_assetPreviewNameMap;

    static std::string ConvertToHRSize(std::uintmax_t size) {
        int o{};
        double mantissa = size;
        for (; mantissa >= 1024.; mantissa /= 1024., ++o);
        std::string os = FormatString("%0.2f", std::ceil(mantissa * 10.) / 10.) + "BKMGTPE"[o];
        return o ? os + "B" : os;
    }

    static ImVec2 FitRectInRect(ImVec2 dst, ImVec2 src) {
        float scale = std::min(dst.x / src.x, dst.y / src.y);
        return ImVec2{src.x * scale, src.y * scale};
    }

    static std::vector<int> s_targetDeleteAssets;

    void AssetManagerUI::RenderAssetPopup(AbstractAsset& t_asset) {
        auto& project = Workspace::GetProject();
        t_asset->RenderPopup();
        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_FOLDER_PLUS, Localization::GetString("REIMPORT_ASSET").c_str()).c_str())) {
            auto originalAsset = t_asset;
            auto assetCandidate = ImportAsset();
            if (assetCandidate.has_value()) {
                t_asset->Delete();
                t_asset = assetCandidate.value();
                t_asset->id = originalAsset->id;
            }
        }
        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_REPLY, Localization::GetString("REPLACE_WITH_PLACEHOLDER_ASSET").c_str()).c_str())) {
            auto placeholderCandidate = Assets::InstantiateAsset(RASTER_PACKAGED "placeholder_asset");
            if (placeholderCandidate.has_value()) {
                t_asset->Delete();
                int originalID = t_asset->id;
                std::string originalName = t_asset->name;
                t_asset = placeholderCandidate.value();
                t_asset->id = originalID;
                t_asset->name = originalName;
            }
        }
        auto pathCandidate = t_asset->GetPath();
        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_FOLDER_OPEN, Localization::GetString("REVEAL_IN_FILE_EXPLORER").c_str()).c_str(), "Ctrl+R", nullptr, pathCandidate.has_value())) {
            auto& path = pathCandidate.value();
            if (std::filesystem::exists(path)) {
                SystemOpenURL(ReplaceString(path, " ", "\\ "));
            }
        }
        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_CLONE, Localization::GetString("DUPLICATE_ASSETS").c_str()).c_str(), "Ctrl+D")) {
            std::vector<int> tempSelectedAssets;
            for (auto& assetID : project.selectedAssets) {
                auto selectedAssetCandidate = Workspace::GetAssetByAssetID(assetID);
                if (selectedAssetCandidate.has_value()) {
                    auto& selectedAsset = selectedAssetCandidate.value();
                    auto duplicateCandidate = Assets::CopyAsset(selectedAsset);
                    if (duplicateCandidate.has_value()) {
                        project.assets.push_back(duplicateCandidate.value());
                        tempSelectedAssets.push_back(duplicateCandidate.value()->id);
                    }
                }
            }
            project.selectedAssets = tempSelectedAssets;
        }
        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_TRASH_CAN, Localization::GetString("DELETE_ASSETS").c_str()).c_str(), "Delete")) {
            s_targetDeleteAssets = project.selectedAssets;
            project.selectedAssets.clear();
        }
    }

    std::optional<AbstractAsset> AssetManagerUI::ImportAsset(std::optional<std::string> targetAssetPackageName) {
        std::vector<std::string> stringStorage;
        std::vector<nfdfilteritem_t> filePickerFilters;
        for (auto& implementation : Assets::s_implementations) {
            auto& description = implementation.description;
            if (targetAssetPackageName.has_value() && targetAssetPackageName.value() != description.packageName) continue; 
            if (description.extensions.empty()) continue;
            nfdfilteritem_t filter;
            filter.name = description.prettyName.c_str();
            
            std::string extensionsAcc = "";

            for (int i = 0; i < description.extensions.size(); i++) {
                bool last = (i == description.extensions.size() - 1);
                extensionsAcc += description.extensions[i] + (last ? "" : ",");
            }
            stringStorage.push_back(extensionsAcc);
            filter.spec = stringStorage.back().c_str();

            filePickerFilters.push_back(filter);
        }

        if (!filePickerFilters.empty()) {
            NFD::UniquePath path;
            auto result = NFD::OpenDialog(path, filePickerFilters.data(), filePickerFilters.size());
            if (result == NFD_OKAY) {
                std::optional<std::string> assetPackageNameCandidate;
                std::string pathExtension = GetExtension(path.get());
                pathExtension = ReplaceString(pathExtension, "\\.", "");
                for (auto& implementation : Assets::s_implementations) {
                    auto& extensions = implementation.description.extensions;
                    if (std::find(extensions.begin(), extensions.end(), pathExtension) != extensions.end()) {
                        assetPackageNameCandidate = implementation.description.packageName;
                        break;
                    }
                }

                if (assetPackageNameCandidate.has_value()) {
                    auto& assetPackageName = assetPackageNameCandidate.value();
                    auto assetCandidate = Assets::InstantiateAsset(assetPackageName);
                    if (assetCandidate.has_value()) {
                        auto& asset = assetCandidate.value();
                        asset->Import(path.get());
                        return asset;
                    }
                }
            }
        }

        if (filePickerFilters.empty() && targetAssetPackageName.has_value()) {
            auto assetCandidate = Assets::InstantiateAsset(targetAssetPackageName.value());
            return assetCandidate;
        }

        return std::nullopt;
    }

    void AssetManagerUI::AutoImportAsset(std::optional<std::string> targetAssetPackageName) {
        auto assetCandidate = ImportAsset(targetAssetPackageName);
        if (assetCandidate.has_value()) {
            auto& project = Workspace::GetProject();
            auto& asset = assetCandidate.value();
            project.assets.push_back(asset);
            project.selectedAssets = {asset->id};
        }
    }

    void AssetManagerUI::Render() {
        if (ImGui::Begin(FormatString("%s %s", ICON_FA_FOLDER, Localization::GetString("ASSET_MANAGER").c_str()).c_str())) {
            if (!Workspace::IsProjectLoaded()) {
                ImGui::PushFont(Font::s_denseFont);
                ImGui::SetWindowFontScale(2.0f);
                    ImVec2 exclamationSize = ImGui::CalcTextSize(ICON_FA_TRIANGLE_EXCLAMATION);
                    ImGui::SetCursorPos(ImGui::GetWindowSize() / 2.0f - exclamationSize / 2.0f);
                    ImGui::Text(ICON_FA_TRIANGLE_EXCLAMATION);
                ImGui::SetWindowFontScale(1.0f);
                ImGui::PopFont();
                ImGui::End();
                return;
            }
            if (s_assetPreviewNameMap.empty()) {
                s_assetPreviewNameMap = {
                    {AssetPreviewType::Table, Localization::GetString("TABLE")},
                    {AssetPreviewType::Grid, Localization::GetString("GRID")},
                    {AssetPreviewType::List, Localization::GetString("LIST")}
                };
            }
            auto& project = Workspace::GetProject();
            auto& selectedAssets = project.selectedAssets;
            ImVec2 assetPreviewSize(100, 100);
            if (ImGui::BeginChild("##assetPreview", assetPreviewSize)) {
                if (!selectedAssets.empty()) {
                    auto assetCandidate = Workspace::GetAssetByAssetID(selectedAssets[0]);
                    if (assetCandidate.has_value()) {
                        auto& asset = assetCandidate.value();
                        auto textureCandidate = asset->GetPreviewTexture();
                        if (textureCandidate.has_value()) {
                            auto& texture = textureCandidate.value();
                            auto fitSize = FitRectInRect(assetPreviewSize, ImVec2(texture.width, texture.height));
                            ImGui::SetCursorPos(ImGui::GetWindowSize() / 2.0f - fitSize / 2.0f);
                            static bool imageHovered = false;
                            float factor = imageHovered ? 0.7f : 1.0f;
                            // auto imageCursor = ImGui::GetCursorPos();
                            ImGui::Image((ImTextureID) texture.handle, fitSize, ImVec2(0, 0), ImVec2(1, 1), ImVec4(factor));
                            imageHovered = ImGui::IsItemHovered();
                            ImGui::SetCursorPos({0, 0});
                            asset->RenderPreviewOverlay(glm::vec2(assetPreviewSize.x, assetPreviewSize.y));
                            if (imageHovered && ImGui::IsWindowFocused() && ImGui::GetIO().MouseClicked[ImGuiMouseButton_Left]) {
                                ImGui::OpenPopup("##assetPreviewTextureMaximized");
                            }
                            if (ImGui::BeginPopup("##assetPreviewTextureMaximized")) {
                                ImGui::Image((ImTextureID) texture.handle, FitRectInRect(ImGui::GetWindowViewport()->Size, ImVec2(texture.width, texture.height)) / 2);
                                ImGui::EndPopup();
                            }
                        }
                    }
                } else {
                    ImGui::PushFont(Font::s_denseFont);
                    ImGui::SetWindowFontScale(2.0f);
                        ImVec2 iconSize = ImGui::CalcTextSize(ICON_FA_QUESTION);
                        ImGui::SetCursorPos(ImGui::GetWindowSize() / 2.0f - iconSize / 2.0f);
                        ImGui::Text(ICON_FA_QUESTION);
                    ImGui::SetWindowFontScale(1.0f);
                    ImGui::PopFont();
                }
            }
            ImGui::EndChild();
            ImGui::SameLine(0, 8);
            if (ImGui::BeginChild("##assetDetailsInfo", ImVec2(ImGui::GetContentRegionAvail().x, assetPreviewSize.y))) {
                if (!selectedAssets.empty()) {
                    auto assetCandidate = Workspace::GetAssetByAssetID(selectedAssets[0]);
                    if (assetCandidate.has_value()) {
                        auto& asset = assetCandidate.value();
                        auto assetDescription = Assets::GetAssetImplementation(asset->packageName).value().description;
                        if (ImGui::Button(ICON_FA_COPY)) {
                            ImGui::SetClipboardText(std::to_string(asset->id).c_str());
                        }
                        ImGui::SetItemTooltip("%s %s", ICON_FA_COPY, Localization::GetString("COPY_ASSET_ID").c_str());
                        ImGui::SameLine(0, 3);
                        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                            ImGui::InputTextWithHint("##assetNameField", FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("ASSET_NAME").c_str()).c_str(), &asset->name);
                        ImGui::PopItemWidth();
                        asset->RenderDetails();
                    }
                } else {
                    std::string nothingToShowText = Localization::GetString("NOTHING_TO_SHOW");
                    ImVec2 textSize = ImGui::CalcTextSize(nothingToShowText.c_str());
                    ImGui::SetCursorPos(ImGui::GetWindowSize() / 2.0f - textSize / 2.0f);
                    ImGui::Text("%s", nothingToShowText.c_str());
                }
            }
            ImGui::EndChild();
            ImGui::Spacing();

            static std::string assetSearch = "";

            if (ImGui::Button(ICON_FA_FOLDER_PLUS)) {
                AutoImportAsset();
            }

            ImGui::SameLine(0, 3);

            if (ImGui::Button(ICON_FA_PLUS)) {
                ImGui::OpenPopup("##manualSelectAsset");
            }

            if (ImGui::BeginPopup("##manualSelectAsset")) {
                ImGui::SeparatorText(FormatString("%s %s", ICON_FA_LIST, Localization::GetString("SELECT_ASSET_TYPE").c_str()).c_str());
                for (auto& implementation : Assets::s_implementations) {
                    auto& description = implementation.description;
                    if (ImGui::MenuItem(FormatString("%s %s %s", ICON_FA_PLUS, description.icon.c_str(), description.prettyName.c_str()).c_str())) {
                        AutoImportAsset(description.packageName);
                    }
                }
                ImGui::EndPopup();
            }

            static AssetPreviewType s_previewType = AssetPreviewType::Table;
            if (!project.customData.contains("AssetPreviewType")) {
                project.customData["AssetPreviewType"] = (int) s_previewType;
            } else {
                s_previewType = (AssetPreviewType) project.customData["AssetPreviewType"].get<int>();
            }

            ImGui::SameLine(0, 3);
            if (ImGui::Button(s_assetPreviewIconMap[s_previewType])) {
                ImGui::OpenPopup("##assetPreviewTypeChooser");
            }

            if (ImGui::BeginPopup("##assetPreviewTypeChooser")) {
                ImGui::SeparatorText(FormatString("%s %s (%s)", s_assetPreviewIconMap[s_previewType], Localization::GetString("LISTING_TYPE").c_str(), s_assetPreviewNameMap[s_previewType].c_str()).c_str());

                if (ImGui::MenuItem(FormatString("%s %s", s_assetPreviewIconMap[AssetPreviewType::Table], s_assetPreviewNameMap[AssetPreviewType::Table].c_str()).c_str())) {
                    s_previewType = AssetPreviewType::Table;
                }

                if (ImGui::MenuItem(FormatString("%s %s", s_assetPreviewIconMap[AssetPreviewType::Grid], s_assetPreviewNameMap[AssetPreviewType::Grid].c_str()).c_str())) {
                    s_previewType = AssetPreviewType::Grid;
                }

                if (ImGui::MenuItem(FormatString("%s %s", s_assetPreviewIconMap[AssetPreviewType::List], s_assetPreviewNameMap[AssetPreviewType::List].c_str()).c_str())) {
                    s_previewType = AssetPreviewType::List;
                }

                ImGui::EndPopup();
            }

            project.customData["AssetPreviewType"] = (int) s_previewType;

            ImGui::SameLine(0, 3);
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputTextWithHint("##assetSearch", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &assetSearch);
            ImGui::PopItemWidth();

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
            bool assetsListBegin = ImGui::BeginChild("##assetsList", ImGui::GetContentRegionAvail());
            ImGui::PopStyleVar(2);
            if (assetsListBegin) {
                if (s_previewType == AssetPreviewType::List) {
                    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_Button));
                    if (ImGui::BeginChild("##assetsList", ImGui::GetContentRegionAvail())) {
                        for (auto& asset : project.assets) {
                            ImGui::PushID(asset->id);
                            bool isSelected = std::find(selectedAssets.begin(), selectedAssets.end(), asset->id) != selectedAssets.end();
                            static std::unordered_map<int, bool> s_hoveredMap;
                            if (s_hoveredMap.find(asset->id) == s_hoveredMap.end()) {
                                s_hoveredMap[asset->id] = false;
                            }
                            ImVec4 childBgColor = ImGui::GetStyleColorVec4(ImGuiCol_Button);
                            bool& isHovered = s_hoveredMap[asset->id];
                            if (isHovered) childBgColor = ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered);
                            if (isSelected) childBgColor = ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive);
                            ImGui::PushStyleColor(ImGuiCol_ChildBg, childBgColor);
                            if (ImGui::BeginChild("##assetChild", ImVec2(ImGui::GetContentRegionAvail().x, 0), ImGuiChildFlags_AutoResizeY)) {
                                auto textureCandidate = asset->GetPreviewTexture();
                                bool assetReady = asset->IsReady();
                                ImVec2 previewSize = ImVec2(50, 50);
                                if (textureCandidate.has_value() && assetReady) {
                                    auto& texture = textureCandidate.value();
                                    ImVec2 fitSize = FitRectInRect(ImVec2(50, 50), ImVec2(texture.width, texture.height));
                                    ImGui::SetCursorPosX(previewSize.x / 2.0f - fitSize.x / 2.0f);
                                    ImGui::SetCursorPosY(ImGui::GetWindowSize().y / 2.0f - fitSize.y / 2.0f);
                                    ImGui::Image((ImTextureID) texture.handle, fitSize);
                                } else {
                                    auto assetImplementationCandidate = Assets::GetAssetImplementation(asset->packageName);
                                    if (assetImplementationCandidate.has_value()) {
                                        auto& assetImplementation = assetImplementationCandidate.value();
                                        ImGui::PushFont(Font::s_denseFont);
                                        ImGui::SetWindowFontScale(previewSize.y / ImGui::GetFontSize());
                                        ImGui::Text("%s", assetReady ? assetImplementation.description.icon.c_str() : ICON_FA_SPINNER);
                                        ImGui::SetWindowFontScale(1.0f);
                                        ImGui::PopFont();
                                    }
                                }
                                ImGui::SameLine(0, 12);

                                ImGui::BeginGroup();
                                auto assetImplementationCandidate = Assets::GetAssetImplementation(asset->packageName);
                                if (assetImplementationCandidate.has_value()) {
                                    auto& assetImplementation = assetImplementationCandidate.value();
                                    auto durationCandidate = asset->GetDuration();
                                    auto resolutionCandidate = asset->GetResolution();
                                    auto sizeCandidate = asset->GetSize();
                                    auto pathCandidate = asset->GetPath();
                                    ImGui::Text("%s %s", assetImplementation.description.icon.c_str(), asset->name.c_str());
                                    ImGui::Text("%s %s | %s %s | %s %s | %s %s", ICON_FA_STOPWATCH, durationCandidate.value_or("-").c_str(), 
                                                                                ICON_FA_EXPAND, resolutionCandidate.value_or("-").c_str(), 
                                                                                ICON_FA_SCALE_BALANCED, sizeCandidate.has_value() ? ConvertToHRSize(sizeCandidate.value()).c_str() : "-", 
                                                                                ICON_FA_FOLDER_OPEN, pathCandidate.value_or("-").c_str());
                                    ImGui::SetItemTooltip("%s %s | %s %s | %s %s | %s %s", ICON_FA_STOPWATCH, durationCandidate.value_or("-").c_str(), 
                                                                                ICON_FA_EXPAND, resolutionCandidate.value_or("-").c_str(), 
                                                                                ICON_FA_SCALE_BALANCED, sizeCandidate.has_value() ? ConvertToHRSize(sizeCandidate.value()).c_str() : "-", 
                                                                                ICON_FA_FOLDER_OPEN, pathCandidate.value_or("-").c_str());
                                }  
                                ImGui::EndGroup();
                            }
                            ImGui::EndChild();
                            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                                ImGui::SetDragDropPayload(ASSET_MANAGER_DRAG_DROP_PAYLOAD, &asset->id, sizeof(asset->id));
                                asset->RenderDetails();
                                ImGui::EndDragDropSource();
                            }

                            if (ImGui::BeginDragDropTarget()) {
                                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ASSET_MANAGER_DRAG_DROP_PAYLOAD)) {
                                    int aID = *((int*) payload->Data);
                                    for (auto& testAsset : project.assets) {
                                        if (testAsset->id == aID) {
                                            auto& aAsset = testAsset;
                                            std::swap(aAsset, asset);
                                        }
                                    }
                                }
                                ImGui::EndDragDropTarget();
                            }
                            bool leftClick = isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Left);
                            bool rightClick = isHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right);
                            isHovered = ImGui::IsItemHovered(ImGuiHoveredFlags_RectOnly);
                            if (leftClick) {
                                if (ImGui::GetIO().KeyCtrl) {
                                    auto idIterator = std::find(selectedAssets.begin(), selectedAssets.end(), asset->id);
                                    if (idIterator == selectedAssets.end()) {
                                        selectedAssets.push_back(asset->id);
                                    } else {
                                        selectedAssets.erase(idIterator);
                                    }
                                } else {
                                    selectedAssets = {asset->id};
                                }
                            }

                            std::string popupID = FormatString("##assetPopup%i", asset->id);
                            if (rightClick) {
                                ImGui::OpenPopup(popupID.c_str());
                            }

                            if (ImGui::BeginPopup(popupID.c_str())) {
                                ImGui::SeparatorText(FormatString("%s %s", Assets::GetAssetImplementation(asset->packageName).value().description.icon.c_str(), asset->name.c_str()).c_str());
                                RenderAssetPopup(asset);
                                ImGui::EndPopup();
                            }
                            ImGui::Separator();
                            ImGui::PopStyleColor();
                            ImGui::PopID();
                        }
                    } 
                    ImGui::EndChild();
                    ImGui::PopStyleColor();
                }
                if (s_previewType == AssetPreviewType::Table) {
                    // asset table columns list
                    // 1 - name
                    // 2 - duration
                    // 3 - resolution
                    // 4 - size
                    // 5 - path
                    ImGuiTableFlags tableFlags = 0;
                    tableFlags |= ImGuiTableFlags_RowBg;
                    tableFlags |= ImGuiTableFlags_BordersV;
                    tableFlags |= ImGuiTableFlags_BordersOuterV;
                    tableFlags |= ImGuiTableFlags_BordersInnerV;
                    tableFlags |= ImGuiTableFlags_BordersH;
                    tableFlags |= ImGuiTableFlags_BordersOuterH;
                    tableFlags |= ImGuiTableFlags_BordersInnerH;
                    tableFlags |= ImGuiTableFlags_NoBordersInBody;
                    tableFlags |= ImGuiTableFlags_Hideable;
                    tableFlags |= ImGuiTableFlags_Reorderable;
                    tableFlags |= ImGuiTableFlags_Resizable;
                    tableFlags |= ImGuiTableFlags_ScrollX;
                    tableFlags |= ImGuiTableFlags_ScrollY;

                    if (ImGui::BeginTable("##assetsTable", 5, tableFlags)) {
                        ImGui::TableSetupColumn(FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("NAME").c_str()).c_str());
                        ImGui::TableSetupColumn(FormatString("%s %s", ICON_FA_STOPWATCH, Localization::GetString("DURATION").c_str()).c_str());
                        ImGui::TableSetupColumn(FormatString("%s %s", ICON_FA_EXPAND, Localization::GetString("RESOLUTION").c_str()).c_str());
                        ImGui::TableSetupColumn(FormatString("%s %s", ICON_FA_SCALE_BALANCED, Localization::GetString("SIZE").c_str()).c_str());
                        ImGui::TableSetupColumn(FormatString("%s %s", ICON_FA_FOLDER, Localization::GetString("PATH").c_str()).c_str());

                        ImGui::TableHeadersRow();

                        for (auto& asset : project.assets) {
                            ImGui::TableNextRow();

                            auto description = Assets::GetAssetImplementation(asset->packageName).value().description;

                            std::string assetIcon = asset->IsReady() ? description.icon : ICON_FA_SPINNER;
                            ImGui::TableSetColumnIndex(0);
                            if (ImGui::Selectable(FormatString("%s %s", assetIcon.c_str(), asset->name.c_str()).c_str(), std::find(selectedAssets.begin(), selectedAssets.end(), asset->id) != selectedAssets.end(), ImGuiSelectableFlags_SpanAllColumns)) {
                                if (ImGui::GetIO().KeyCtrl) {
                                    auto idIterator = std::find(selectedAssets.begin(), selectedAssets.end(), asset->id);
                                    if (idIterator == selectedAssets.end()) {
                                        selectedAssets.push_back(asset->id);
                                    } else {
                                        selectedAssets.erase(idIterator);
                                    }
                                } else {
                                    selectedAssets = {asset->id};
                                }
                                UIShared::s_lastClickedObjectType = LastClickedObjectType::Asset;
                            }

                            std::string popupID = FormatString("##assetPopup%i", asset->id);
                            if (ImGui::IsItemHovered() && ImGui::IsWindowFocused() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                                ImGui::OpenPopup(popupID.c_str());
                            }

                            if (ImGui::BeginPopup(popupID.c_str())) {
                                ImGui::SeparatorText(FormatString("%s %s", Assets::GetAssetImplementation(asset->packageName).value().description.icon.c_str(), asset->name.c_str()).c_str());
                                RenderAssetPopup(asset);
                                ImGui::EndPopup();
                            }


                            std::string renamePopupID = FormatString("##renameAsset%i", asset->id);
                            if (ImGui::IsItemHovered() && ImGui::GetIO().MouseDoubleClicked[ImGuiMouseButton_Left]) {
                                ImGui::OpenPopup(renamePopupID.c_str());
                            }

                            static bool renameFieldFocused = true;
                            if (ImGui::BeginPopup(renamePopupID.c_str())) {
                                if (!renameFieldFocused) {
                                    ImGui::SetKeyboardFocusHere(0);
                                    renameFieldFocused = true;
                                }
                                ImGui::InputTextWithHint("##renameField", FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("ASSET_NAME").c_str()).c_str(), &asset->name);
                                if (ImGui::IsKeyPressed(ImGuiKey_Enter)) ImGui::CloseCurrentPopup();
                                ImGui::EndPopup();
                            } else renameFieldFocused = false;

                            if (ImGui::BeginDragDropSource()) {
                                ImGui::SetDragDropPayload(ASSET_MANAGER_DRAG_DROP_PAYLOAD, &asset->id, sizeof(asset->id));
                                asset->RenderDetails();
                                ImGui::EndDragDropSource();
                            }

                            if (ImGui::BeginDragDropTarget()) {
                                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ASSET_MANAGER_DRAG_DROP_PAYLOAD)) {
                                    int aID = *((int*) payload->Data);
                                    for (auto& testAsset : project.assets) {
                                        if (testAsset->id == aID) {
                                            auto& aAsset = testAsset;
                                            std::swap(aAsset, asset);
                                        }
                                    }
                                }
                                ImGui::EndDragDropTarget();
                            }

                            ImGui::TableSetColumnIndex(1);
                            auto durationCandidate = asset->GetDuration();
                            if (durationCandidate.has_value()) {
                                auto& duration = durationCandidate.value();
                                ImGui::Text("%s", duration.c_str());
                            }

                            ImGui::TableSetColumnIndex(2);
                            auto resolutionCandidate = asset->GetResolution();
                            if (resolutionCandidate.has_value()) {
                                auto& resolution = resolutionCandidate.value();
                                ImGui::Text("%s", resolution.c_str());
                            }

                            ImGui::TableSetColumnIndex(3);
                            auto sizeCandidate = asset->GetSize();
                            if (sizeCandidate.has_value()) {
                                ImGui::Text("%s", ConvertToHRSize(sizeCandidate.value()).c_str());
                            }

                            ImGui::TableSetColumnIndex(4);
                            auto pathCandidate = asset->GetPath();
                            if (pathCandidate.has_value()) {
                                ImGui::Text("%s", pathCandidate.value().c_str());
                            }
                        }
                        ImGui::EndTable();
                    }
                } else if (s_previewType == AssetPreviewType::Grid) {

                } else if (s_previewType == AssetPreviewType::List) {
                    
                }
            }
            ImGui::EndChild();
        }

        if (Workspace::IsProjectLoaded()) {
            auto& project = Workspace::GetProject();
            for (auto& assetID : s_targetDeleteAssets) {
                auto indexCandidate = Workspace::GetAssetIndexByAssetID(assetID);
                if (indexCandidate.has_value()) {
                    auto& index = indexCandidate.value();
                    project.assets[index]->Delete();
                    project.assets.erase(project.assets.begin() + index);
                }
            }
        }
        ImGui::End();
    }
};