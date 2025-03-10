#include "folder_asset.h"
#include "common/asset_base.h"
#include "font/IconsFontAwesome5.h"
#include "raster.h"

namespace Raster {
    FolderAsset::FolderAsset() {
        AssetBase::Initialize();
    }

    std::optional<std::vector<AbstractAsset>*> FolderAsset::AbstractGetChildAssets() {
        return &assets;
    }

    bool FolderAsset::AbstractIsReady() {
        for (auto& asset : assets) {
            if (!asset->IsReady()) return false;
        }
        return true;
    }

    Json FolderAsset::AbstractSerialize() {
        Json result = Json::array();
        for (auto& asset : assets) {
            result.push_back(asset->Serialize());
        }
        return result;
    }

    void FolderAsset::AbstractLoad(Json t_data) {
        for (auto& data : t_data) {
            auto assetCandidate = Assets::InstantiateSerializedAsset(data);
            if (assetCandidate) {
                assets.push_back(*assetCandidate);
            }
        }
    }

    void FolderAsset::AbstractOnTimelineDrop(float t_frame) {
        for (auto& asset : assets) {
            asset->OnTimelineDrop(t_frame);
        }
    }

    std::optional<Texture> FolderAsset::AbstractGetPreviewTexture() {
        for (auto& asset : assets) {
            auto candidate = asset->GetPreviewTexture();
            if (candidate) return candidate;
        }
        return std::nullopt;
    }

    void FolderAsset::AbstractRenderDetails() {
        if (!assets.empty()) {
            for (auto& asset : assets) {
                auto assetImplementationCandidate = Assets::GetAssetImplementationByPackageName(asset->packageName);
                if (!assetImplementationCandidate) continue;
                auto& assetImplementation = *assetImplementationCandidate;
                ImGui::BulletText("%s %s", assetImplementation.description.icon.c_str(), asset->name.c_str());
                if (ImGui::IsItemClicked()) {
                    Workspace::GetProject().selectedAssets = {asset->id};
                }
                if (ImGui::BeginItemTooltip()) {
                    asset->RenderDetails();
                    ImGui::EndTooltip();
                }
            }
        }
    }

    void FolderAsset::AbstractDelete() {
        for (auto& asset : assets) {
            asset->Delete();
        }
    }
};

extern "C" {
    Raster::AbstractAsset SpawnAsset() {
        return (Raster::AbstractAsset) std::make_shared<Raster::FolderAsset>();
    }

    Raster::AssetDescription GetDescription() {
        return Raster::AssetDescription{
            .prettyName = "Folder Asset",
            .packageName = RASTER_PACKAGED "folder_asset",
            .icon = ICON_FA_FOLDER_OPEN,
            .extensions = {}
        };
    }
}