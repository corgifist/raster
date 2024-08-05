#pragma once
#include "raster.h"
#include "common/common.h"
#include "gpu/gpu.h"
#include "image/image.h"
#include "gpu/async_upload.h"

namespace Raster {

    struct TextureArchive {
        Texture texture;
        std::string path;

        TextureArchive(Texture t_texture, std::string t_path) :
            texture(t_texture), path(t_path) {}
    };

    struct LoadTextureByPath : public NodeBase {
    public:
        std::optional<TextureArchive> archive;

        LoadTextureByPath();
        ~LoadTextureByPath();
        
        void UpdateTextureArchive();

        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});

        bool AbstractDetailsAvailable();

        void AbstractRenderProperties();

        std::string Icon();
        std::string AbstractHeader();
        std::optional<std::string> Footer();

    private:
        AsyncUploadInfoID m_asyncUploadID;
        AsyncImageLoader m_loader;
    };
};