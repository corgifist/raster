#include "common/asset_base.h"

namespace Raster {

    AssetBase::AssetBase() {}

    AssetBase::~AssetBase() {}

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
            {"Data", AbstractSerialize()},
            {"PackageName", packageName}
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
    
    std::optional<std::uintmax_t> AssetBase::GetSize() {
        return AbstractGetSize();
    }

    std::optional<std::string> AssetBase::GetResolution() {
        return AbstractGetResolution();
    }

    std::optional<std::string> AssetBase::GetDuration() {
        return AbstractGetDuration();
    }

    std::optional<std::string> AssetBase::GetPath() {
        return AbstractGetPath();
    }

    void AssetBase::Delete() {
        AbstractDelete();
    }

    void AssetBase::Load(Json t_data) {
        AbstractLoad(t_data);
    }
};