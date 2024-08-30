#include "get_asset_texture.h"

namespace Raster {

    GetAssetTexture::GetAssetTexture() {
        NodeBase::Initialize();

        SetupAttribute("AssetID", 0);

        AddOutputPin("Texture");
        AddOutputPin("Resolution");
        AddOutputPin("AspectRatio");
    }

    AbstractPinMap GetAssetTexture::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};
        auto assetIDCandidate = GetAttribute<int>("AssetID");
        if (assetIDCandidate.has_value()) {
            auto& assetID = assetIDCandidate.value();
            auto assetCandidate = Workspace::GetAssetByAssetID(assetID);
            if (assetCandidate.has_value()) {
                auto& asset = assetCandidate.value();
                auto textureCandidate = asset->GetPreviewTexture();
                if (textureCandidate.has_value()) {
                    auto& texture = textureCandidate.value();
                    TryAppendAbstractPinMap(result, "Texture", texture);
                    TryAppendAbstractPinMap(result, "Resolution", glm::vec2(texture.width, texture.height));
                    TryAppendAbstractPinMap(result, "AspectRatio", (float) texture.width / (float) texture.height);
                }
            }
        }
        return result;
    }

    void GetAssetTexture::AbstractRenderProperties() {
        RenderAttributeProperty("AssetID");
    }

    void GetAssetTexture::AbstractLoadSerialized(Json t_data) {
        RASTER_DESERIALIZE_WRAPPER(int, "AssetID");
    }

    Json GetAssetTexture::AbstractSerialize() {
        return {
            RASTER_SERIALIZE_WRAPPER(int, "AssetID")
        };
    }

    bool GetAssetTexture::AbstractDetailsAvailable() {
        return false;
    }

    std::string GetAssetTexture::AbstractHeader() {
        return "Get Asset Texture";
    }

    std::string GetAssetTexture::Icon() {
        return ICON_FA_IMAGE " " ICON_FA_BOX_OPEN;
    }

    std::optional<std::string> GetAssetTexture::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::GetAssetTexture>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Get Asset Texture",
            .packageName = RASTER_PACKAGED "get_asset_texture",
            .category = Raster::DefaultNodeCategories::s_resources
        };
    }
}