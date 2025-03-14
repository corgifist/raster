#pragma once

#include "common/assets.h"
#include "gpu/gpu.h"
#include "../../ImGui/imgui.h"
#include "../../ImGui/imgui_internal.h"
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

    struct AudioStreamInfo {
        int sampleRate;
        std::string sampleFormatName;
        std::string codecName;
        int bitrate;
    };

    struct VideoStreamInfo {
        int width, height;
        std::string codecName;
        std::string pixelFormatName;
        int bitrate;
        bool attachedPic;
    };

    using StreamInfo = std::variant<AudioStreamInfo, VideoStreamInfo>;

    struct AudioWaveformData {
        std::shared_ptr<std::vector<std::vector<float>>> streamData;

        AudioWaveformData() : streamData(nullptr) {}
    };

    struct MediaAsset : public AssetBase {
    public:
        MediaAsset();

    private:
        static AudioWaveformData CalculateWaveformsForPath(std::string t_path);

        bool AbstractIsReady();

        void AbstractImport(std::string t_path);
        void AbstractDelete();
        std::optional<Texture> AbstractGetPreviewTexture();

        void AbstractLoad(Json t_data);
        Json AbstractSerialize();

        void AbstractRenderDetails();
        void AbstractRenderPopup();

        std::optional<std::string> AbstractGetResolution();
        std::optional<std::string> AbstractGetPath();
        std::optional<std::string> AbstractGetDuration();
        std::optional<uintmax_t> AbstractGetSize();

        void AbstractRenderPreviewOverlay(glm::vec2 t_regionSize);
        void AbstractOnTimelineDrop(float t_frame);

        std::string m_relativePath;
        std::string m_originalPath;

        bool m_formatCtxWasOpened;
        std::optional<Texture> m_attachedPicTexture;
        std::vector<StreamInfo> m_streamInfos;
        std::vector<std::pair<std::string, std::string>> m_metadata;
        std::optional<std::future<bool>> m_copyFuture;
        std::optional<std::uintmax_t> m_cachedSize;
        std::optional<float> m_cachedDuration;
        std::optional<AudioWaveformData> m_waveformData;
        std::optional<std::future<AudioWaveformData>> m_waveformFuture;
        int m_selectedWaveform;
    };
};