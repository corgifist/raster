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

        AssetBase();
        ~AssetBase();

        void Initialize();
        void RenderDetails();
        void RenderPopup();

        std::optional<Texture> GetPreviewTexture();
        void Import(std::string t_path);

        std::optional<std::uintmax_t> GetSize();
        std::optional<std::string> GetResolution();
        std::optional<std::string> GetDuration();
        std::optional<std::string> GetPath();

        void RenderPreviewOverlay(glm::vec2 t_regionSize);

        bool IsReady();

        void Delete();

        Json Serialize();
        void Load(Json t_data);
    private:

        virtual bool AbstractIsReady() { return true; }

        virtual std::optional<Texture> AbstractGetPreviewTexture() { return std::nullopt; }
        virtual void AbstractImport(std::string t_path) {}

        virtual std::optional<std::string> AbstractGetResolution() { return std::nullopt; }

        virtual std::optional<std::string> AbstractGetDuration() { return std::nullopt; }

        virtual std::optional<std::string> AbstractGetPath() { return std::nullopt; }

        virtual void AbstractRenderPreviewOverlay(glm::vec2 t_regionSize) { }

        virtual void AbstractLoad(Json t_data) {}
        virtual Json AbstractSerialize() { return {}; };

        virtual void AbstractDelete() {}

        virtual std::optional<std::uintmax_t> AbstractGetSize() { return std::nullopt; }

        virtual void AbstractRenderDetails() {};
        virtual void AbstractRenderPopup() {};
    };

    using AbstractAsset = std::shared_ptr<AssetBase>;
    using AssetSpawnProcedure = std::function<AbstractAsset()>;
};