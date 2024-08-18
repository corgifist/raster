#pragma once

#include "raster.h"
#include "typedefs.h"
#include "dylib.hpp"
#include "randomizer.h"
#include "gpu/gpu.h"

namespace Raster {
    struct AssetBase {
    public:
        int id;
        std::string name;
        std::string packageName;

        void Initialize();
        void RenderDetails();

        std::optional<Texture> GetPreviewTexture();
        void Import(std::string t_path);

        bool IsReady();

        Json Serialize();
        void Load(Json t_data);
    private:

        virtual bool AbstractIsReady() { return true; }

        virtual std::optional<Texture> AbstractGetPreviewTexture() { return std::nullopt; }
        virtual void AbstractImport(std::string t_path);

        virtual void AbstractLoad(Json t_data) = 0;
        virtual Json AbstractSerialize() = 0;

        virtual void AbstractRenderDetails() {};
    };

    using AbstractAsset = std::shared_ptr<AssetBase>;
    using AssetSpawnProcedure = std::function<AbstractAsset()>;
};