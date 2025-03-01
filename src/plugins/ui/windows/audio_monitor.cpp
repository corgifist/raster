#include "audio_monitor.h"
#include "common/audio_bus.h"
#include "common/localization.h"
#include "common/ui_helpers.h"
#include "common/audio_info.h"
#include "../../../ImGui/imgui_stdlib.h"
#include "common/layouts.h"
#include "common/workspace.h"
#include "font/IconsFontAwesome5.h"

namespace Raster {

    static void RenderCustomDecibelScale(float t_linear, float t_width, float t_height) {
        ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        float decibel = 80 - std::min(std::abs(LinearToDecibel(std::abs(t_linear))), 80.0f);
        const ImVec2 scaleSize(t_width, t_height);
        
        ImVec2 frameUL = cursorPos;
        ImVec2 frameBR = cursorPos;
        frameBR += scaleSize;
        ImGui::RenderFrame(frameUL, frameBR, ImGui::GetColorU32(ImGuiCol_PopupBg));
        ImGui::ItemSize(ImRect{frameUL, frameBR});
        if (!ImGui::ItemAdd(ImRect{frameUL, frameBR}, ImGui::GetID(t_linear))) return;
        ImGui::GetWindowDrawList()->AddRectFilledMultiColor(frameUL, frameBR, ImGui::GetColorU32(ImVec4(1, 0, 0, 0.5f)), 
                                                                                            ImGui::GetColorU32(ImVec4(1, 0, 0, 0.5f)), 
                                                                                            ImGui::GetColorU32(ImVec4(0, 1, 0, 0.5f)), 
                                                                                            ImGui::GetColorU32(ImVec4(0, 1, 0, 0.5f)));

        frameUL.y += (80 - decibel) / 80.0f * scaleSize.y;
        glm::vec4 mixedColor = glm::mix(glm::vec4(0, 1, 0, 1), glm::vec4(1, 0, 0, 1), decibel / 60.0);
        ImVec4 c4(mixedColor.r, mixedColor.g, mixedColor.b, mixedColor.a);
        ImGui::GetWindowDrawList()->AddRectFilledMultiColor(frameUL, frameBR, ImGui::GetColorU32(c4), 
                                                                                    ImGui::GetColorU32(c4), 
                                                                                    ImGui::GetColorU32(ImVec4(0, 1, 0, 1)), 
                                                                                    ImGui::GetColorU32(ImVec4(0, 1, 0, 1)));
    }

    Json AudioMonitorUI::AbstractSerialize() {
        return {
            {"BusID", busID}
        };
    }

    void AudioMonitorUI::AbstractLoad(Json t_data) {
        this->busID = t_data["BusID"];
    }

    void AudioMonitorUI::AbstractRender() {
        if (!open) {
            Layouts::DestroyWindow(id);
        }
        AudioBus* targetAudioBus = nullptr;
        if (Workspace::IsProjectLoaded()) {
            auto& project = Workspace::GetProject();
            project.audioBusesMutex->lock();
            for (auto& bus : project.audioBuses) {
                if ((busID < 0 && bus.main) || (busID == bus.id)) {
                    targetAudioBus = &bus;
                    break;
                }
            }
            project.audioBusesMutex->unlock();
        }
        ImGui::SetNextWindowSize(ImVec2(300, 700), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(FormatString("%s %s###%i", busID > 0 ? ICON_FA_VOLUME_HIGH : ICON_FA_STAR, targetAudioBus ? targetAudioBus->name.c_str() : Localization::GetString("AUDIO_MONITOR").c_str(), id).c_str(), &open)) {
            if (!Workspace::IsProjectLoaded() || !targetAudioBus || (targetAudioBus && targetAudioBus->samples.size() < AudioInfo::s_channels)) {
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
            auto parentWindowSize = ImGui::GetWindowSize();
            ImGui::SetCursorPos(ImGui::GetWindowSize() / 2.0f - audioMonitorChildSize / 2.0f);
            if (ImGui::BeginChild("##audioMonitorChild", ImVec2(0, 0), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
                if (targetAudioBus && targetAudioBus->samples.size() > AudioInfo::s_channels) {
                    for (int channel = 0; channel < AudioInfo::s_channels; channel++) {
                        std::vector<float> constructedWaveform(100);
                        int waveformIndex = 0;
                        for (int i = AudioInfo::s_globalAudioOffset + AudioInfo::s_channels - 1; i < AudioInfo::s_globalAudioOffset + 100 * AudioInfo::s_channels; i += AudioInfo::s_channels) {
                            constructedWaveform[waveformIndex++] = targetAudioBus->samples[i % AudioInfo::s_periodSize];
                        }
                        RenderCustomDecibelScale(constructedWaveform[0], 30, parentWindowSize.y - ImGui::GetStyle().WindowPadding.y);
                        ImGui::SameLine();
                    }
                }
                audioMonitorChildSize = ImGui::GetWindowSize();
            }
            ImGui::EndChild();

        }
        ImGui::End();
    }
};