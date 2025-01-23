#include "echo_effect.h"
#include "common/audio_info.h"
#include "common/audio_samples.h"
#include "common/generic_audio_decoder.h"
#include "raster.h"
#include <memory>

namespace Raster {

    EchoBuffer::EchoBuffer() {
        this->health = MAX_BUFFER_LIFESPAN;
        this->samples = nullptr;
        this->pos = 0;
        this->len = 0;
    }

    EchoEffect::EchoEffect() {
        NodeBase::Initialize();

        SetupAttribute("Samples", GenericAudioDecoder());
        SetupAttribute("Delay", 0.5f);
        SetupAttribute("Decay", 0.5f);

        AddInputPin("Samples");
        AddOutputPin("Output");
    }

    AbstractPinMap EchoEffect::AbstractExecute(ContextData& t_contextData) {
        SharedLockGuard echoGuard(m_mutex);
        AbstractPinMap result = {};

        auto samplesCandidate = GetAttribute<AudioSamples>("Samples", t_contextData);
        auto delayCandidate = GetAttribute<float>("Delay", t_contextData);
        auto decayCandidate = GetAttribute<float>("Decay", t_contextData);

        auto& project = Workspace::GetProject();
        if (!RASTER_GET_CONTEXT_VALUE(t_contextData, "AUDIO_PASS", bool) && delayCandidate.has_value()) {
            auto delay = (1 + delayCandidate.value());
            auto& echoBuffer = *m_contexts.GetContext(delay, t_contextData);
            auto cacheCandidate = echoBuffer.cache.GetCachedSamples();
            if (cacheCandidate.has_value()) {
                TryAppendAbstractPinMap(result, "Output", cacheCandidate.value());
            }
            return result;
        }

        if (samplesCandidate.has_value() && delayCandidate.has_value() && decayCandidate.has_value() && samplesCandidate.value().samples) {
            auto& samples = samplesCandidate.value();
            auto delay = (1 + delayCandidate.value());
            auto& decay = decayCandidate.value();
            auto& reverbBuffer = *m_contexts.GetContext(delay, t_contextData);
            if (!reverbBuffer.samples) {
                reverbBuffer.samples = std::make_shared<std::vector<float>>(AudioInfo::s_periodSize * AudioInfo::s_channels * delay);
                reverbBuffer.len = AudioInfo::s_periodSize * AudioInfo::s_channels * delay;
                reverbBuffer.pos = 0;
            }

            auto inputBuffer = samples.samples->data();
            auto rawAudioSamples = AudioInfo::MakeRawAudioSamples();
            auto outputBuffer = rawAudioSamples->data();
            auto historyBuffer = reverbBuffer.samples->data();
            auto& pos = reverbBuffer.pos;
            for (int i = 0; i < AudioInfo::s_periodSize * AudioInfo::s_channels; i++, pos++) {
                if (pos >= reverbBuffer.len) {
                    pos = 0;
                }
                historyBuffer[pos] = outputBuffer[i] = inputBuffer[i] + historyBuffer[pos] * decay;
            }

            AudioSamples resultSamples = samples;
            resultSamples.samples = rawAudioSamples;
            reverbBuffer.cache.SetCachedSamples(resultSamples);
            TryAppendAbstractPinMap(result, "Output", resultSamples);
        } 

        return result;
    }

    void EchoEffect::AbstractRenderProperties() {
        RenderAttributeProperty("Samples", {
            IconMetadata(ICON_FA_WAVE_SQUARE)
        });
        RenderAttributeProperty("Delay", {
            FormatStringMetadata("seconds"),
            SliderStepMetadata(0.1),
            SliderRangeMetadata(0, 3)
        });
        RenderAttributeProperty("Decay", {
            SliderStepMetadata(0.1),
            SliderRangeMetadata(0, 1)
        });
    }

    void EchoEffect::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);   
    }

    Json EchoEffect::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool EchoEffect::AbstractDetailsAvailable() {
        return false;
    }

    std::string EchoEffect::AbstractHeader() {
        return "Echo Effect";
    }

    std::string EchoEffect::Icon() {
        return ICON_FA_VOLUME_HIGH;
    }

    std::optional<std::string> EchoEffect::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::EchoEffect>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Echo Effect",
            .packageName = RASTER_PACKAGED "echo_effect",
            .category = Raster::DefaultNodeCategories::s_audio
        };
    }
}