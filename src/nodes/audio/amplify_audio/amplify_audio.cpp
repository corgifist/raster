#include "amplify_audio.h"
#include "common/generic_audio_decoder.h"

namespace Raster {

    AmplifyAudio::AmplifyAudio() {
        NodeBase::Initialize();

        SetupAttribute("Samples", GenericAudioDecoder());
        SetupAttribute("Intensity", 1.0f);

        AddOutputPin("Output");
    }

    AbstractPinMap AmplifyAudio::AbstractExecute(ContextData& t_contextData) {
        SharedLockGuard amplifyGuard(m_mutex);
        AbstractPinMap result = {};
        auto& project = Workspace::GetProject();
        auto samplesCandidate = GetAttribute<AudioSamples>("Samples", t_contextData);
        auto intensityCandidate = GetAttribute<float>("Intensity", t_contextData);
        if (RASTER_GET_CONTEXT_VALUE(t_contextData, "AUDIO_PASS", bool)) {
            auto cacheCandidate = m_cache.GetCachedSamples();
            if (cacheCandidate.has_value()) {
                TryAppendAbstractPinMap(result, "Output", cacheCandidate.value());
            }
            return result;
        }

        if (samplesCandidate.has_value() && intensityCandidate.has_value()) {
            auto& samples = samplesCandidate.value();
            auto& intensity = intensityCandidate.value();

            auto resultSamplesVector = AudioInfo::MakeRawAudioSamples();
            auto resultSamplesPtr = resultSamplesVector->data();
            auto originalSamplesPtr = samples.samples->data();
            for (int i = 0; i < resultSamplesVector->size(); i++) {
                resultSamplesPtr[i] = originalSamplesPtr[i] * intensity;
            }

            AudioSamples resultSamples = samples;
            resultSamples.samples = resultSamplesVector;
            m_cache.SetCachedSamples(resultSamples);
            TryAppendAbstractPinMap(result, "Output", resultSamples);
        }

        return result;
    }

    void AmplifyAudio::AbstractRenderProperties() {
        RenderAttributeProperty("Samples", {
            IconMetadata(ICON_FA_WAVE_SQUARE)
        });
        RenderAttributeProperty("Intensity", {
            FormatStringMetadata("%"),
            SliderBaseMetadata(100),
            SliderRangeMetadata(0, 200)
        });
    }

    void AmplifyAudio::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);   
    }

    Json AmplifyAudio::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool AmplifyAudio::AbstractDetailsAvailable() {
        return false;
    }

    std::string AmplifyAudio::AbstractHeader() {
        return "Amplify Audio";
    }

    std::string AmplifyAudio::Icon() {
        return ICON_FA_VOLUME_HIGH " " ICON_FA_PLUS;
    }

    std::optional<std::string> AmplifyAudio::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::AmplifyAudio>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Amplify Audio",
            .packageName = RASTER_PACKAGED "amplify_audio",
            .category = Raster::DefaultNodeCategories::s_audio
        };
    }
}