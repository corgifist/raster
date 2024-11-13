#pragma once

#include "common/asset_base.h"
#include "gpu/async_upload.h"
#include "gpu/gpu.h"
#include "../../ImGui/imgui.h"

namespace Raster {
    struct ImageAsset : public AssetBase {
    public:
        ImageAsset();

    private:
        bool AbstractIsReady();

        std::optional<Texture> AbstractGetPreviewTexture();
        void AbstractImport(std::string t_path);

        void AbstractLoad(Json t_data);
        Json AbstractSerialize();

        void AbstractRenderDetails();
        
        void AbstractDelete();

        std::optional<std::uintmax_t> AbstractGetSize();
        std::optional<std::string> AbstractGetResolution();
        std::optional<std::string> AbstractGetPath();

        std::string m_relativePath;
        std::string m_originalPath;

        AsyncUploadInfoID m_uploadID;
        AsyncImageLoader m_loader;

        std::optional<std::future<bool>> m_asyncCopy;

        std::optional<Texture> m_texture;
        std::optional<std::uintmax_t> m_cachedSize;

        static std::optional<Pipeline> s_gammaPipeline;
    };
};