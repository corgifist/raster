#include "asset_manager.h"

namespace Raster {
    void AssetManagerUI::Render() {
        ImGui::Begin(FormatString("%s %s", ICON_FA_FOLDER, Localization::GetString("ASSET_MANAGER").c_str()).c_str());
        ImGui::End();
    }
};