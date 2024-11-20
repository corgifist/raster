#include "common/ui_helpers.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_stdlib.h"
#include "common/audio_info.h"
#include "../ImGui/imgui_internal.h"
#include "common/dispatchers.h"
#include "../ImGui/imgui_drag.h"

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
                static std::unordered_map<ImGuiID, std::string> s_allowedAssetTypes;
                ImGuiID metadataID = ImGui::GetID("##allowedAssetTypes");
                if (s_allowedAssetTypes.find(metadataID) == s_allowedAssetTypes.end()) {
                    s_allowedAssetTypes[metadataID] = "";
                }
                auto& allowedAssetType = s_allowedAssetTypes[metadataID];
                ImVec4 buttonColor = ImGui::GetStyleColorVec4(ImGuiCol_Button);
                if (!allowedAssetType.empty()) buttonColor.w *= 0.4f;
                ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
                if (ImGui::Button(allowedAssetType.empty() ? ICON_FA_FILTER " " ICON_FA_XMARK : ICON_FA_FILTER " " ICON_FA_CHECK)) {
                    ImGui::OpenPopup("##assetFilterSpecifier");
                }
                ImGui::SameLine();
                ImGui::PopStyleColor();
                if (ImGui::BeginPopup("##assetFilterSpecifier")) {
                    ImGui::SeparatorText(FormatString("%s %s", ICON_FA_FILTER, Localization::GetString("FILTER_ASSETS_BY_TYPE").c_str()).c_str());
                    if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_XMARK, Localization::GetString("NONE").c_str()).c_str())) {
                        allowedAssetType = "";
                        ImGui::CloseCurrentPopup();
                    }
                    for (auto& implementation : Assets::s_implementations) {
                        if (ImGui::MenuItem(FormatString("%s%s %s", implementation.description.icon.c_str(), allowedAssetType == implementation.description.packageName ? ICON_FA_CHECK " " : "", implementation.description.prettyName.c_str()).c_str())) {
                            allowedAssetType = implementation.description.packageName;
                            ImGui::CloseCurrentPopup();
                        }
                    }
                    ImGui::EndPopup();
                }
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
                        if (!allowedAssetType.empty() && allowedAssetType != asset->packageName) continue;
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
        if (!t_samples.samples || t_samples.samples->size() <= 0) {
            std::string waveformWarningText = FormatString("%s %s", ICON_FA_TRIANGLE_EXCLAMATION, Localization::GetString("WAVEFORM_PREVIEW_IS_NOT_AVAILABLE").c_str());
            ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - ImGui::CalcTextSize(waveformWarningText.c_str()).x / 2.0f);
            ImGui::Text("%s", waveformWarningText.c_str());
            return;
        }
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

    void UIHelpers::RenderGradient1D(Gradient1D& t_gradient, float t_width, float t_height, float t_alpha) {
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        if (t_width == 0.0f) t_width = 240;
        if (t_height == 0.0f) t_height = 40;

        ImRect bb(cursorPos, cursorPos + ImVec2(t_width, t_height));
        ImGui::ItemSize(bb);
        if (!ImGui::ItemAdd(bb, 0, 0)) return;
        if (t_gradient.stops.size() < 2) return;
        ImGui::RenderFrameBorder(cursorPos, cursorPos + ImVec2(t_width, t_height));

        if (ImGui::IsMouseHoveringRect(bb.Min, bb.Max) && ImGui::IsWindowFocused()) {
            auto mouseCoord = ImGui::GetMousePos();
            float mouseDifference = mouseCoord.x - bb.Min.x;
            glm::vec4 color = t_gradient.Get(mouseDifference / t_width);
            if (ImGui::BeginTooltip()) {
                ImGui::PushItemWidth(200);
                    ImGui::ColorPicker4("##colorPreview", glm::value_ptr(color), ImGuiColorEditFlags_PickerHueWheel | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoAlpha);
                ImGui::PopItemWidth();
                ImGui::EndTooltip();
            }
        }

        float lowestPercentage = FLT_MAX;
        float biggestPercentage = FLT_MIN;
        for (int i = 1; i < t_gradient.stops.size(); i++) {
            GradientStop1D& previousStop = t_gradient.stops[i - 1];
            GradientStop1D& currentStop = t_gradient.stops[i];
            if (previousStop.percentage < lowestPercentage) {
                lowestPercentage = previousStop.percentage;
            }
            if (previousStop.percentage > biggestPercentage) {
                biggestPercentage = previousStop.percentage;
            }
            if (currentStop.percentage < lowestPercentage) {
                lowestPercentage = currentStop.percentage;
            }
            if (currentStop.percentage > biggestPercentage) {
                biggestPercentage = currentStop.percentage;
            }

            ImVec2 UL = cursorPos;
            UL.x += previousStop.percentage * t_width;
            
            ImVec2 BR = cursorPos;
            BR.x += currentStop.percentage * t_width;
            BR.y += t_height;

            
            ImU32 previousColor = ImGui::GetColorU32(ImVec4(previousStop.color.r, previousStop.color.g, previousStop.color.b, previousStop.color.a * t_alpha));
            ImU32 currentColor = ImGui::GetColorU32(ImVec4(currentStop.color.r, currentStop.color.g, currentStop.color.b, currentStop.color.a * t_alpha));
            ImGui::GetWindowDrawList()->AddRectFilledMultiColor(UL, BR, previousColor, currentColor, currentColor, previousColor);
        }

        if (lowestPercentage != 0.0) {
            auto& firstStop = t_gradient.stops[0];
            ImVec2 UL = cursorPos;
            ImVec2 BR = cursorPos;
            BR.x += lowestPercentage * t_width;
            BR.y += t_height;

            ImU32 firstColor = ImGui::GetColorU32(ImVec4(firstStop.color.r, firstStop.color.g, firstStop.color.b, firstStop.color.a * t_alpha));
            ImGui::GetWindowDrawList()->AddRectFilled(UL, BR, firstColor);
        }

        if (biggestPercentage != 1.0f) {
            auto& lastStop = t_gradient.stops[t_gradient.stops.size() - 1];
            ImVec2 UL = cursorPos;
            UL.x += biggestPercentage * t_width;

            ImVec2 BR = cursorPos;
            BR.x += t_width;
            BR.y += t_height;

            ImU32 lastColor = ImGui::GetColorU32(ImVec4(lastStop.color.r, lastStop.color.g, lastStop.color.b, lastStop.color.a * t_alpha));
            ImGui::GetWindowDrawList()->AddRectFilled(UL, BR, lastColor);
        }

        // ImGui::RenderFrame(cursorPos, cursorPos + ImVec2(t_width, t_height), ImGui::GetColorU32(ImGui::GetStyleColorVec4(ImGuiCol_PopupBg)));
    }

    struct Gradient1DAttributeContext {
        std::vector<DragStructure> stopDrags;
        Gradient1DAttributeContext() {

        }
    };

