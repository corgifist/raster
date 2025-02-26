#include "asset_manager.h"
#include "common/layouts.h"
#include "common/localization.h"
#include "common/ui_shared.h"
#include "font/IconsFontAwesome5.h"
#include "raster.h"

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

    static bool TextColorButton(const char* id, ImVec4 color) {
        if (ImGui::BeginChild(FormatString("##%scolorMark", id).c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0), ImGuiChildFlags_AutoResizeY)) {
            ImGui::SetCursorPos({0, 0});
            ImGui::PushStyleColor(ImGuiCol_Button, color);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color * 1.1f);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, color * 1.2f);
            ImGui::ColorButton(FormatString("%s %s", ICON_FA_DROPLET, id).c_str(), color, ImGuiColorEditFlags_AlphaPreview);
            ImGui::PopStyleColor(3);
            ImGui::SameLine();
            if (ImGui::IsWindowHovered()) ImGui::BeginDisabled();
            std::string defaultColorMarkText = "";
            if (Workspace::s_defaultColorMark == id) {
                defaultColorMarkText = FormatString(" (%s)", Localization::GetString("DEFAULT").c_str());
            }
            ImGui::Text("%s %s%s", ICON_FA_DROPLET, id, defaultColorMarkText.c_str());
            if (ImGui::IsWindowHovered()) ImGui::EndDisabled();
        }
        ImGui::EndChild();
        return ImGui::IsItemClicked();
    }

    static std::vector<int> s_targetDeleteAssets;

    void AssetManagerUI::RenderAssetPopup(AbstractAsset& t_asset) {
        auto& project = Workspace::GetProject();
        bool isFolder = t_asset->GetChildAssets().has_value();
        t_asset->RenderPopup();
        if (!isFolder && ImGui::MenuItem(FormatString("%s %s", ICON_FA_FOLDER_PLUS, Localization::GetString("REIMPORT_ASSET").c_str()).c_str())) {
            auto originalAsset = t_asset;
            auto assetCandidate = ImportAsset();
            if (assetCandidate.has_value()) {
                t_asset->Delete();
                t_asset = assetCandidate.value();
                t_asset->id = originalAsset->id;
            }
        }
        if (!isFolder && ImGui::MenuItem(FormatString("%s %s", ICON_FA_REPLY, Localization::GetString("REPLACE_WITH_PLACEHOLDER_ASSET").c_str()).c_str())) {
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
        if (!isFolder && ImGui::MenuItem(FormatString("%s %s", ICON_FA_FOLDER_OPEN, Localization::GetString("REVEAL_IN_FILE_EXPLORER").c_str()).c_str(), "Ctrl+R", nullptr, pathCandidate.has_value())) {
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
                    auto scopeCandidate = Workspace::GetAssetScopeByAssetID(assetID);
                    if (duplicateCandidate.has_value() && scopeCandidate) {
                        (*scopeCandidate)->push_back(duplicateCandidate.value());
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
                return Workspace::ImportAsset(path.get());
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

    Json AssetManagerUI::AbstractSerialize() {
        return {};
    }

    void AssetManagerUI::AbstractLoad(Json t_data) {
        
    }

    void AssetManagerUI::AbstractRender() {
        if (!open) {
            Layouts::DestroyWindow(id);
        }
        ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(FormatString("%s %s###%i", ICON_FA_FOLDER, Localization::GetString("ASSET_MANAGER").c_str(), id).c_str(), &open)) {
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

            static uint32_t s_colorMarkFilter = 0;
            if (!project.customData.contains("AssetColorMarkFilter")) {
                project.customData["AssetColorMarkFilter"] = s_colorMarkFilter;
            } else {
                s_colorMarkFilter = project.customData["AssetColorMarkFilter"];
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

            ImVec4 colorMark4 = ImGui::ColorConvertU32ToFloat4(s_colorMarkFilter);
            if (ImGui::ColorButton("##assetColorMarkFilterPicker", colorMark4, ImGuiColorEditFlags_AlphaPreview)) {
                ImGui::OpenPopup("##filterAssetsByColorMark");
            }
            if (ImGui::BeginPopup("##filterAssetsByColorMark")) {
                ImGui::SeparatorText(FormatString("%s %s", ICON_FA_FILTER, Localization::GetString("FILTER_BY_COLOR_MARK").c_str()).c_str());
                static std::string s_colorMarkNameFilter = "";
                ImGui::InputTextWithHint("##colorMarkNameFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &s_colorMarkNameFilter);
                if (ImGui::BeginChild("##colorMarkCandidates", ImVec2(0, RASTER_PREFERRED_POPUP_HEIGHT))) {
                    if (TextColorButton(Localization::GetString("NONE").c_str(), ImVec4(0))) {
                        s_colorMarkFilter = 0;
                    }
                    for (auto& colorMarkPair : Workspace::s_colorMarks) {
                        if (!s_colorMarkNameFilter.empty() && LowerCase(colorMarkPair.first).find(LowerCase(s_colorMarkNameFilter)) == std::string::npos) continue;
                        ImVec4 colorMarkPair4 = ImGui::ColorConvertU32ToFloat4(colorMarkPair.second);
                        if (TextColorButton(colorMarkPair.first.c_str(), colorMarkPair4)) {
                            s_colorMarkFilter = colorMarkPair.second;
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }
                ImGui::EndChild();
                ImGui::EndPopup();
            }

            ImGui::SameLine(0, 3);
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputTextWithHint("##assetSearch", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &assetSearch);
            ImGui::PopItemWidth();

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
            bool assetsListBegin = ImGui::BeginChild("##assetsList", ImGui::GetContentRegionAvail());
            ImGui::PopStyleVar(2);
            static int s_targetSwapA = -1;
            static int s_targetSwapB = -1;
            static std::vector<AbstractAsset>* s_targetChildAssetsScope = nullptr;
            static std::vector<AbstractAsset>* s_targetDuplicateScope = nullptr;
            static int s_targetDuplicateID = -1;
            if (assetsListBegin) {
                if (s_previewType == AssetPreviewType::List) {
                    if (ImGui::BeginChild("##assetsList", ImGui::GetContentRegionAvail())) {
                        std::function<void(std::vector<AbstractAsset>&)> renderAssets = [&](std::vector<AbstractAsset>& t_assets) {
                            for (auto& asset : t_assets) {
                                if (s_colorMarkFilter != 0 && asset->colorMark != s_colorMarkFilter) continue;
                                if (!assetSearch.empty() && LowerCase(asset->name).find(LowerCase(assetSearch)) == std::string::npos) continue;
                                ImGui::PushID(asset->id);
                                auto childAssetsCandidate = asset->GetChildAssets();
                                bool isSelected = std::find(selectedAssets.begin(), selectedAssets.end(), asset->id) != selectedAssets.end();
                                ImVec4 childBgColor = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
                                ImVec2 assetBeginCursor = ImGui::GetCursorPos();
                                ImVec2 assetChildSize = ImVec2(ImGui::GetContentRegionAvail().x, 50);
                                auto textureCandidate = asset->GetPreviewTexture();
                                bool assetReady = asset->IsReady();
                                static std::unordered_map<int, bool> s_hoveredMap;
                                if (s_hoveredMap.find(asset->id) == s_hoveredMap.end()) {
                                    s_hoveredMap[asset->id] = false;
                                }
                                bool& isHovered = s_hoveredMap[asset->id];
                                ImVec2 previewSize = ImVec2(50, 50);
                                ImGui::SetNextItemAllowOverlap();
                                ImGuiSelectableFlags selectableFlags = ImGuiSelectableFlags_AllowOverlap;
                                if (isHovered) {
                                    selectableFlags |= ImGuiSelectableFlags_Highlight;
                                }
                                ImGui::Selectable("##assetSelectable", false, selectableFlags, assetChildSize);
                                ImGui::SetCursorPos(assetBeginCursor);
                                ImGui::BeginGroup();
                                if (textureCandidate.has_value() && assetReady) {
                                    auto& texture = textureCandidate.value();
                                    ImVec2 fitSize = FitRectInRect(ImVec2(50, 50), ImVec2(texture.width, texture.height));
                                    ImGui::SetCursorPosX(assetBeginCursor.x + previewSize.x / 2.0f - fitSize.x / 2.0f);
                                    ImGui::SetCursorPosY(assetBeginCursor.y + previewSize.y / 2.0f - fitSize.y / 2.0f);
                                    ImGui::Image((ImTextureID) texture.handle, fitSize);
                                    ImGui::SameLine(0, 0);
                                    auto afterImageCursor = ImGui::GetCursorPos();
                                    ImGui::SetCursorPos(assetBeginCursor);
                                    ImGui::PushClipRect(ImGui::GetCursorScreenPos(), ImGui::GetCursorScreenPos() + previewSize, false);
                                    asset->RenderPreviewOverlay({previewSize.x, previewSize.y});
                                    ImGui::PopClipRect();
                                    ImGui::SetCursorPos(afterImageCursor);
                                } else {
                                    auto assetImplementationCandidate = Assets::GetAssetImplementation(asset->packageName);
                                    if (assetImplementationCandidate.has_value()) {
                                        auto& assetImplementation = assetImplementationCandidate.value();
                                        ImGui::PushFont(Font::s_denseFont);
                                        ImGui::SetWindowFontScale(previewSize.y / ImGui::GetFontSize());
                                        ImGui::Text("%s", assetReady ? assetImplementation.description.icon.c_str() : ICON_FA_SPINNER);
                                        ImGui::SameLine(0, 0);
                                        ImGui::SetWindowFontScale(1.0f);
                                        ImGui::PopFont();
                                        auto afterImageCursor = ImGui::GetCursorPos();
                                        ImGui::SetCursorPos(assetBeginCursor);
                                        ImGui::PushClipRect(ImGui::GetCursorScreenPos(), ImGui::GetCursorScreenPos() + previewSize, false);
                                        asset->RenderPreviewOverlay({previewSize.x, previewSize.y});
                                        ImGui::PopClipRect();
                                        ImGui::SetCursorPos(afterImageCursor);
                                    }
                                }
                                ImGui::NewLine();
                                ImGui::SameLine(0, 12);

                                ImGui::BeginGroup();
                                auto assetImplementationCandidate = Assets::GetAssetImplementation(asset->packageName);
                                bool assetsTreeOpen = false;
                                ImGuiID treeNodeID;
                                std::string treeNodeIDString;
                                if (assetImplementationCandidate.has_value()) {
                                    auto& assetImplementation = assetImplementationCandidate.value();
                                    auto durationCandidate = asset->GetDuration();
                                    auto resolutionCandidate = asset->GetResolution();
                                    auto sizeCandidate = asset->GetSize();
                                    auto pathCandidate = asset->GetPath();
                                    auto assetColorMark4 = ImGui::ColorConvertU32ToFloat4(asset->colorMark);
                                    if (ImGui::ColorButton(FormatString("%s %s", ICON_FA_DROPLET, Localization::GetString("COLOR_MARK").c_str()).c_str(), assetColorMark4, ImGuiColorEditFlags_AlphaPreview, ImVec2(16, 16))) {
                                        ImGui::OpenPopup("##assetColorMarkSelector");
                                    }

                                    if (ImGui::BeginPopup("##assetColorMarkSelector")) {
                                        ImGui::SeparatorText(FormatString("%s %s", ICON_FA_FILTER, Localization::GetString("FILTER_BY_COLOR_MARK").c_str()).c_str());
                                        static std::string s_colorMarkNameFilter = "";
                                        ImGui::InputTextWithHint("##colorMarkNameFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &s_colorMarkNameFilter);
                                        if (ImGui::BeginChild("##colorMarkCandidates", ImVec2(0, RASTER_PREFERRED_POPUP_HEIGHT))) {
                                            for (auto& colorMarkPair : Workspace::s_colorMarks) {
                                                if (!s_colorMarkNameFilter.empty() && LowerCase(colorMarkPair.first).find(LowerCase(s_colorMarkNameFilter)) == std::string::npos) continue;
                                                ImVec4 colorMarkPair4 = ImGui::ColorConvertU32ToFloat4(colorMarkPair.second);
                                                if (TextColorButton(colorMarkPair.first.c_str(), colorMarkPair4)) {
                                                    asset->colorMark = colorMarkPair.second;
                                                    ImGui::CloseCurrentPopup();
                                                }
                                            }
                                        }
                                        ImGui::EndChild();
                                        ImGui::EndPopup();
                                    }
                                    ImGui::SameLine();
                                    if (!childAssetsCandidate) ImGui::Text("%s %s", assetImplementation.description.icon.c_str(), asset->name.c_str()); 
                                    else {
                                        treeNodeIDString = FormatString("%s %s###%i", assetImplementation.description.icon.c_str(), asset->name.c_str(), asset->id);
                                        assetsTreeOpen = ImGui::TreeNodeEx(treeNodeIDString.c_str(), ImGuiTreeNodeFlags_NoTreePushOnOpen);
                                        treeNodeID = ImGui::GetItemID();
                                    }
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
                                ImGui::EndGroup();
                                auto afterChildCursor = ImGui::GetCursorPos();
                                ImGui::SetCursorPos(assetBeginCursor);
                                ImGui::SetNextItemAllowOverlap();
                                ImGui::InvisibleButton("##assetOverlap", assetChildSize);
                                ImGui::SetCursorPos(afterChildCursor);
                                bool leftClick = ImGui::IsItemClicked(ImGuiMouseButton_Left);
                                bool rightClick = ImGui::IsItemClicked(ImGuiMouseButton_Right);
                                isHovered = ImGui::IsItemHovered();
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
                                if (leftClick && childAssetsCandidate) {
                                    ImGui::TreeNodeSetOpen(treeNodeID, !assetsTreeOpen);
                                }
                                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                                    ImGui::SetDragDropPayload(ASSET_MANAGER_DRAG_DROP_PAYLOAD, &asset->id, sizeof(asset->id));
                                    asset->RenderDetails();
                                    ImGui::EndDragDropSource();
                                }

                                if (!childAssetsCandidate && ImGui::BeginDragDropTarget()) {
                                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ASSET_MANAGER_DRAG_DROP_PAYLOAD)) {
                                        int aID = *((int*) payload->Data);
                                        s_targetSwapA = asset->id;
                                        s_targetSwapB = aID;
                                    }
                                    ImGui::EndDragDropTarget();
                                }
                                if (childAssetsCandidate && ImGui::BeginDragDropTarget()) {
                                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ASSET_MANAGER_DRAG_DROP_PAYLOAD)) {
                                        int aID = *((int*) payload->Data);
                                        auto anotherAssetCandidate = Workspace::GetAssetByAssetID(aID);
                                        auto scopeCandidate = Workspace::GetAssetScopeByAssetID(aID);
                                        if (anotherAssetCandidate && scopeCandidate) {
                                            s_targetDuplicateScope = *scopeCandidate;
                                            s_targetDuplicateID = aID;
                                            s_targetChildAssetsScope = (*childAssetsCandidate);
                                        }
                                    }
                                    ImGui::EndDragDropTarget();
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

                                if (assetsTreeOpen) {
                                    ImGui::TreePush(treeNodeIDString.c_str());
                                    renderAssets(**childAssetsCandidate);
                                    ImGui::TreePop();
                                }
                                ImGui::PopID();
                            }
                        };
                        renderAssets(project.assets);
                    } 
                    ImGui::EndChild();
                    if (ImGui::BeginDragDropTarget()) {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ASSET_MANAGER_DRAG_DROP_PAYLOAD)) {
                            int aID = *((int*) payload->Data);
                            auto anotherAssetCandidate = Workspace::GetAssetByAssetID(aID);
                            auto scopeCandidate = Workspace::GetAssetScopeByAssetID(aID);
                            if (anotherAssetCandidate && scopeCandidate) {
                                s_targetDuplicateScope = *scopeCandidate;
                                s_targetDuplicateID = aID;
                                s_targetChildAssetsScope = &project.assets;
                            }
                        }
                        ImGui::EndDragDropTarget();
                    }
                }
                if (s_previewType == AssetPreviewType::Table) {
                    // asset table columns list
                    // 0 - color mark
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

                    if (ImGui::BeginChild("##tableContainer", ImGui::GetContentRegionAvail())) {
                        if (ImGui::BeginTable("##assetsTable", 5, tableFlags)) {
                            ImGui::TableSetupColumn(FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("NAME").c_str()).c_str());
                            ImGui::TableSetupColumn(FormatString("%s %s", ICON_FA_STOPWATCH, Localization::GetString("DURATION").c_str()).c_str());
                            ImGui::TableSetupColumn(FormatString("%s %s", ICON_FA_EXPAND, Localization::GetString("RESOLUTION").c_str()).c_str());
                            ImGui::TableSetupColumn(FormatString("%s %s", ICON_FA_SCALE_BALANCED, Localization::GetString("SIZE").c_str()).c_str());
                            ImGui::TableSetupColumn(FormatString("%s %s", ICON_FA_FOLDER, Localization::GetString("PATH").c_str()).c_str());

                            ImGui::TableHeadersRow();

                            std::function<void(std::vector<AbstractAsset>& t_assets)> renderAssets = [&](std::vector<AbstractAsset>& t_assets) {
                                for (auto& asset : t_assets) {
                                    if (s_colorMarkFilter != 0 && s_colorMarkFilter != asset->colorMark) continue;
                                    if (!assetSearch.empty() && LowerCase(asset->name).find(LowerCase(assetSearch)) == std::string::npos) continue;
                                    ImGui::PushID(asset->id);
                                    ImGui::TableNextRow();

                                    auto description = Assets::GetAssetImplementation(asset->packageName).value().description;
                                    auto childAssetsCandidate = asset->GetChildAssets();
                                    ImGui::TableNextColumn();
                                    auto assetColorMark4 = ImGui::ColorConvertU32ToFloat4(asset->colorMark);
                                    if (ImGui::ColorButton(FormatString("%s %s", ICON_FA_DROPLET, Localization::GetString("COLOR_MARK").c_str()).c_str(), assetColorMark4, ImGuiColorEditFlags_AlphaPreview, ImVec2(16, 16))) {
                                        ImGui::OpenPopup("##assetColorMarkSelector");
                                    }

                                    if (ImGui::BeginPopup("##assetColorMarkSelector")) {
                                        ImGui::SeparatorText(FormatString("%s %s", ICON_FA_FILTER, Localization::GetString("FILTER_BY_COLOR_MARK").c_str()).c_str());
                                        static std::string s_colorMarkNameFilter = "";
                                        ImGui::InputTextWithHint("##colorMarkNameFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &s_colorMarkNameFilter);
                                        if (ImGui::BeginChild("##colorMarkCandidates", ImVec2(0, RASTER_PREFERRED_POPUP_HEIGHT))) {
                                            for (auto& colorMarkPair : Workspace::s_colorMarks) {
                                                if (!s_colorMarkNameFilter.empty() && LowerCase(colorMarkPair.first).find(LowerCase(s_colorMarkNameFilter)) == std::string::npos) continue;
                                                ImVec4 colorMarkPair4 = ImGui::ColorConvertU32ToFloat4(colorMarkPair.second);
                                                if (TextColorButton(colorMarkPair.first.c_str(), colorMarkPair4)) {
                                                    asset->colorMark = colorMarkPair.second;
                                                    ImGui::CloseCurrentPopup();
                                                }
                                            }
                                        }
                                        ImGui::EndChild();
                                        ImGui::EndPopup();
                                    }

                                    ImGui::SameLine();
                                    std::string assetIcon = asset->IsReady() ? description.icon : ICON_FA_SPINNER;
                                    if (!childAssetsCandidate && ImGui::Selectable(FormatString("%s %s", assetIcon.c_str(), asset->name.c_str()).c_str(), std::find(selectedAssets.begin(), selectedAssets.end(), asset->id) != selectedAssets.end(), ImGuiSelectableFlags_SpanAllColumns)) {
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
                                    if (!childAssetsCandidate) {
                                        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                                            ImGui::SetDragDropPayload(ASSET_MANAGER_DRAG_DROP_PAYLOAD, &asset->id, sizeof(asset->id));
                                            asset->RenderDetails();
                                            ImGui::EndDragDropSource();
                                        }
                                    }

                                    if (childAssetsCandidate) {
                                        bool childAssetsTreeOpen = ImGui::TreeNodeEx(FormatString("%s %s", assetIcon.c_str(), asset->name.c_str()).c_str(), ImGuiTreeNodeFlags_SpanAllColumns);
                                        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                                            ImGui::SetDragDropPayload(ASSET_MANAGER_DRAG_DROP_PAYLOAD, &asset->id, sizeof(asset->id));
                                            ImGui::EndDragDropSource();
                                        }
                                        if (ImGui::BeginDragDropTarget()) {
                                            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ASSET_MANAGER_DRAG_DROP_PAYLOAD)) {
                                                int aID = *((int*) payload->Data);
                                                auto anotherAssetCandidate = Workspace::GetAssetByAssetID(aID);
                                                auto scopeCandidate = Workspace::GetAssetScopeByAssetID(aID);
                                                if (anotherAssetCandidate && scopeCandidate) {
                                                    s_targetDuplicateScope = *scopeCandidate;
                                                    s_targetDuplicateID = aID;
                                                    s_targetChildAssetsScope = (*childAssetsCandidate);
                                                }
                                            }
                                            ImGui::EndDragDropTarget();
                                        }
                                        if (ImGui::IsItemClicked()) {
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
                                        if (childAssetsTreeOpen) {
                                            renderAssets(*(*childAssetsCandidate));
                                            ImGui::TreePop();
                                        }
                                    }

                                    if (!childAssetsCandidate && ImGui::BeginDragDropTarget()) {
                                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ASSET_MANAGER_DRAG_DROP_PAYLOAD)) {
                                            int aID = *((int*) payload->Data);
                                            s_targetSwapA = asset->id;
                                            s_targetSwapB = aID;
                                        }
                                        ImGui::EndDragDropTarget();
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

                                    ImGui::TableNextColumn();
                                    auto durationCandidate = asset->GetDuration();
                                    if (durationCandidate.has_value()) {
                                        auto& duration = durationCandidate.value();
                                        ImGui::Text("%s", duration.c_str());
                                    }

                                    ImGui::TableNextColumn();
                                    auto resolutionCandidate = asset->GetResolution();
                                    if (resolutionCandidate.has_value()) {
                                        auto& resolution = resolutionCandidate.value();
                                        ImGui::Text("%s", resolution.c_str());
                                    }

                                    ImGui::TableNextColumn();
                                    auto sizeCandidate = asset->GetSize();
                                    if (sizeCandidate.has_value()) {
                                        ImGui::Text("%s", ConvertToHRSize(sizeCandidate.value()).c_str());
                                    }

                                    ImGui::TableNextColumn();
                                    auto pathCandidate = asset->GetPath();
                                    if (pathCandidate.has_value()) {
                                        ImGui::Text("%s", pathCandidate.value().c_str());
                                    }

                                    ImGui::PopID();
                                } 
                            };
                            renderAssets(project.assets);
                            ImGui::EndTable();
                        }
                    }
                    ImGui::EndChild();
                    if (ImGui::BeginDragDropTarget()) {
                        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ASSET_MANAGER_DRAG_DROP_PAYLOAD)) {
                            int aID = *((int*) payload->Data);
                            auto anotherAssetCandidate = Workspace::GetAssetByAssetID(aID);
                            auto scopeCandidate = Workspace::GetAssetScopeByAssetID(aID);
                            if (anotherAssetCandidate && scopeCandidate) {
                                s_targetDuplicateScope = *scopeCandidate;
                                s_targetDuplicateID = aID;
                                s_targetChildAssetsScope = &project.assets;
                            }
                        }
                        ImGui::EndDragDropTarget();
                    }

                } else if (s_previewType == AssetPreviewType::Grid) {

                }
                if (s_targetDuplicateID > 0) {
                    int assetRemoveIndex = 0;
                    for (auto& asset : *s_targetDuplicateScope) {
                        if (asset->id == s_targetDuplicateID) break;
                        assetRemoveIndex++;
                    }
                    auto duplicateAssetCandidate = Workspace::GetAssetByAssetID(s_targetDuplicateID);
                    if (duplicateAssetCandidate && s_targetChildAssetsScope) {
                        s_targetChildAssetsScope->push_back(*duplicateAssetCandidate);
                    }
                    s_targetDuplicateScope->erase(s_targetDuplicateScope->begin() + assetRemoveIndex);
                    s_targetDuplicateID = -1;
                }
                if (s_targetSwapA > 0 && s_targetSwapB > 0) {
                    auto aCandidate = Workspace::GetAssetByAssetID(s_targetSwapA);
                    auto bCandidate = Workspace::GetAssetByAssetID(s_targetSwapB);
                    auto aScopeCandidate = Workspace::GetAssetScopeByAssetID(s_targetSwapA);
                    auto bScopeCandidate = Workspace::GetAssetScopeByAssetID(s_targetSwapB);
                    if (aCandidate && bCandidate && aScopeCandidate && bScopeCandidate) {
                        // DUMP_VAR((*aScopeCandidate)->size());
                        // DUMP_VAR((*bScopeCandidate)->size());
                        bool swappedSuccessfully = false;
                        for (auto& aAsset : **aScopeCandidate) {
                            if (swappedSuccessfully) break;
                            for (auto& bAsset : **bScopeCandidate) {
                                if ((aAsset->id == s_targetSwapA && bAsset->id == s_targetSwapB) || (aAsset->id == s_targetSwapB && bAsset->id == s_targetSwapA)) {
                                    auto x = aAsset;
                                    aAsset = bAsset;
                                    bAsset = x;
                                    swappedSuccessfully = true;
                                    break;
                                }
                            }
                        }
                    }
                    s_targetSwapA = s_targetSwapB = -1;
                }
                project.customData["AssetColorMarkFilter"] = s_colorMarkFilter;
            }
            ImGui::EndChild();
        }

        if (Workspace::IsProjectLoaded()) {
            auto& project = Workspace::GetProject();

            std::function<void(std::vector<AbstractAsset>&, int)> deleteAsset = [&](std::vector<AbstractAsset>& t_assets, int t_assetID) {
                int assetIndex = 0;
                int assetRemoveTarget = -1;
                for (auto& asset : t_assets) {
                    assetIndex++;
                    if (asset->id == t_assetID) {
                        asset->Delete();
                        assetRemoveTarget = assetIndex - 1;
                        continue;
                    }
                    auto childAssetsCandidate = asset->GetChildAssets();
                    if (childAssetsCandidate) {
                        auto& childAssets = *childAssetsCandidate;
                        deleteAsset(*childAssets, t_assetID);
                    }
                }
                if (assetRemoveTarget > 0) {
                    t_assets.erase(t_assets.begin() + assetRemoveTarget);
                }
            };
            for (auto& assetID : s_targetDeleteAssets) {
                deleteAsset(project.assets, assetID);
            }
        }
        ImGui::End();
    }
};