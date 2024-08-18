#include "image_asset.h"

namespace Raster {
    ImageAsset::ImageAsset() {
        AssetBase::Initialize();

        this->m_uploadID = 0;
        this->m_texture = std::nullopt;
        this->m_asyncCopy = std::nullopt;
    }

    bool ImageAsset::AbstractIsReady() {
        if (m_texture.has_value()) return true;
        return false;
    }

    std::optional<Texture> ImageAsset::AbstractGetPreviewTexture() {
        return m_texture;
    }

    void ImageAsset::AbstractImport(std::string t_path) {
        // copy image from t_path to project folder
        this->m_relativePath = FormatString("%i%s", id, std::filesystem::path(t_path).extension().c_str());
        
    }

    Json ImageAsset::AbstractSerialize() {
        return {};
    }

    void ImageAsset::AbstractRenderDetails() {

    }
};