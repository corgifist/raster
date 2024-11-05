#include "audio_waveform_sine.h"

#ifndef TAU_D
    #define TAU_D   6.28318530717958647693
#endif
#include "common/audio_info.h"

namespace Raster {

    AudioWaveformSine::AudioWaveformSine() {
        NodeBase::Initialize();

        SetupAttribute("Length", 1.0f);
        SetupAttribute("Amplitude", 1.0f);
        SetupAttribute("Phase", 0.0f);
        SetupAttribute("Advance", 0.0f);

        AddOutputPin("Output");
    }

    AbstractPinMap AudioWaveformSine::AbstractExecute(ContextData& t_contextData) {
        SharedLockGuard waveformGuard(m_mutex);
        AbstractPinMap result = {};
        
        auto lengthCandidate = GetAttribute<float>("Length", t_contextData);
        auto amplitudeCandidate = GetAttribute<float>("Amplitude", t_contextData);
        auto phaseCandidate = GetAttribute<float>("Phase", t_contextData);
        auto advanceCandidate = GetAttribute<float>("Advance", t_contextData);

        auto& project = Workspace::GetProject();


        if (t_contextData.find("AUDIO_PASS") == t_contextData.end()) {
            auto cacheCandidate = m_cache.GetCachedSamples();
            if (cacheCandidate.has_value()) {
                TryAppendAbstractPinMap(result, "Output", cacheCandidate.value());
            }
            return result;
        }
        if (t_contextData.find("AUDIO_PASS") == t_contextData.end() || !project.playing) return {};

        if (lengthCandidate.has_value() && amplitudeCandidate.has_value() && phaseCandidate.has_value() && advanceCandidate.has_value()) {
            auto length = lengthCandidate.value() * 5000;
            auto& amplitude = amplitudeCandidate.value();
            auto& phase = phaseCandidate.value();
            auto& advance = advanceCandidate.value();

            float currentTime = (project.GetCorrectCurrentTime() - Workspace::GetCompositionByNodeID(nodeID).value()->beginFrame + phase) / project.framerate;

            SharedRawAudioSamples resultSamples = AudioInfo::MakeRawAudioSamples();
            auto samplesPtr = resultSamples->data();
            for (int i = 0; i < AudioInfo::s_periodSize * AudioInfo::s_channels; i += AudioInfo::s_channels) {
                float s = (float)(sin(TAU_D * currentTime) * amplitude);
                currentTime += (1.0f / (float) AudioInfo::s_sampleRate) * length + advance * AudioInfo::s_sampleRate;
                for (int channel = 0; channel < AudioInfo::s_channels; channel++) {
                    samplesPtr[i + channel] = s;
                }
            }

            AudioSamples samples;
            samples.sampleRate = AudioInfo::s_sampleRate;
            samples.samples = resultSamples;
            m_cache.SetCachedSamples(samples);
            TryAppendAbstractPinMap(result, "Output", samples);
        }

        return result;
    }

    void AudioWaveformSine::AbstractRenderProperties() {
        RenderAttributeProperty("Length", {
            SliderStepMetadata(0.05f)
        });
        RenderAttributeProperty("Amplitude", {
            SliderRangeMetadata(0, 1)
        });
        RenderAttributeProperty("Phase", {
            SliderStepMetadata(0.1f)
        });
        RenderAttributeProperty("Advance", {
            SliderStepMetadata(0.1f)
        });
    }

    void AudioWaveformSine::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);   
    }

    Json AudioWaveformSine::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool AudioWaveformSine::AbstractDetailsAvailable() {
        return false;
    }

    std::string AudioWaveformSine::AbstractHeader() {
        return "Audio Waveform Sine";
    }

    std::string AudioWaveformSine::Icon() {
        return ICON_FA_WAVE_SQUARE;
    }

    std::optional<std::string> AudioWaveformSine::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::AudioWaveformSine>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Audio Waveform Sine",
            .packageName = RASTER_PACKAGED "audio_waveform_sine",
            .category = Raster::DefaultNodeCategories::s_audio
        };
    }
}