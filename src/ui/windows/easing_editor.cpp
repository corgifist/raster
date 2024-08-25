#include "easing_editor.h"

namespace Raster {
    void EasingEditorUI::Render() {
        if (ImGui::Begin(FormatString("%s %s", ICON_FA_BEZIER_CURVE, Localization::GetString("EASING_EDITOR").c_str()).c_str())) {
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
            auto& selectedKeyframes = project.selectedKeyframes;
            auto attributeCandidate = Workspace::GetAttributeByKeyframeID(selectedKeyframes.empty() ? -1 : selectedKeyframes[0]);
            auto keyframeCandidate = Workspace::GetKeyframeByKeyframeID(selectedKeyframes.empty() ? -1 : selectedKeyframes[0]);
            bool easingAvailable = keyframeCandidate.has_value() && keyframeCandidate.value()->easing.has_value();
            std::optional<AbstractEasing> easingCandidate = easingAvailable ? keyframeCandidate.value()->easing : std::nullopt;

            std::string headerText = FormatString("%s %s | %s %s", 
                ICON_FA_LINK, attributeCandidate.has_value() ? attributeCandidate.value()->name.c_str() : Localization::GetString("NO_ATTRIBUTE").c_str(), 
                ICON_FA_BEZIER_CURVE, easingCandidate.has_value() ? easingCandidate.value()->prettyName.c_str() : Localization::GetString("NO_EASING").c_str());

            ImVec2 headerSize = ImGui::CalcTextSize(headerText.c_str());
            ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - headerSize.x / 2.0f);
            ImGui::Text("%s", headerText.c_str());

            if (ImGui::BeginChild("##isolatedEasingContent", ImGui::GetContentRegionAvail())) {
                static ImVec2 contentSize = ImVec2(100, 100);
                ImGui::SetCursorPos(
                    ImGui::GetWindowSize() / 2.0f - contentSize / 2.0f
                );
                if (ImGui::BeginChild("##easingEditorContent", ImVec2(0, 0), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
                    if (easingCandidate.has_value()) {
                        easingCandidate.value()->RenderDetails();
                    } else {
                        ImGui::SetWindowFontScale(1.6f);
                        ImGui::PushFont(Font::s_denseFont);
                            std::string warningMessage;
                            if (!attributeCandidate.has_value()) {
                                warningMessage = Localization::GetString("NO_ATTRIBUTE_AVAILABLE");
                            } else if (!keyframeCandidate.has_value()) {
                                warningMessage = Localization::GetString("NO_KEYFRAME_AVAILABLE");
                            } else if (!easingCandidate.has_value()) {
                                warningMessage = Localization::GetString("NO_EASING_AVAILABLE");
                            }
                            ImGui::Text("%s", warningMessage.c_str());
                        ImGui::PopFont();
                        ImGui::SetWindowFontScale(1.0f);
                    }
                    contentSize = ImGui::GetWindowSize();
                }
                ImGui::EndChild();
            }
            ImGui::EndChild();
        }
        
        ImGui::End();
    }
};