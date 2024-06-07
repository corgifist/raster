#include "rendering.h"
#include "font/font.h"

namespace Raster {
    void RenderingUI::Render() {
        ImGui::Begin(FormatString("%s %s", ICON_FA_IMAGE, Localization::GetString("RENDERING").c_str()).c_str(), nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
            if (Workspace::s_project.has_value()) {
                auto& project = Workspace::s_project.value();

                static float miniTimelineSize = 20;
                ImGui::BeginChild("##renderPreview", ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - miniTimelineSize));
                ImGui::EndChild();

                ImGui::BeginChild("##miniTimeline", ImVec2(ImGui::GetContentRegionAvail().x, miniTimelineSize));
                    float firstTimelineCursorY = ImGui::GetCursorPosY();
                    ImGui::Text("%s", project.FormatFrameToTime(project.currentFrame).c_str());
                    miniTimelineSize = ImGui::GetCursorPosY() - firstTimelineCursorY;
                ImGui::EndChild();
            }
        ImGui::End();
    }
};