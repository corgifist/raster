#include "merge_audio_samples.h"
#include "common/attribute_metadata.h"
#include "common/generic_audio_decoder.h"

namespace Raster {

    MergeAudioSamples::MergeAudioSamples() {
        NodeBase::Initialize();

        SetupAttribute("A", GenericAudioDecoder());
        SetupAttribute("B", GenericAudioDecoder());

        AddInputPin("A");
        AddInputPin("B");

        AddOutputPin("Output");
    }

    AbstractPinMap MergeAudioSamples::AbstractExecute(AbstractPinMap t_accumulator) {
        SharedLockGuard mixerGuard(m_mutex);
        AbstractPinMap result = {};
        
        auto aCandidate = GetAttribute<AudioSamples>("A");
        auto bCandidate = GetAttribute<AudioSamples>("B");
        auto contextData = GetContextData();
        auto& project = Workspace::GetProject();

        if (contextData.find("AUDIO_PASS") == contextData.end()) {
            auto cacheCandidate = m_cache.GetCachedSamples();
            if (cacheCandidate.has_value()) {
                TryAppendAbstractPinMap(result, "Output", cacheCandidate.value());
            }
            return result;
        }
        if (contextData.find("AUDIO_PASS") == contextData.end() || !project.playing) return {};
        
        if (aCandidate.has_value() && bCandidate.has_value()) {
            auto& a = aCandidate.value();
            auto& b = bCandidate.value();
            if (a.samples && b.samples) {
                SharedRawAudioSamples rawSamples = std::make_shared<std::vector<float>>(AudioInfo::s_periodSize * AudioInfo::s_channels);
                auto ptr = rawSamples->data();
                auto aPtr = a.samples->data();
                auto bPtr = b.samples->data();
                for (int i = 0; i < AudioInfo::s_channels * AudioInfo::s_periodSize; i++) {
                    ptr[i] = aPtr[i] + bPtr[i];
                }

                AudioSamples resultSamples = a;
                resultSamples.samples = rawSamples;
                for (auto& picture : b.attachedPictures) {
                    resultSamples.attachedPictures.push_back(picture);
                }
                m_cache.SetCachedSamples(resultSamples);
                TryAppendAbstractPinMap(result, "Output", resultSamples);
            }
        }

        return result;
    }

    void MergeAudioSamples::AbstractRenderProperties() {
        RenderAttributeProperty("A", {
            IconMetadata(ICON_FA_WAVE_SQUARE)
        });
        RenderAttributeProperty("B", {
            IconMetadata(ICON_FA_WAVE_SQUARE)
        });
    }

    void MergeAudioSamples::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);   
    }

    Json MergeAudioSamples::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool MergeAudioSamples::AbstractDetailsAvailable() {
        return false;
    }

    std::string MergeAudioSamples::AbstractHeader() {
        return "Merge Audio Samples";
    }

    std::string MergeAudioSamples::Icon() {
        return ICON_FA_VOLUME_HIGH " " ICON_FA_PLUS;
    }

    std::optional<std::string> MergeAudioSamples::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::MergeAudioSamples>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Merge Audio Samples",
            .packageName = RASTER_PACKAGED "merge_audio_samples",
            .category = Raster::DefaultNodeCategories::s_audio
        };
    }
}