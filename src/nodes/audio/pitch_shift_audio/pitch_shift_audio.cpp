#include "pitch_shift_audio.h"
#include "audio/time_stretcher.h"
#include "common/audio_context_storage.h"
#include "common/audio_info.h"
#include "common/audio_samples.h"
#include "common/typedefs.h"
#include "raster.h"
#include <iomanip>
#include <memory>

namespace Raster {

    PitchShiftContext::PitchShiftContext() {
        this->stretcher = std::make_shared<TimeStretcher>(AudioInfo::s_sampleRate, AudioInfo::s_channels);
        this->waveformStretcher = std::make_shared<TimeStretcher>(AudioInfo::s_sampleRate, AudioInfo::s_channels);
        this->health = MAX_CONTEXT_HEALTH;
        this->seeked = true;
        this->highQualityPitch = false;
    }

    std::shared_ptr<TimeStretcher> PitchShiftContext::GetStretcher(ContextData& t_contextData) {
        return RASTER_GET_CONTEXT_VALUE(t_contextData, "WAVEFORM_PASS", bool) ? waveformStretcher : stretcher;
    }

    PitchShiftAudio::PitchShiftAudio() {
        NodeBase::Initialize();

        SetupAttribute("Samples", GenericAudioDecoder());
        SetupAttribute("PitchScale", 1.0f);
        SetupAttribute("UseHighQualityPitch", false);

        AddOutputPin("Output");
    }

    AbstractPinMap PitchShiftAudio::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};
        SharedLockGuard guard(m_mutex);
        auto samplesCandidate = GetAttribute<AudioSamples>("Samples", t_contextData);
        auto pitchScaleCandidate = GetAttribute<float>("PitchScale", t_contextData);
        auto useHighQualityPitchCandidate = GetAttribute<bool>("UseHighQualityPitch", t_contextData);

        auto& project = Workspace::GetProject();
        if (!RASTER_GET_CONTEXT_VALUE(t_contextData, "AUDIO_PASS", bool)) {
            auto pitchScaleContext = m_contexts.GetContext(project.GetTimeTravelOffset(), t_contextData);
            auto cacheCandidate = pitchScaleContext->cache.GetCachedSamples();
            if (cacheCandidate.has_value()) {
                TryAppendAbstractPinMap(result, "Output", cacheCandidate.value());
            }
            return result;
        }

        if (samplesCandidate.has_value() && pitchScaleCandidate.has_value() && useHighQualityPitchCandidate.has_value()) {
            auto& samples = samplesCandidate.value();
            auto pitchScale = pitchScaleCandidate.value();
            pitchScale = glm::abs(pitchScale);
            auto& useHighQualityPitch = useHighQualityPitchCandidate.value();
            auto context = m_contexts.GetContext(project.GetTimeTravelOffset(), t_contextData);
            context->GetStretcher(t_contextData)->UseHighQualityEngine(useHighQualityPitch);
            context->GetStretcher(t_contextData)->Validate();
            auto stretcher = context->GetStretcher(t_contextData);
            if (context->seeked || (RASTER_GET_CONTEXT_VALUE(t_contextData, "WAVEFORM_FIRST_PASS", bool))) {
                stretcher->Reset();
                context->seeked = false;
            }

            if (samples.samples) {
                auto& samplesVector = samples.samples;
                if (stretcher->GetPitchRatio() != pitchScale) {
                    stretcher->SetPitchRatio(pitchScale);
                }
   
                stretcher->Push(samples.samples);

                if (stretcher->AvailableSamples() >= AudioInfo::s_periodSize) {
                    auto pitchSamples = stretcher->Pop();

                    AudioSamples outputSamples = samples;
                    outputSamples.samples = pitchSamples;
                    context->cache.SetCachedSamples(outputSamples);
                    TryAppendAbstractPinMap(result, "Output", outputSamples);
                }
            }
        }

        return result;
    }

    void PitchShiftAudio::AbstractRenderProperties() {
        RenderAttributeProperty("Samples");
        RenderAttributeProperty("PitchScale", {
            SliderStepMetadata(0.01f)
        });
        RenderAttributeProperty("UseHighQualityPitch");
    }

    void PitchShiftAudio::AbstractOnTimelineSeek() {
        SharedLockGuard guard(m_mutex);
        for (auto& context : m_contexts.storage) {
            context.second->seeked = true;
        }
    }

    void PitchShiftAudio::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);   
    }

    Json PitchShiftAudio::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool PitchShiftAudio::AbstractDetailsAvailable() {
        return false;
    }

    std::string PitchShiftAudio::AbstractHeader() {
        return "Pitch Shift Audio";
    }

    std::string PitchShiftAudio::Icon() {
        return ICON_FA_UP_DOWN " " ICON_FA_VOLUME_HIGH;
    }

    std::optional<std::string> PitchShiftAudio::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::PitchShiftAudio>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Pitch Shift Audio",
            .packageName = RASTER_PACKAGED "pitch_shift_audio",
            .category = Raster::DefaultNodeCategories::s_audio
        };
    }
}