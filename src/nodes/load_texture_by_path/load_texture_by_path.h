#pragma once
#include "raster.h"
#include "common/common.h"
#include "gpu/gpu.h"

namespace Raster {

    struct TextureArchive {
        Texture texture;
        std::string path;

        TextureArchive(Texture t_texture, std::string t_path) :
            texture(t_texture), path(t_path) {}
    };

    struct LoadTextureByPath : public NodeBase {
        std::optional<TextureArchive> archive;

        LoadTextureByPath();
        
        void UpdateTextureArchive();

        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});

        void AbstractRenderProperties();

        std::string Icon();
        std::string AbstractHeader();
        std::optional<std::string> Footer();
    };
};