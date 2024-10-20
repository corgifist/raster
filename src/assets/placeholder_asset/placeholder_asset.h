#pragma once

#include "common/assets.h"
#include "gpu/gpu.h"
#include "../../ImGui/imgui.h"
#include "font/font.h"

namespace Raster {
    struct PlaceholderAsset : public AssetBase {
    public:
        PlaceholderAsset();

    private:
        bool AbstractIsReady();

        std::optional<Texture> AbstractGetPreviewTexture();

        void AbstractLoad(Json t_data);
        Json AbstractSerialize();

        void AbstractRenderDetails();

        std::optional<std::string> AbstractGetResolution();

        std::optional<Texture> m_texture;

        glm::vec2 m_resolution;

        static std::optional<Pipeline> s_smptePipeline;
    };
};