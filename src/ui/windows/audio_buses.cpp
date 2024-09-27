#include "audio_buses.h"

namespace Raster {

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

            if (ImGui::BeginChild("##busesSearchChild", ImVec2(ImGui::GetContentRegionAvail().x, 0), ImGuiChildFlags_AutoResizeY)) {
                static std::string s_newAudioBusName = "";
                if (ImGui::Button(ICON_FA_PLUS)) {
                    ImGui::OpenPopup("##createNewAudioBus");
                    s_newAudioBusName = Localization::GetString("NEW_AUDIO_BUS");
                }
                if (ImGui::BeginPopup("##createNewAudioBus")) {
                    ImGui::InputTextWithHint("##newAudioBusName", FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("NEW_AUDIO_BUS_NAME").c_str()).c_str(), &s_newAudioBusName);
                    ImGui::SameLine();
                    if (ImGui::Button(FormatString("%s %s", ICON_FA_CHECK, Localization::GetString("OK").c_str()).c_str()) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                        AudioBus newAudioBus;
                        newAudioBus.name = s_newAudioBusName;
                        newAudioBus.redirectID = mainAudioBusID;
                        project.audioBuses.push_back(newAudioBus);
                        ImGui::CloseCurrentPopup();
                    }
                    ImGui::EndPopup();
                }

                auto filter4 = ImGui::ColorConvertU32ToFloat4(s_colorMarkFilter);
                ImGui::SameLine();
                if (ImGui::ColorButton(FormatString("%s %s", ICON_FA_DROPLET, Localization::GetString("COLOR_MARK_FILTER").c_str()).c_str(), filter4)) {

                }
                ImGui::SameLine();
                ImGui::InputTextWithHint("##audioBusFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &s_audioBusFilter);
            }
            ImGui::EndChild();
            if (ImGui::BeginChild("##audioBuses", ImGui::GetContentRegionAvail())) {
                ImGui::Spacing();
                for (auto& bus : project.audioBuses) {
                    if (!s_audioBusFilter.empty() && LowerCase(bus.name).find(LowerCase(s_audioBusFilter)) == std::string::npos) continue;
                    ImGui::PushID(bus.id);
                        ImGui::AlignTextToFramePadding();
                        ImGui::Text("%s %s", ICON_FA_VOLUME_HIGH, bus.name.c_str());
                        ImGui::SameLine();
                        if (ImGui::Button(FormatString("%s %s", bus.main ? ICON_FA_CHECK : ICON_FA_XMARK, Localization::GetString("MAIN").c_str()).c_str())) {
                            bus.main = !bus.main;
                            for (auto& iterableBus : project.audioBuses) {
                                if (iterableBus.id == bus.id) continue;
                                iterableBus.main = false;
                            }
                        }
                        ImGui::SameLine();

                        auto redirectBusCandidate = Workspace::GetAudioBusByID(bus.redirectID);
                        std::string redirectBusName = Localization::GetString("NONE");
                        if (redirectBusCandidate.has_value()) {
                            redirectBusName = redirectBusCandidate.value()->name;
                        }
                        std::string redirectPopupID = FormatString("##redirectBus%i", bus.id);
                        if (ImGui::Button(FormatString("%s %s: %s", ICON_FA_FORWARD, Localization::GetString("REDIRECT_TO_BUS").c_str(), redirectBusName.c_str()).c_str())) {
                            ImGui::OpenPopup(redirectPopupID.c_str());
                        }

                        if (ImGui::BeginPopup(redirectPopupID.c_str())) {
                            ImGui::SeparatorText(FormatString("%s %s: %s", ICON_FA_FORWARD, Localization::GetString("REDIRECT_TO_BUS"), redirectBusName.c_str()).c_str());
                            ImGui::EndPopup();
                        }
                    ImGui::PopID();
                }
            }
            ImGui::EndChild();
        }
        ImGui::End();
    }
};