#define GRADIENT_ATTRIBUTE_PAYLOAD "GRADIENT_ATTRIBUTE_PAYLOAD"

    static bool CenteredButton(const char* label, float alignment) {
        ImGuiStyle &style = ImGui::GetStyle();

        float size = ImGui::CalcTextSize(label).x + style.FramePadding.x * 2.0f;
        float avail = ImGui::GetContentRegionAvail().x;

        float off = (avail - size) * alignment;
        if (off > 0.0f)
            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

        return ImGui::Button(label);
    }

    bool UIHelpers::RenderGradient1DEditor(Gradient1D& value, float t_width, float t_height) {
        std::string attributeID = std::to_string(ImGui::GetID("##gradientEditorContextID"));
        static std::unordered_map<std::string, Gradient1DAttributeContext> s_contexts;
        if (s_contexts.find(attributeID) == s_contexts.end()) {
            s_contexts[attributeID] = Gradient1DAttributeContext();
        }

        auto& gradientContext = s_contexts[attributeID];

        float gradientWidth = t_width > 0 ? t_width : ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x;
        float gradientHeight = t_height > 0 ? t_height : 45;
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        ImVec2 gradientUL = cursorPos;
        ImVec2 gradientBL = cursorPos;
        gradientBL.x += gradientWidth;
        gradientBL.y += gradientHeight;

        UIHelpers::RenderGradient1D(value, gradientWidth, gradientHeight);
        if (gradientContext.stopDrags.size() != value.stops.size()) {
            gradientContext.stopDrags.resize(value.stops.size());
        }

        bool gradientMustBeResorted = false;
        bool gradientWasEdited = true;

        ImVec2 dragTrackCursor = ImGui::GetCursorPos();
        ImVec2 dragSize = ImVec2(20, 20);
        for (int i = 0; i < value.stops.size(); i++) {
            auto& stop = value.stops[i];
            ImGui::PushID(stop.id);
            auto& drag = gradientContext.stopDrags[i];

            ImGui::SetCursorPosY(dragTrackCursor.y);
            ImGui::SetCursorPosX(dragTrackCursor.x + gradientWidth * stop.percentage - dragSize.x / 2.0f);
            if (ImGui::ColorButton("##dragTrackColorButton", ImVec4(stop.color.x, stop.color.y, stop.color.z, stop.color.w), ImGuiColorEditFlags_AlphaPreview, dragSize)) {
                ImGui::OpenPopup("##dragTrackPopup");
            }
            if (ImGui::IsItemHovered() && ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowFocused()) {
                drag.Activate();
            }
            if ((ImGui::IsItemHovered() || drag.isActive) && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                float deltaX = ImGui::GetIO().MouseDelta.x;
                stop.percentage += deltaX / gradientWidth;
                stop.percentage = glm::clamp(stop.percentage, 0.0f, 1.0f);
                gradientMustBeResorted = true;
            } else drag.Deactivate();

            if (ImGui::BeginPopup("##dragTrackPopup")) {
                ImGui::ColorPicker4("##dragTrackColorEdit", glm::value_ptr(stop.color));
                ImGui::EndPopup();
            }

            ImGui::PopID();
        }

        if (ImGui::IsMouseHoveringRect(gradientUL, gradientBL) && ImGui::IsWindowFocused()) {
            ImVec2 mouseCoord = ImGui::GetMousePos();
            float mouseDifference = (mouseCoord.x - gradientUL.x) / gradientWidth;
            float highlightWidth = 10;

            ImVec2 highlightUL = cursorPos;
            highlightUL.x += mouseDifference * gradientWidth - highlightWidth / 2.0f;
        
            ImVec2 highlightBR = cursorPos;
            highlightBR.x = highlightUL.x + highlightWidth / 2.0f;
            highlightBR.y += gradientHeight;

            auto color = value.Get(mouseDifference);
            auto reservedColor = color;
            color.r = 1.0f - color.r;
            color.g = 1.0f - color.g;
            color.b = 1.0f - color.b;
            ImGui::GetWindowDrawList()->AddRectFilled(highlightUL, highlightBR, ImGui::GetColorU32(ImVec4(color.r, color.g, color.b, color.a)));

            bool stopExists = false;
            for (auto& stop : value.stops) {
                if (stop.percentage == mouseDifference) {
                    stopExists = true;
                    break;
                }
            }
            if (!stopExists) {
                ImGui::SetCursorPosY(dragTrackCursor.y);
                ImGui::SetCursorPosX(dragTrackCursor.x + gradientWidth * mouseDifference - dragSize.x / 2.0f);
                ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
                ImGui::Button("+", dragSize);
                ImGui::PopStyleVar();
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
                    value.AddStop(mouseDifference, reservedColor);
                    gradientMustBeResorted = true;
                }
            }
        }

        int stopDeleteIndex = -1;
        for (int i = 0; i < value.stops.size(); i++) {
            auto& stop = value.stops[i];
            ImGui::PushID(stop.id);
                ImGui::PushID(i);
                if (i == 0) ImGui::BeginDisabled();
                if (ImGui::Button(ICON_FA_TRASH_CAN)) {
                    stopDeleteIndex = i;
                }
                if (i == 0) ImGui::EndDisabled();
                ImGui::PopID();
                ImGui::SameLine();
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s %i", ICON_FA_STOPWATCH, i);
                if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                    ImGui::SetDragDropPayload(GRADIENT_ATTRIBUTE_PAYLOAD, &i, sizeof(int));
                    ImGui::EndDragDropSource();
                }
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(GRADIENT_ATTRIBUTE_PAYLOAD)) {
                        int fromIndex = *((int*) payload->Data);
                        int toIndex = i;
                        std::swap(value.stops[fromIndex], value.stops[toIndex]);
                    }
                    ImGui::EndDragDropTarget();
                }
                ImGui::SameLine();
                ImGui::PushItemWidth(100);
                    ImGui::SliderFloat("##gradientStopPercentage", &stop.percentage, 0, 1, "%0.2f");
                    if (ImGui::IsItemEdited()) {
                        gradientMustBeResorted = true;
                    }
                ImGui::PopItemWidth();
                ImGui::SameLine();
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x);
                    glm::vec4 originalColor = stop.color;
                    ImGui::ColorEdit4("##gradientStopColor", glm::value_ptr(stop.color));
                    if (originalColor != stop.color) {
                        gradientWasEdited = true;
                    }
                ImGui::PopItemWidth();
            ImGui::PopID();
        }

        if (gradientMustBeResorted) {
            value.SortStops();
            gradientWasEdited = true;
        }

        static float s_newStopPercentage = 1.0f;
        static glm::vec4 s_newStopColor = glm::vec4(1);

        if (CenteredButton(FormatString("%s %s", ICON_FA_PLUS, Localization::GetString("NEW_GRADIENT_STOP").c_str()).c_str(), 0.5f)) {
            s_newStopPercentage = 1.0f;
            s_newStopColor = glm::vec4(1);
            ImGui::OpenPopup("##newGradientStopPopup");
        }

        if (ImGui::BeginPopup("##newGradientStopPopup")) {
            ImGui::PushItemWidth(100);
                ImGui::SliderFloat("##newGradientPercentage", &s_newStopPercentage, 0, 1, "%0.2f");
            ImGui::PopItemWidth();
            ImGui::SameLine();
            ImGui::PushItemWidth(400);
                ImGui::ColorEdit4("##gradientStopColor", glm::value_ptr(s_newStopColor));
            ImGui::PopItemWidth();
            ImGui::SameLine();
            if (ImGui::Button(Localization::GetString("OK").c_str()) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                value.AddStop(s_newStopPercentage, s_newStopColor);
                value.SortStops();
                gradientMustBeResorted = true;
                gradientWasEdited = true;
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (stopDeleteIndex > 0) {
            value.stops.erase(value.stops.begin() + stopDeleteIndex);
            gradientMustBeResorted = true;
            gradientWasEdited = true;
            value.SortStops();
        }

        return gradientWasEdited;
    }
};