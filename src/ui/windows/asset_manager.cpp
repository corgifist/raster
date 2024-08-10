#include "asset_manager.h"

namespace Raster {
    void AssetManagerUI::Render() {
        ImGui::Begin(FormatString("%s %s", ICON_FA_FOLDER, Localization::GetString("ASSET_MANAGER").c_str()).c_str());
            ImGui::Text("Lol billion quadrillion assets");

            ImGui::Stripes(ImVec4(0.05f, 0.05f, 0.05f, 1), ImVec4(0.1f, 0.1f, 0.1f, 1), 12);
        ImGui::End();
    }
};