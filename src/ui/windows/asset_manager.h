#pragma once

#include "raster.h"
#include "ui/ui.h"
#include "common/common.h"
#include "utils/widgets.h"
#include "font/IconsFontAwesome5.h"
#include "font/font.h"

#include "../../ImGui/imgui.h"
#include "../../ImGui/imgui_stdlib.h"
#include "../../ImGui/imgui_stripes.h"

#define ASSET_MANAGER_DRAG_DROP_PAYLOAD "ASSET_MANAGER_DRAG_DROP_PAYLOAD"

namespace Raster {
    struct AssetManagerUI : public UI {
        void Render();

        static void RenderAssetPopup(AbstractAsset& asset);
        static std::optional<AbstractAsset> ImportAsset(std::optional<std::string> targetAssetPackageName = std::nullopt);
        static void AutoImportAsset(std::optional<std::string> targetAssetPackageName = std::nullopt);
    };
};