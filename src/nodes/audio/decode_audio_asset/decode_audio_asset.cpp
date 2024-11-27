#include "decode_audio_asset.h"
#include "common/asset_id.h"

#include "common/audio_info.h"
#include "raster.h"

namespace Raster {

    DecodeAudioAsset::DecodeAudioAsset() {
        NodeBase::Initialize();

        SetupAttribute("Asset", AssetID());
        SetupAttribute("Volume", 1.0f);

        AddOutputPin("Samples");

        MakePinPersistent("Samples");
    }

    DecodeAudioAsset::~DecodeAudioAsset() {
        DestroyPersistentPins();
    }

    AbstractPinMap DecodeAudioAsset::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};
        SharedLockGuard guard(m_decodingMutex);
        auto assetIDCandidate = GetAttribute<int>("Asset", t_contextData);
        auto volumeCandidate = GetAttribute<float>("Volume", t_contextData);

        auto& project = Workspace::GetProject();
        if (!RASTER_GET_CONTEXT_VALUE(t_contextData, "AUDIO_PASS", bool) || !RASTER_GET_CONTEXT_VALUE(t_contextData, "ALLOW_MEDIA_DECODING", bool)) {
            auto cacheCandidate = m_decoder.GetCachedSamples();
            if (cacheCandidate.has_value()) {
                TryAppendAbstractPinMap(result, "Samples", cacheCandidate.value());
            }
            return result;
        }

        if (assetIDCandidate.has_value()) {
            m_decoder.assetID = assetIDCandidate.value();
        }

        auto samplesCandidate = m_decoder.DecodeSamples(RASTER_GET_CONTEXT_VALUE(t_contextData, "AUDIO_PASS_ID", int));
        if (samplesCandidate.has_value()) {
            auto resampledSamples = samplesCandidate.value();
            auto samplesPtr = resampledSamples.samples->data();
            if (volumeCandidate.has_value()) {
                auto& volume = volumeCandidate.value();
                auto rawSamplesPtr = resampledSamples.samples->data();
                for (int i = 0; i < resampledSamples.samples->size(); i++) {
                    rawSamplesPtr[i] *= volume; 
                }
            } 

            TryAppendAbstractPinMap(result, "Samples", resampledSamples);
        }

        return result;
    }

    void DecodeAudioAsset::AbstractOnTimelineSeek() {
        auto& project = Workspace::GetProject();
        m_decoder.Seek((project.GetCorrectCurrentTime() - Workspace::GetCompositionByNodeID(nodeID).value()->beginFrame) / project.framerate);
    }

    std::optional<float> DecodeAudioAsset::AbstractGetContentDuration() {
        return m_decoder.GetContentDuration();
    }

    void DecodeAudioAsset::AbstractRenderProperties() {
        RenderAttributeProperty("Asset");
        RenderAttributeProperty("Volume", {
            FormatStringMetadata("%"),
            SliderRangeMetadata(0, 150),
            SliderBaseMetadata(100)
        });
    }

    void DecodeAudioAsset::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);   
    }

    Json DecodeAudioAsset::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool DecodeAudioAsset::AbstractDetailsAvailable() {
        return false;
    }

    std::string DecodeAudioAsset::AbstractHeader() {
        return "Decode Audio";
    }

    std::string DecodeAudioAsset::Icon() {
        return ICON_FA_WAVE_SQUARE;
    }

    std::optional<std::string> DecodeAudioAsset::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::DecodeAudioAsset>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Decode Audio Asset",
            .packageName = RASTER_PACKAGED "decode_audio_asset",
            .category = Raster::DefaultNodeCategories::s_audio
        };
    }
}