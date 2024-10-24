#include "mix_audio_samples.h"
#include "common/attribute_metadata.h"

namespace Raster {

    MixAudioSamples::MixAudioSamples() {
        NodeBase::Initialize();

        SetupAttribute("A", AudioSamples());
        SetupAttribute("B", AudioSamples());
        SetupAttribute("Phase", 0.5f);

        AddInputPin("A");
        AddInputPin("B");

        AddOutputPin("Output");
    }

    AbstractPinMap MixAudioSamples::AbstractExecute(AbstractPinMap t_accumulator) {
        SharedLockGuard mixerGuard(m_mutex);
        AbstractPinMap result = {};
        
        auto aCandidate = GetAttribute<AudioSamples>("A");
        auto bCandidate = GetAttribute<AudioSamples>("B");
        auto phaseCandidate = GetAttribute<float>("Phase");
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
        
        if (aCandidate.has_value() && bCandidate.has_value() && phaseCandidate.has_value()) {
            auto& a = aCandidate.value();
            auto& b = bCandidate.value();
            auto& phase = phaseCandidate.value();
            if (a.samples && b.samples) {
                SharedRawAudioSamples rawSamples = std::make_shared<std::vector<float>>(Audio::s_samplesCount * Audio::GetChannelCount());
                auto ptr = rawSamples->data();
                auto aPtr = a.samples->data();
                auto bPtr = b.samples->data();
                for (int i = 0; i < Audio::GetChannelCount() * Audio::s_samplesCount; i++) {
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