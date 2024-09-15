#pragma once

#include "common/assets.h"
#include "gpu/gpu.h"
#include "../../ImGui/imgui.h"
#include "font/font.h"

#include "../../avcpp/av.h"
#include "../../avcpp/ffmpeg.h"
#include "../../avcpp/codec.h"
#include "../../avcpp/packet.h"
#include "../../avcpp/videorescaler.h"
#include "../../avcpp/audioresampler.h"
#include "../../avcpp/avutils.h"

// API2
#include "../../avcpp/format.h"
#include "../../avcpp/formatcontext.h"
#include "../../avcpp/codec.h"
#include "../../avcpp/codeccontext.h"

using namespace av;

namespace Raster {
    struct MediaAsset : public AssetBase {
    public:
        MediaAsset();

    private:
        bool AbstractIsReady();

        void AbstractImport(std::string t_path);
        void AbstractDelete();
        std::optional<Texture> AbstractGetPreviewTexture();

        void AbstractLoad(Json t_data);
        Json AbstractSerialize();

        void AbstractRenderDetails();

        std::optional<std::string> AbstractGetResolution();
        std::optional<std::string> AbstractGetPath();
        std::optional<uintmax_t> AbstractGetSize();

        std::string m_relativePath;
        std::string m_originalPath;

        av::FormatContext m_formatCtx;

        bool m_formatCtxWasOpened;
        std::optional<Texture> m_attachedPicTexture;
        std::optional<std::future<bool>> m_copyFuture;
    };
};