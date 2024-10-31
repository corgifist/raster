#include "common/ui_helpers.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_stdlib.h"
#include "common/audio_info.h"
#include "../ImGui/imgui_internal.h"
#include "common/dispatchers.h"

namespace Raster {

    static ImVec2 FitRectInRect(ImVec2 dst, ImVec2 src) {
        float scale = std::min(dst.x / src.x, dst.y / src.y);
        return ImVec2{src.x * scale, src.y * scale};
    }

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
                        ImGui::CloseCurrentPopup();
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
                                ImGui::CloseCurrentPopup();
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
                if (ImGui::BeginChild("##assetsChild", ImVec2(0, RASTER_PREFERRED_POPUP_HEIGHT), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AlwaysAutoResize)) {
                    if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_XMARK, Localization::GetString("NONE").c_str()).c_str())) {
                        t_assetID = -1;
                    }
                    bool hasCandidates = false;
                    for (auto& asset : project.assets) {
                        auto description = Assets::GetAssetImplementation(asset->packageName).value();
                        if (!s_assetFilter.empty() && LowerCase(asset->name).find(LowerCase(s_assetFilter)) == std::string::npos) continue;
                        hasCandidates = true;
                        if (ImGui::MenuItem(FormatString("%s %s", description.description.icon.c_str(), asset->name.c_str()).c_str())) {
                            t_assetID = asset->id;
                            ImGui::CloseCurrentPopup();
                        }
                    }
                    if (!hasCandidates) {
                        std::string nothingToShowText = Localization::GetString("NOTHING_TO_SHOW");
                        ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2 - ImGui::CalcTextSize(nothingToShowText.c_str()).x);
                        ImGui::Text("%s", nothingToShowText.c_str());
                    }
                }
                ImGui::EndChild();
                ImGui::SameLine();
                if (ImGui::BeginChild("##assetDetailsChild", ImVec2(0, RASTER_PREFERRED_POPUP_HEIGHT), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AlwaysAutoResize)) {
                    auto assetCandidate = Workspace::GetAssetByAssetID(t_assetID);
                    if (assetCandidate.has_value()) {
                        auto& asset = assetCandidate.value();
                        auto textureCandidate = asset->GetPreviewTexture();
                        if (textureCandidate.has_value()) {
                            auto& texture = textureCandidate.value();
                            std::any dynamicTexture = texture;
                            Dispatchers::DispatchString(dynamicTexture);
                        }
                        asset->RenderDetails();
                    } else {
                        ImGui::Text("%s", Localization::GetString("NOTHING_TO_SHOW").c_str());
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

    void UIHelpers::RenderAudioSamplesWaveform(AudioSamples& t_samples) {
#define WAVEFORM_PRECISION 100
        for (int channel = 0; channel < AudioInfo::s_channels; channel++) {
            ImGui::PushID(channel);
                std::vector<float> constructedWaveform(WAVEFORM_PRECISION);
                int waveformIndex = 0;
                for (int i = AudioInfo::s_globalAudioOffset + AudioInfo::s_channels - 1; i < AudioInfo::s_globalAudioOffset + WAVEFORM_PRECISION * AudioInfo::s_channels; i += AudioInfo::s_channels) {
                    constructedWaveform[waveformIndex++] = t_samples.samples->data()[i % AudioInfo::s_periodSize];
                }
                RenderDecibelScale(constructedWaveform[0]);
                ImGui::SameLine();
                ImGui::PlotLines("##waveform", constructedWaveform.data(), WAVEFORM_PRECISION, 0, FormatString("%s %s %i", ICON_FA_VOLUME_HIGH, Localization::GetString("AUDIO_CHANNEL").c_str(), channel).c_str(), -1, 1, {0, 80});
            ImGui::PopID();
        }
    }

    void UIHelpers::RenderRawAudioSamplesWaveform(std::vector<float>* t_raw) {
        if (!t_raw || t_raw->size() <= 0) {
            std::string warningMessage = FormatString("%s %s", ICON_FA_TRIANGLE_EXCLAMATION, Localization::GetString("WAVEFORM_PREVIEW_IS_NOT_AVAILABLE").c_str());
            ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - ImGui::CalcTextSize(warningMessage.c_str()).x / 2.0f);
            ImGui::Text("%s", warningMessage.c_str());
            return;
        }
        for (int channel = 0; channel < AudioInfo::s_channels; channel++) {
            ImGui::PushID(channel);
                std::vector<float> constructedWaveform(WAVEFORM_PRECISION);
                int waveformIndex = 0;
                for (int i = AudioInfo::s_globalAudioOffset + AudioInfo::s_channels - 1; i < AudioInfo::s_globalAudioOffset + WAVEFORM_PRECISION * AudioInfo::s_channels; i += AudioInfo::s_channels) {
                    constructedWaveform[waveformIndex++] = t_raw->data()[i % AudioInfo::s_periodSize];
                }
                RenderDecibelScale(constructedWaveform[0]);
                ImGui::SameLine();
                ImGui::PlotLines("##waveform", constructedWaveform.data(), WAVEFORM_PRECISION, 0, FormatString("%s %s %i", ICON_FA_VOLUME_HIGH, Localization::GetString("AUDIO_CHANNEL").c_str(), channel).c_str(), -1, 1, {0, 80});
            ImGui::PopID();
        }
    }

    void UIHelpers::RenderDecibelScale(float t_linear) {
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        float decibel = 60 - std::min(std::abs(LinearToDecibel(std::abs(t_linear))), 60.0f);
        // DUMP_VAR(decibel);
        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, IM_COL32(109, 130, 209, 255));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
        ImGui::PushItemWidth(10);
            ImGui::PlotHistogram("##decibelHistogram", &decibel, 1, 0, nullptr, 0.0f, 60, {0, 80});
        ImGui::PopItemWidth();
        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }

    void UIHelpers::RenderNothingToShowText() {
        std::string nothingToShowText = Localization::GetString("NOTHING_TO_SHOW");
        ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2 - ImGui::CalcTextSize(nothingToShowText.c_str()).x / 2);
        ImGui::Text("%s", nothingToShowText.c_str());
    }
};