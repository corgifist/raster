#include "audio_waveform_sine.h"

#ifndef TAU_D
    #define TAU_D   6.28318530717958647693
#endif

namespace Raster {

    AudioWaveformSine::AudioWaveformSine() {
        NodeBase::Initialize();

        SetupAttribute("Length", 1.0f);
        SetupAttribute("Amplitude", 1.0f);
        SetupAttribute("Phase", 0.0f);
        SetupAttribute("Advance", 0.0f);

        AddOutputPin("Output");
    }

    AbstractPinMap AudioWaveformSine::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};
        
        auto lengthCandidate = GetAttribute<float>("Length");
        auto amplitudeCandidate = GetAttribute<float>("Amplitude");
        auto phaseCandidate = GetAttribute<float>("Phase");
        auto advanceCandidate = GetAttribute<float>("Advance");

        auto& project = Workspace::GetProject();

        auto contextData = GetContextData();
        if (contextData.find("AUDIO_PASS") == contextData.end() || !project.playing) return {};

        if (lengthCandidate.has_value() && amplitudeCandidate.has_value() && phaseCandidate.has_value() && advanceCandidate.has_value()) {
            auto length = lengthCandidate.value() * 5000;
            auto& amplitude = amplitudeCandidate.value();
            auto& phase = phaseCandidate.value();
            auto& advance = advanceCandidate.value();

            float currentTime = (project.GetCorrectCurrentTime() - Workspace::GetCompositionByNodeID(nodeID).value()->beginFrame + phase) / project.framerate;

            SharedRawAudioSamples resultSamples = std::make_shared<std::vector<float>>(Audio::s_samplesCount * Audio::GetChannelCount());
            auto samplesPtr = resultSamples->data();
            for (int i = 0; i < Audio::s_samplesCount * Audio::GetChannelCount(); i += Audio::GetChannelCount()) {
                float s = (float)(sin(TAU_D * currentTime) * amplitude);
                currentTime += (1.0f / (float) Audio::GetSampleRate()) * length + advance * Audio::GetSampleRate();
                for (int channel = 0; channel < Audio::GetChannelCount(); channel++) {
                    samplesPtr[i + channel] = s;
                }
            }

            AudioSamples samples;
            samples.sampleRate = Audio::GetSampleRate();
            samples.samples = resultSamples;
            TryAppendAbstractPinMap(result, "Output", samples);
        }

        return result;
    }

    void AudioWaveformSine::AbstractRenderProperties() {
        RenderAttributeProperty("Length");
        RenderAttributeProperty("Amplitude");
        RenderAttributeProperty("Phase");
        RenderAttributeProperty("Advance");
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