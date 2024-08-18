#include "common/asset_base.h"

namespace Raster {
    void AssetBase::Initialize() {
        this->id = Randomizer::GetRandomInteger();
        this->name = "Uninitialized Asset";
    }

    void AssetBase::RenderDetails() {
        AbstractRenderDetails();
    }

    Json AssetBase::Serialize() {
        return {
            {"ID", id},
            {"Name", name},
            {"Data", AbstractSerialize()}
        };
    }

    bool AssetBase::IsReady() {
        return AbstractIsReady();
    }

    void AssetBase::Import(std::string t_path) {
        AbstractImport(t_path);
    }

    std::optional<Texture> AssetBase::GetPreviewTexture() {
        return AbstractGetPreviewTexture();
    }

    void AssetBase::Load(Json t_data) {
        AbstractLoad(t_data);
    }
};