#include "asset_manager.h"

namespace Raster {
    void AssetManagerUI::Render() {
        ImGui::Begin(FormatString("%s %s", ICON_FA_FOLDER, Localization::GetString("ASSET_MANAGER").c_str()).c_str());
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
        ImGui::End();
    }
};