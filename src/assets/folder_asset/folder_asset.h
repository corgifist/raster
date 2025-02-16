#pragma once

#include "common/asset_base.h"
#include "gpu/async_upload.h"
#include "gpu/gpu.h"
#include "../../ImGui/imgui.h"

namespace Raster {
    struct FolderAsset : public AssetBase {
    public:
        std::vector<AbstractAsset> assets;
        FolderAsset();

    private:
        std::optional<std::vector<AbstractAsset>*> AbstractGetChildAssets();

        bool AbstractIsReady();

        std::optional<Texture> AbstractGetPreviewTexture();
        void AbstractOnTimelineDrop(float t_frame);

        void AbstractLoad(Json t_data);
        Json AbstractSerialize();

        void AbstractRenderDetails();
        
        void AbstractDelete();
    };
};