#include "audio_buses.h"
#include "common/ui_helpers.h"
#include "common/audio_info.h"
#include "../../ImGui/imgui_stdlib.h"

namespace Raster {


/*     if (ImGui::Button(FormatString("%s %s", bus.main ? ICON_FA_CHECK : ICON_FA_XMARK, Localization::GetString("MAIN").c_str()).c_str())) {
        bus.main = !bus.main;
        for (auto& iterableBus : project.audioBuses) {
            if (iterableBus.id == bus.id) continue;
            iterableBus.main = false;
        }
    } */

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

    void AudioBusesUI::Render() {
        if (ImGui::Begin(FormatString("%s %s", ICON_FA_VOLUME_HIGH, Localization::GetString("AUDIO_BUSES").c_str()).c_str())) {
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

            auto& project = Workspace::GetProject();
            int mainAudioBusID = 0;
            for (auto& bus : project.audioBuses) {
                if (bus.main) {
                    mainAudioBusID = bus.id;
                    break;
                }
            }

            static uint32_t s_colorMarkFilter = IM_COL32(0, 0, 0, 0);
            static std::string s_audioBusFilter = "";

            static std::string s_newAudioBusName = "";
            if (ImGui::Button(ICON_FA_PLUS)) {
                ImGui::OpenPopup("##createNewAudioBus");
                s_newAudioBusName = Localization::GetString("NEW_AUDIO_BUS");
            }
            if (ImGui::BeginPopup("##createNewAudioBus")) {
                ImGui::InputTextWithHint("##newAudioBusName", FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("NEW_AUDIO_BUS_NAME").c_str()).c_str(), &s_newAudioBusName);
                ImGui::SameLine();
                if (ImGui::Button(FormatString("%s %s", ICON_FA_CHECK, Localization::GetString("OK").c_str()).c_str()) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                    project.audioBusesMutex->lock();
                    AudioBus newAudioBus;
                    newAudioBus.name = s_newAudioBusName;
                    newAudioBus.redirectID = mainAudioBusID;
                    project.audioBuses.push_back(newAudioBus);
                    ImGui::CloseCurrentPopup();
                    project.audioBusesMutex->unlock();
                }
                ImGui::EndPopup();
            }

            auto filter4 = ImGui::ColorConvertU32ToFloat4(s_colorMarkFilter);
            ImGui::SameLine();

            if (ImGui::ColorButton(FormatString("%s %s", ICON_FA_DROPLET, Localization::GetString("COLOR_MARK_FILTER").c_str()).c_str(), filter4, ImGuiColorEditFlags_AlphaPreview)) {
                ImGui::OpenPopup("##filterByColorMark");
            }

            if (ImGui::BeginPopup("##filterByColorMark")) {
                ImGui::SeparatorText(FormatString("%s %s", ICON_FA_FILTER, Localization::GetString("FILTER_BY_COLOR_MARK").c_str()).c_str());
                static std::string s_colorMarkNameFilter = "";
                ImGui::InputTextWithHint("##colorMarkFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &s_colorMarkNameFilter);
                if (ImGui::BeginChild("##filterColorMarkCandidates", ImVec2(ImGui::GetContentRegionAvail().x, 220))) {
                    if (TextColorButton(Localization::GetString("NO_FILTER").c_str(), IM_COL32(0, 0, 0, 0))) {
                        s_colorMarkFilter = IM_COL32(0, 0, 0, 0);
                        ImGui::CloseCurrentPopup();
                    }
                    for (auto& pair : Workspace::s_colorMarks) {    
                        if (!s_colorMarkNameFilter.empty() && LowerCase(pair.first).find(LowerCase(s_colorMarkNameFilter)) == std::string::npos) continue;
                        if (TextColorButton(pair.first.c_str(), ImGui::ColorConvertU32ToFloat4(pair.second))) {
                            s_colorMarkFilter = pair.second;
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }
                ImGui::EndChild();
                ImGui::EndPopup();
            }

            ImGui::SameLine();

            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputTextWithHint("##audioBusFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &s_audioBusFilter);
            ImGui::PopItemWidth();

            if (ImGui::BeginChild("##audioBuses", ImGui::GetContentRegionAvail())) {
                ImGui::Spacing();
                int busIndex = 0;
                int targetBusRemove = -1;
                bool hasCandidates = false;
                for (auto& bus : project.audioBuses) {
                    if (!s_audioBusFilter.empty() && LowerCase(bus.name).find(LowerCase(s_audioBusFilter)) == std::string::npos) continue;
                    if (s_colorMarkFilter != IM_COL32(0, 0, 0, 0) && bus.colorMark != s_colorMarkFilter) continue;
                    hasCandidates = true;
                    ImGui::PushID(bus.id);
                        if (ImGui::Button(ICON_FA_ELLIPSIS_VERTICAL)) {
                            ImGui::OpenPopup("##audioBusPopup");
                        }
                        ImGui::SetItemTooltip("%s %s", ICON_FA_ELLIPSIS_VERTICAL, Localization::GetString("MORE_AUDIO_BUS_PROPERTIES").c_str());
                        ImGui::SameLine();

                        if (ImGui::Button(ICON_FA_TRASH_CAN)) {
                            targetBusRemove = busIndex;
                        }
                        ImGui::SameLine();

                        bool reservedBusMain = bus.main;
                        ImVec4 buttonCol = ImGui::GetStyleColorVec4(ImGuiCol_Button);
                        if (!reservedBusMain) buttonCol.w = 0.1f;
                        ImGui::PushStyleColor(ImGuiCol_Button, buttonCol);
                        if (ImGui::Button(ICON_FA_STAR)) {
                            bus.main = !bus.main;
                            for (auto& iterableBus : project.audioBuses) {
                                if (iterableBus.id == bus.id) continue;
                                iterableBus.main = false;
                            }
                        }
                        ImGui::PopStyleColor();
                        ImGui::SetItemTooltip("%s %s", ICON_FA_STAR, Localization::GetString("SET_AS_MAIN_AUDIO_BUS").c_str());
                        ImGui::SameLine();

                        if (ImGui::Button(ICON_FA_PENCIL)) {
                            ImGui::OpenPopup("##audioBusRename");
                        }
                        ImGui::SetItemTooltip("%s %s", ICON_FA_PENCIL, Localization::GetString("RENAME_AUDIO_BUS").c_str());
                        ImGui::SameLine();

                        ImVec4 busColorMark4 = ImGui::ColorConvertU32ToFloat4(bus.colorMark);
                        if (ImGui::ColorButton("##audioBusColorMark", busColorMark4, ImGuiColorEditFlags_AlphaPreview)) {
                            ImGui::OpenPopup("##audioBusColorMark");
                        }
                        ImGui::SameLine();

                        if (ImGui::BeginPopup("##audioBusColorMark")) {
                            ImGui::SeparatorText(FormatString("%s %s", ICON_FA_TAG, Localization::GetString("COLOR_MARK").c_str()).c_str());
                            static std::string s_colorMarkFilter = "";
                            ImGui::InputTextWithHint("##colorMarkFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &s_colorMarkFilter);
                            if (ImGui::BeginChild("##colorMarkCandidates", ImVec2(ImGui::GetContentRegionAvail().x, 210))) {
                                for (auto& colorPair : Workspace::s_colorMarks) {
                                    ImVec4 v4ColorMark = ImGui::ColorConvertU32ToFloat4(colorPair.second);
                                    if (TextColorButton(colorPair.first.c_str(), v4ColorMark)) {
                                        bus.colorMark = colorPair.second;
                                        ImGui::CloseCurrentPopup();
                                    }
                                }
                            }
                            ImGui::EndChild();
                            ImGui::EndPopup();
                        }

                        bool treeNodeExpanded = ImGui::TreeNode(FormatString("%s %s", bus.main ? ICON_FA_STAR " " ICON_FA_VOLUME_HIGH : ICON_FA_VOLUME_HIGH, bus.name.c_str()).c_str());
                        bool propertiesPopupMustBeOpened = ImGui::IsItemClicked(ImGuiMouseButton_Right);
                        
                        ImGui::SameLine(0, 12);
                        auto redirectBusCandidate = Workspace::GetAudioBusByID(bus.redirectID);
                        std::string redirectBusText = FormatString("%s %s: %s", ICON_FA_FORWARD, Localization::GetString("REDIRECT_TO_BUS").c_str(), redirectBusCandidate.has_value() ? redirectBusCandidate.value()->name.c_str() : Localization::GetString("NONE").c_str());
                        if (ImGui::Button(redirectBusText.c_str())) {
                            ImGui::OpenPopup("##redirectAudioBus");
                        }

                        if (ImGui::BeginPopup("##redirectAudioBus")) {
                            ImGui::SeparatorText(redirectBusText.c_str());
                            static std::string s_redirectSearchFilter = "";                            
                            ImGui::InputTextWithHint("##audioBusFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &s_redirectSearchFilter);
                            if (ImGui::BeginChild("##audioBusCandidates", ImVec2(ImGui::GetContentRegionAvail().x, 210))) {
                                if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_XMARK, Localization::GetString("NONE").c_str()).c_str())) {
                                    bus.redirectID = -1;
                                    ImGui::CloseCurrentPopup();
                                }
                                bool hasCandidates = false;
                                for (auto& candidate : project.audioBuses) {
                                    if (candidate.id == bus.id) continue;
                                    if (!s_redirectSearchFilter.empty() && LowerCase(bus.name).find(LowerCase(s_redirectSearchFilter)) == std::string::npos) continue;
                                    hasCandidates = true;
                                    if (ImGui::MenuItem(FormatString("%s %s", candidate.main ? ICON_FA_STAR " " ICON_FA_VOLUME_HIGH : ICON_FA_VOLUME_HIGH, candidate.name.c_str()).c_str())) {
                                        project.audioBusesMutex->lock();
                                        bus.redirectID = candidate.id;
                                        project.audioBusesMutex->unlock();
                                        ImGui::CloseCurrentPopup();
                                    }
                                }
                                if (!hasCandidates) {
                                    UIHelpers::RenderNothingToShowText();
                                }
                            }
                            ImGui::EndChild();
                            ImGui::EndPopup();
                        }

                        if (treeNodeExpanded) {
                            if (ImGui::BeginChild("##usedInCompositionsChild", ImVec2(0, 210), ImGuiChildFlags_AutoResizeX)) {
                                bool hasCandidates = false;
                                ImGui::Text(FormatString("%s %s: ", ICON_FA_LAYER_GROUP, Localization::GetString("USED_IN_COMPOSITIONS").c_str()).c_str());
                                for (auto& composition : project.compositions) {
                                    auto usedAudioBuses = composition.GetUsedAudioBuses();
                                    if (std::find(usedAudioBuses.begin(), usedAudioBuses.end(), bus.id) != usedAudioBuses.end()) {
                                        hasCandidates = true;
                                        ImGui::BulletText("%s %s", ICON_FA_LAYER_GROUP, composition.name.c_str());
                                        if (ImGui::IsItemClicked()) {
                                            project.selectedCompositions = {composition.id};
                                        }
                                    }
                                }
                                if (!hasCandidates) {
                                    UIHelpers::RenderNothingToShowText();
                                }
                            }
                            ImGui::EndChild();
                            ImGui::SameLine(0, 20);
                            if (ImGui::BeginChild("##waveformContainer", ImVec2(0, 0), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
                                UIHelpers::RenderRawAudioSamplesWaveform(&bus.samples);
                            }
                            ImGui::EndChild();
                            ImGui::TreePop();
                        } 

                        static bool renameFieldFocused = false;
                        if (ImGui::BeginPopup("##audioBusRename")) {
                            if (!renameFieldFocused) ImGui::SetKeyboardFocusHere(0);
                            ImGui::InputTextWithHint("##renameField", FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("AUDIO_BUS_NAME").c_str()).c_str(), &bus.name);
                            ImGui::EndPopup();
                            renameFieldFocused = true;
                        } else renameFieldFocused = false;

                        if (propertiesPopupMustBeOpened) {
                            ImGui::OpenPopup("##audioBusPopup");
                        }

                        if (ImGui::BeginPopup("##audioBusPopup")) {
                            ImGui::SeparatorText(FormatString("%s %s", ICON_FA_VOLUME_HIGH, bus.name.c_str()).c_str());
                            ImGui::InputTextWithHint("##renameField", FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("AUDIO_BUS_NAME").c_str()).c_str(), &bus.name);
                            if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_STAR, Localization::GetString("SET_AS_MAIN_AUDIO_BUS").c_str()).c_str())) {
                                bus.main = !bus.main;
                                for (auto& iterableBus : project.audioBuses) {
                                    if (iterableBus.id == bus.id) continue;
                                    iterableBus.main = false;
                                }
                            }
                            if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_COPY, Localization::GetString("COPY_AUDIO_BUS_ID").c_str()).c_str())) {
                                ImGui::SetClipboardText(std::to_string(bus.id).c_str());
                            }
                            if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_TRASH_CAN, Localization::GetString("REMOVE_AUDIO_BUS").c_str()).c_str())) {
                                targetBusRemove = busIndex;
                            }
                            ImGui::EndPopup();
                        }
                    ImGui::PopID();
                    busIndex++;
                }

                if (targetBusRemove >= 0) {
                    project.audioBusesMutex->lock();
                    project.audioBuses.erase(project.audioBuses.begin() + targetBusRemove);
                    project.audioBusesMutex->unlock();
                }

                if (!hasCandidates) {
                    UIHelpers::RenderNothingToShowText();
                }

                ImGui::Spacing();
                std::string totalAudioBusCountText = FormatString("%s %s: %i", ICON_FA_GEARS, Localization::GetString("TOTAL_AUDIO_BUSES_COUNT").c_str(), (int) project.audioBuses.size());
                ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - ImGui::CalcTextSize(totalAudioBusCountText.c_str()).x / 2.0f);
                ImGui::Text("%s", totalAudioBusCountText.c_str());
            }
            ImGui::EndChild();
        }
        ImGui::End();
    }
};