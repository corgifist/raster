#pragma once

#include "asset_base.h"
#include "libraries.h"

namespace Raster {
    struct AssetDescription {
        std::string prettyName;
        std::string packageName;
        std::vector<std::string> extensions;
    };

    struct AssetImplementation {
        AssetDescription description;
        AssetSpawnProcedure spawn;
    };

    struct Assets {
        static std::vector<AssetImplementation> s_implementations;

        static void Initialize();
        static std::optional<AbstractAsset> InstantiateAsset(std::string t_packageName);
        static std::optional<AbstractAsset> InstantiateSerializedAsset(Json t_data);

        static std::optional<AssetImplementation> GetAssetImplementation(std::string t_packageName);
    };
}