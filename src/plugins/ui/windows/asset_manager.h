#pragma once

#include "raster.h"
#include "common/user_interface.h"
#include "common/common.h"
#include "utils/widgets.h"
#include "font/IconsFontAwesome5.h"
#include "font/font.h"

#include "../../../ImGui/imgui.h"
#include "../../../ImGui/imgui_stdlib.h"
#include "../../../ImGui/imgui_stripes.h"

#define ASSET_MANAGER_DRAG_DROP_PAYLOAD "ASSET_MANAGER_DRAG_DROP_PAYLOAD"

namespace Raster {
    struct AssetManagerUI : public UserInterfaceBase {
        bool open;
        AssetManagerUI() : UserInterfaceBase(), open(true) {}

        void AbstractRender();
        Json AbstractSerialize();
        void AbstractLoad(Json t_data);

        static void RenderAssetPopup(AbstractAsset& asset);
        static std::optional<AbstractAsset> ImportAsset(std::optional<std::string> targetAssetPackageName = std::nullopt);
        static void AutoImportAsset(std::optional<std::string> targetAssetPackageName = std::nullopt);
    };
};