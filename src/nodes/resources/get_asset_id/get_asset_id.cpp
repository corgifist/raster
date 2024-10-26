#include "get_asset_id.h"
#include "common/asset_id.h"
#include "../../../ImGui/imgui.h"

namespace Raster {

    GetAssetID::GetAssetID() {
        NodeBase::Initialize();

        SetAttributeValue("AssetID", 0);

        AddOutputPin("ID");
    }

    AbstractPinMap GetAssetID::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};
        auto assetIDCandidate = GetAttribute<int>("AssetID");
        if (assetIDCandidate.has_value()) {
            auto& assetID = assetIDCandidate.value();
            auto assetCandidate = Workspace::GetAssetByAssetID(assetID);
            if (assetCandidate.has_value()) {
                AssetID wrappedAssetID(assetID);
                TryAppendAbstractPinMap(result, "ID", wrappedAssetID);
            }
        }
        return result;
    }

    void GetAssetID::AbstractRenderProperties() {
        RenderAttributeProperty("AssetID");
    }

    void GetAssetID::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json GetAssetID::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    void GetAssetID::AbstractRenderDetails() {
        auto assetIDCandidate = GetAttribute<int>("AssetID");
        if (assetIDCandidate.has_value()) {
            auto& assetID = assetIDCandidate.value();
            auto assetCandidate = Workspace::GetAssetByAssetID(assetID);
            if (assetCandidate.has_value()) {
                assetCandidate.value()->RenderDetails();
            }
        }
    }

    bool GetAssetID::AbstractDetailsAvailable() {
        auto assetIDCandidate = GetAttribute<int>("AssetID");
        if (assetIDCandidate.has_value()) {
            auto& assetID = assetIDCandidate.value();
            auto assetCandidate = Workspace::GetAssetByAssetID(assetID);
            if (assetCandidate.has_value()) {
                return true;
            }
        }
        return false;
    }

    std::string GetAssetID::AbstractHeader() {
        auto assetIDCandidate = GetAttribute<int>("AssetID");
        if (assetIDCandidate.has_value()) {
            auto& assetID = assetIDCandidate.value();
            auto assetCandidate = Workspace::GetAssetByAssetID(assetID);
            if (assetCandidate.has_value()) {
                return assetCandidate.value()->name;
            }
        }
        return "Get Asset ID";
    }

    std::string GetAssetID::Icon() {
        return ICON_FA_GEARS " " ICON_FA_BOX_OPEN;
    }

    std::optional<std::string> GetAssetID::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::GetAssetID>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Get Asset ID",
            .packageName = RASTER_PACKAGED "get_asset_id",
            .category = Raster::DefaultNodeCategories::s_resources
        };
    }
}