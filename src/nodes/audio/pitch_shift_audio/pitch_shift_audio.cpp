#include "pitch_shift_audio.h"

namespace Raster {

    PitchShiftContext::PitchShiftContext() {
        this->stretcher = nullptr;
        this->health = MAX_PITCH_SHIFT_CONTEXT_HEALTH;
        this->seeked = true;
    }

    PitchShiftAudio::PitchShiftAudio() {
        NodeBase::Initialize();

        SetupAttribute("Samples", GenericAudioDecoder());
        SetupAttribute("PitchScale", 1.0f);

        AddOutputPin("Output");
    }

    AbstractPinMap PitchShiftAudio::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};
        SharedLockGuard guard(m_mutex);
        auto samplesCandidate = GetAttribute<AudioSamples>("Samples", t_contextData);
        auto pitchScaleCandidate = GetAttribute<float>("PitchScale", t_contextData);

        auto& project = Workspace::GetProject();
        if (t_contextData.find("AUDIO_PASS") == t_contextData.end()) {
            auto pitchScaleContext = GetContext();
            auto cacheCandidate = pitchScaleContext->cache.GetCachedSamples();
            if (cacheCandidate.has_value()) {
                TryAppendAbstractPinMap(result, "Output", cacheCandidate.value());
            }
            return result;
        }

        if (samplesCandidate.has_value() && pitchScaleCandidate.has_value()) {
            auto& samples = samplesCandidate.value();
            auto pitchScale = pitchScaleCandidate.value();
            pitchScale = glm::abs(pitchScale);
            auto context = GetContext();
            if (!context->stretcher) {
                context->stretcher = std::make_shared<RubberBand::RubberBandStretcher>(AudioInfo::s_sampleRate, AudioInfo::s_channels, 
                                                                                    RubberBand::RubberBandStretcher::OptionEngineFaster 
                                                                                    | RubberBand::RubberBandStretcher::OptionProcessRealTime 
                                                                                    | RubberBand::RubberBandStretcher::OptionPitchHighSpeed
                                                                                    | RubberBand::RubberBandStretcher::OptionThreadingNever);
            }
            auto& stretcher = context->stretcher;
            if (context->seeked) {
                stretcher->reset();
                context->seeked = false;
            }

            if (samples.samples) {
                auto& samplesVector = samples.samples;
                stretcher->setTimeRatio(1);
                stretcher->setPitchScale(pitchScale);
   
                {
                    static SharedRawDeinterleavedAudioSamples s_planarSamples = MakeDeinterleavedAudioSamples(AudioInfo::s_periodSize, AudioInfo::s_channels);
                    DeinterleaveAudioSamples(samples.samples, s_planarSamples, AudioInfo::s_periodSize, AudioInfo::s_channels);

                    std::vector<float*> planarBuffersData;
                    for (int i = 0; i < s_planarSamples->size(); i++) {
                        planarBuffersData.push_back(s_planarSamples->at(i).data());
                    }
                    stretcher->process(planarBuffersData.data(), AudioInfo::s_periodSize, false);
                };

                {
                    static SharedRawDeinterleavedAudioSamples s_outputPlanarBuffers = MakeDeinterleavedAudioSamples(AudioInfo::s_periodSize, AudioInfo::s_channels);

                    std::vector<float*> planarBuffersData;
                    for (int i = 0; i < s_outputPlanarBuffers->size(); i++) {
                        planarBuffersData.push_back(s_outputPlanarBuffers->at(i).data());
                    }
                    if (stretcher->available() >= AudioInfo::s_periodSize) {
                        stretcher->retrieve(planarBuffersData.data(), AudioInfo::s_periodSize);

                        SharedRawInterleavedAudioSamples resultInterleavedBuffer = MakeInterleavedAudioSamples(AudioInfo::s_periodSize, AudioInfo::s_channels);
                        InterleaveAudioSamples(s_outputPlanarBuffers, resultInterleavedBuffer, AudioInfo::s_periodSize, AudioInfo::s_channels);
                        AudioSamples outputSamples = samples;
                        outputSamples.samples = resultInterleavedBuffer;
                        context->cache.SetCachedSamples(outputSamples);
                        TryAppendAbstractPinMap(result, "Output", outputSamples);
                    }
                };

            }
        }

        return result;
    }

    SharedPitchShiftContext PitchShiftAudio::GetContext() {
        std::vector<float> deadReverbs;
        for (auto& reverb : m_contexts) {
            reverb.second->health--;
            if (reverb.second->health < 0) {
                deadReverbs.push_back(reverb.first);
            }
        }

        for (auto& deadReverb : deadReverbs) {
            m_contexts.erase(deadReverb);
        }

        auto& project = Workspace::GetProject();
        float offset = project.GetTimeTravelOffset();
        if (m_contexts.find(offset) != m_contexts.end()) {
            auto& reverb = m_contexts[offset];
            reverb->health = MAX_PITCH_SHIFT_CONTEXT_HEALTH;
            return reverb;
        }

        SharedPitchShiftContext context = std::make_shared<PitchShiftContext>();
        m_contexts[offset] = context;
        return m_contexts[offset];
    }

    void PitchShiftAudio::AbstractRenderProperties() {
        RenderAttributeProperty("Samples");
        RenderAttributeProperty("PitchScale", {
            SliderStepMetadata(0.01f)
        });
    }

    void PitchShiftAudio::AbstractOnTimelineSeek() {
        SharedLockGuard guard(m_mutex);
        for (auto& context : m_contexts) {
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