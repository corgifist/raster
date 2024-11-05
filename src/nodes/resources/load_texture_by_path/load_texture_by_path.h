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
        
        void UpdateTextureArchive(ContextData& t_contextData);

        AbstractPinMap AbstractExecute(ContextData& t_contextData);

        bool AbstractDetailsAvailable();

        void AbstractRenderProperties();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        std::string Icon();
        std::string AbstractHeader();
        std::optional<std::string> Footer();

    private:
        AsyncUploadInfoID m_asyncUploadID;
        AsyncImageLoader m_loader;

        std::optional<std::string> m_lastPath;
    };
};