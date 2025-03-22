#include "mix_audio_samples.h"
#include "common/attribute_metadata.h"
#include "common/generic_audio_decoder.h"

namespace Raster {

    MixAudioSamples::MixAudioSamples() {
        NodeBase::Initialize();

        SetupAttribute("A", GenericAudioDecoder());
        SetupAttribute("B", GenericAudioDecoder());
        SetupAttribute("Phase", 0.5f);

        AddInputPin("A");
        AddInputPin("B");

        AddOutputPin("Output");
    }

    AbstractPinMap MixAudioSamples::AbstractExecute(ContextData& t_contextData) {
        SharedLockGuard mixerGuard(m_mutex);
        AbstractPinMap result = {};
        
        auto aCandidate = GetAttribute<AudioSamples>("A", t_contextData);
        auto bCandidate = GetAttribute<AudioSamples>("B", t_contextData);
        auto phaseCandidate = GetAttribute<float>("Phase", t_contextData);
        auto& project = Workspace::GetProject();

        if (RASTER_GET_CONTEXT_VALUE(t_contextData, "AUDIO_PASS", bool)) {
            auto cacheCandidate = m_cache.GetCachedSamples();
            if (cacheCandidate.has_value()) {
                TryAppendAbstractPinMap(result, "Output", cacheCandidate.value());
            }
            return result;
        }
        
        if (aCandidate.has_value() && bCandidate.has_value() && phaseCandidate.has_value()) {
            auto& a = aCandidate.value();
            auto& b = bCandidate.value();
            auto& phase = phaseCandidate.value();
            if (a.samples && b.samples) {
                SharedRawAudioSamples rawSamples = AudioInfo::MakeRawAudioSamples();
                auto ptr = rawSamples;
                auto aPtr = a.samples;
                auto bPtr = b.samples;
                for (int i = 0; i < AudioInfo::s_channels * AudioInfo::s_periodSize; i++) {
                    ptr[i] = glm::mix(aPtr[i], bPtr[i], phase);
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

    void MixAudioSamples::AbstractRenderProperties() {
        RenderAttributeProperty("A", {
            IconMetadata(ICON_FA_WAVE_SQUARE)
        });
        RenderAttributeProperty("B", {
            IconMetadata(ICON_FA_WAVE_SQUARE)
        });
        RenderAttributeProperty("Phase", {
            FormatStringMetadata("%"),
            SliderRangeMetadata(0, 100),
            SliderBaseMetadata(100)
        });
    }

    void MixAudioSamples::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);   
    }

    Json MixAudioSamples::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool MixAudioSamples::AbstractDetailsAvailable() {
        return false;
    }

    std::string MixAudioSamples::AbstractHeader() {
        return "Mix Audio Samples";
    }

    std::string MixAudioSamples::Icon() {
        return ICON_FA_VOLUME_HIGH " " ICON_FA_DROPLET;
    }

    std::optional<std::string> MixAudioSamples::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::MixAudioSamples>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Mix Audio Samples",
            .packageName = RASTER_PACKAGED "mix_audio_samples",
            .category = Raster::DefaultNodeCategories::s_audio
        };
    }
}