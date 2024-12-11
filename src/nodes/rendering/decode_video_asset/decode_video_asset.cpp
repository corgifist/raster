#include "decode_video_asset.h"
#include "common/asset_id.h"

#include "common/generic_video_decoder.h"
#include "common/localization.h"
#include "font/IconsFontAwesome5.h"
#include "gpu/gpu.h"
#include "raster.h"

namespace Raster {

    DecodeVideoAsset::DecodeVideoAsset() {
        NodeBase::Initialize();

        SetupAttribute("Asset", AssetID());

        AddOutputPin("Output");
    }

    DecodeVideoAsset::~DecodeVideoAsset() {
        m_decoder.Destroy();
    }

    AbstractPinMap DecodeVideoAsset::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};
        SharedLockGuard guard(m_decodingMutex);
        auto assetIDCandidate = GetAttribute<int>("Asset", t_contextData);

        auto& project = Workspace::GetProject();
        if (!RASTER_GET_CONTEXT_VALUE(t_contextData, "RENDERING_PASS", bool) || !RASTER_GET_CONTEXT_VALUE(t_contextData, "ALLOW_MEDIA_DECODING", bool)) {
            return result;
        }

        m_decoder.SetVideoAsset(assetIDCandidate.value_or(0));

        *m_decoder.seekTarget = project.currentFrame / project.framerate;
        VideoFramePrecision targetPrecision = VideoFramePrecision::Usual;
        int elementSize = 1;

        auto framerateCandidate = m_decoder.GetContentFramerate();
        if (framerateCandidate) {
            auto& framerate = *framerateCandidate;
            auto composition = *Workspace::GetCompositionByNodeID(nodeID);
            TexturePrecision targetTexturePrecision = TexturePrecision::Usual;

            float currentSeconds = (project.currentFrame - composition->beginFrame) / project.framerate;
            int currentVideoFrame = framerate * currentSeconds;

            m_decoder.targetPrecision = targetPrecision;
            auto decodingResult = m_decoder.DecodeFrame(m_imageAllocation, RASTER_GET_CONTEXT_VALUE(t_contextData, "RENDERING_PASS_ID", int), (project.currentFrame - composition->beginFrame) / project.framerate);
            if (decodingResult) {
                if (!m_videoTexture.handle || (m_videoTexture.width != m_imageAllocation.width || m_videoTexture.height != m_imageAllocation.height)) {
                    if (m_videoTexture.handle) {
                        GPU::DestroyTexture(m_videoTexture);
                    }
                    m_videoTexture = GPU::GenerateTexture(m_imageAllocation.width, m_imageAllocation.height, 4, targetTexturePrecision);
                }
                GPU::UpdateTexture(m_videoTexture, 0, 0, m_videoTexture.width, m_videoTexture.height, 4, m_imageAllocation.Get());
            }
        }

        if (m_videoTexture.handle) {
            TryAppendAbstractPinMap(result, "Output", m_videoTexture);
        }

        return result;
    }

    void DecodeVideoAsset::AbstractOnTimelineSeek() {
        auto& project = Workspace::GetProject();
        m_decoder.Seek((project.GetCorrectCurrentTime() - Workspace::GetCompositionByNodeID(nodeID).value()->beginFrame) / project.framerate);
    }

    std::optional<float> DecodeVideoAsset::AbstractGetContentDuration() {
        return m_decoder.GetContentDuration();
    }

    void DecodeVideoAsset::AbstractRenderProperties() {
        RenderAttributeProperty("Asset");
    }

    void DecodeVideoAsset::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);   
    }

    Json DecodeVideoAsset::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool DecodeVideoAsset::AbstractDetailsAvailable() {
        return false;
    }

    std::string DecodeVideoAsset::AbstractHeader() {
        return "Read Video";
    }

    std::string DecodeVideoAsset::Icon() {
        return ICON_FA_VIDEO;
    }

    std::optional<std::string> DecodeVideoAsset::Footer() {
        auto percentageCandidate = m_decoder.GetDecodingProgress();
        if (percentageCandidate) {
            return FormatString("%s: %i%%", Localization::GetString("DECODING_IN_PROGRESS").c_str(), std::clamp((int) (*percentageCandidate * 100), 0, 100));
        }
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::DecodeVideoAsset>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Decode Video Asset",
            .packageName = RASTER_PACKAGED "decode_video_asset",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    }
}