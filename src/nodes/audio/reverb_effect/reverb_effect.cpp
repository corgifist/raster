#include "reverb_effect.h"
#include "common/attribute_metadata.h"
#include "common/generic_audio_decoder.h"
#include "raster.h"

#define REVERB_BLOCK_SIZE 16384

namespace Raster {

    ReverbPrivate::ReverbPrivate() {
        this->initialized = false;
        this->wet = std::vector<float*>(2);
        
        this->wetGain = this->roomSize = this->reverberance = this->hfDamping =
        this->preDelay = this->stereoWidth = this->toneHigh = this->toneLow = 0;
    }

    ReverbContext::ReverbContext() {
        this->health = MAX_BUFFER_LIFESPAN;
    }

    ReverbEffect::ReverbEffect() {
        NodeBase::Initialize();

        SetupAttribute("Samples", GenericAudioDecoder());
        SetupAttribute("RoomSize", 70.0f);
        SetupAttribute("PreDelay", 20.0f);
        SetupAttribute("Reverb", 40.0f);
        SetupAttribute("HfDamping", 99.0f);
        SetupAttribute("ToneLow", 100.0f);
        SetupAttribute("ToneHigh", 50.0f);
        SetupAttribute("WetGain", -12.0f);
        SetupAttribute("DryGain", 0.0f);
        SetupAttribute("StereoWidth", 70.0f);
        SetupAttribute("WetOnly", false);

        AddInputPin("Samples");
        AddOutputPin("Output"); 
    }

    AbstractPinMap ReverbEffect::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};
        SharedLockGuard reverbGuard(m_mutex);

        auto samplesCandidate = GetAttribute<AudioSamples>("Samples", t_contextData);
        auto roomSizeCandidate = GetAttribute<float>("RoomSize", t_contextData);
        auto preDelayCandidate = GetAttribute<float>("PreDelay", t_contextData);
        auto reverberanceCandidate = GetAttribute<float>("Reverb", t_contextData);
        auto hfDampingCandidate = GetAttribute<float>("HfDamping", t_contextData);
        auto toneLowCandidate = GetAttribute<float>("ToneLow", t_contextData);
        auto toneHighCandidate = GetAttribute<float>("ToneHigh", t_contextData);
        auto wetGainCandidate = GetAttribute<float>("WetGain", t_contextData);
        auto dryGainCandidate = GetAttribute<float>("DryGain", t_contextData);
        auto stereoWidthCandidate = GetAttribute<float>("StereoWidth", t_contextData);
        auto wetOnlyCandidate = GetAttribute<bool>("WetOnly", t_contextData);

        auto& project = Workspace::GetProject();
        if (!RASTER_GET_CONTEXT_VALUE(t_contextData, "AUDIO_PASS", bool)) {
            auto& reverbBuffer = *m_contexts.GetContext(project.GetTimeTravelOffset(), t_contextData);
            auto cacheCandidate = reverbBuffer.cache.GetCachedSamples();
            if (cacheCandidate.has_value()) {
                TryAppendAbstractPinMap(result, "Output", cacheCandidate.value());
            }
            return result;
        }

        if (samplesCandidate.has_value() && samplesCandidate.value().samples && roomSizeCandidate.has_value() 
            && preDelayCandidate.has_value() && reverberanceCandidate.has_value() && hfDampingCandidate.has_value()
            && toneLowCandidate.has_value() && toneHighCandidate.has_value() && wetGainCandidate.has_value() 
            && dryGainCandidate.has_value() && stereoWidthCandidate.has_value() && wetOnlyCandidate.has_value()) {
            auto& samples = samplesCandidate.value();
            auto& roomSize = roomSizeCandidate.value();
            auto& preDelay = preDelayCandidate.value();
            auto& hfDamping = hfDampingCandidate.value();
            auto& reverberance = reverberanceCandidate.value();
            auto& toneLow = toneLowCandidate.value();
            auto& toneHigh = toneHighCandidate.value();
            auto& wetGain = wetGainCandidate.value();
            auto& dryGain = dryGainCandidate.value();
            auto& stereoWidth = stereoWidthCandidate.value();
            auto& wetOnly = wetOnlyCandidate.value();

            auto& reverbBuffer = *m_contexts.GetContext(project.GetTimeTravelOffset(), t_contextData);
            if (reverbBuffer.m_reverbs.empty()) {
                for (int i = 0; i < AudioInfo::s_channels; i++) {
                    reverbBuffer.m_reverbs.push_back(std::make_shared<ManagedReverbPrivate>());

                    auto& reverb = reverbBuffer.m_reverbs[i];
                    reverb_create(&reverb.get()->reverb,
                        AudioInfo::s_sampleRate,
                        wetGain, roomSize, reverberance,
                        hfDamping, preDelay, stereoWidth,
                        toneLow, toneHigh, REVERB_BLOCK_SIZE, reverb->wet.data()
                    );
                    reverb->initialized = true;
                    reverb->wetGain = wetGain; reverb->roomSize = roomSize;
                    reverb->reverberance = reverberance; reverb->hfDamping = hfDamping;
                    reverb->preDelay = preDelay; reverb->stereoWidth = stereoWidth;
                    reverb->toneLow = toneLow; reverb->toneHigh = toneHigh;
                }
            }

            // making sure reverbs are up-to-date
            for (int channel = 0; channel < AudioInfo::s_channels; channel++) {
                auto& reverb = reverbBuffer.m_reverbs[channel];
                if (reverb->initialized) {
                    auto& r = reverb->reverb;
                    if (  reverb->wetGain != wetGain || reverb->roomSize != roomSize
                       || reverb->reverberance != reverberance || reverb->hfDamping != hfDamping
                       || reverb->preDelay != preDelay || reverb->stereoWidth != stereoWidth
                       || reverb->toneLow != toneLow || reverb->toneHigh != toneHigh) {
                        reverb_init(&r, AudioInfo::s_sampleRate, wetGain, roomSize, reverberance, hfDamping, preDelay, stereoWidth, toneLow, toneHigh);
                        
                        reverb->wetGain = wetGain; reverb->roomSize = roomSize;
                        reverb->reverberance = reverberance; reverb->hfDamping = hfDamping;
                        reverb->preDelay = preDelay; reverb->stereoWidth = stereoWidth;
                        reverb->toneLow = toneLow; reverb->toneHigh = toneHigh;
                    }
                }
            }

            std::vector<float*> ichans(AudioInfo::s_channels);
            std::vector<float*> ochans(AudioInfo::s_channels);

            SharedRawAudioSamples outputSamples = AudioInfo::MakeRawAudioSamples();
            for (int i = 0; i < AudioInfo::s_channels; i++) {
                ichans[i] = samples.samples + i;
                ochans[i] = outputSamples + i;
            }

            float dryMult = wetOnly ? 0 : DecibelToLinear(dryGain);
            int remaining = AudioInfo::s_periodSize * AudioInfo::s_channels;
            while (remaining > 0) {
                int len = std::min(remaining, REVERB_BLOCK_SIZE);
                for (int c = 0; c < AudioInfo::s_channels; c++) {
                    auto& reverb = reverbBuffer.m_reverbs[c];
                    reverb->dry = (float*) fifo_write(&reverb->reverb.input_fifo, len, ichans[c]);
                    reverb_process(&reverb->reverb, len);
                }

                if (AudioInfo::s_channels == 2) {
                    for (int i = 0; i < len; i++) {
                        for (int w = 0; w < 2; w++) {
                            auto& reverb = reverbBuffer.m_reverbs[w];
                            ochans[w][i] = dryMult * reverb->dry[i] + 0.5 * (reverbBuffer.m_reverbs[0]->wet[w][i] + reverbBuffer.m_reverbs[1]->wet[w][i]);
                        }
                    }
                } else {
                    for (int i = 0; i < len; i++) {
                        ochans[0][i] = dryMult * reverbBuffer.m_reverbs[0]->dry[i] + reverbBuffer.m_reverbs[0]->wet[0][i];
                    }
                }

                remaining -= len;

                for (int c = 0; c < AudioInfo::s_channels; c++) {
                    ichans[c] += len;
                    ochans[c] += len;
                }
            }

            AudioSamples reverbSamples = samples;
            reverbSamples.samples = outputSamples;

            reverbBuffer.cache.SetCachedSamples(reverbSamples);
            TryAppendAbstractPinMap(result, "Output", reverbSamples);
        }

        return result;
    }

    void ReverbEffect::AbstractRenderProperties() {
        RenderAttributeProperty("Samples", {
            IconMetadata(ICON_FA_WAVE_SQUARE)
        });
        
        RenderAttributeProperty("RoomSize", {
            FormatStringMetadata("%"),
            SliderRangeMetadata(0, 100)
        });

        RenderAttributeProperty("PreDelay", {
            FormatStringMetadata("ms"),
            SliderRangeMetadata(0, 100)
        });

        RenderAttributeProperty("Reverb", {
            FormatStringMetadata("%"),
            SliderRangeMetadata(0, 100)
        });

        RenderAttributeProperty("HfDamping", {
            FormatStringMetadata("%"),
            SliderRangeMetadata(0, 100)
        });

        RenderAttributeProperty("ToneLow", {
            FormatStringMetadata("%"),
            SliderRangeMetadata(0, 100)
        });

        RenderAttributeProperty("ToneHigh", {
            FormatStringMetadata("%"),
            SliderRangeMetadata(0, 100)
        });

        RenderAttributeProperty("WetGain", {
            FormatStringMetadata("dB"),
            SliderRangeMetadata(-60, 30)
        });

        RenderAttributeProperty("DryGain", {
            FormatStringMetadata("dB"),
            SliderRangeMetadata(-60, 30)
        });

        RenderAttributeProperty("StereoWidth", {
            FormatStringMetadata("%"),
            SliderRangeMetadata(0, 100)
        });

        RenderAttributeProperty("WetOnly");

    }

    void ReverbEffect::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);   
    }

    Json ReverbEffect::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool ReverbEffect::AbstractDetailsAvailable() {
        return false;
    }

    std::string ReverbEffect::AbstractHeader() {
        return "Reverb Effect";
    }

    std::string ReverbEffect::Icon() {
        return ICON_FA_VOLUME_HIGH;
    }

    std::optional<std::string> ReverbEffect::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::ReverbEffect>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Reverb Effect",
            .packageName = RASTER_PACKAGED "reverb_time",
            .category = Raster::DefaultNodeCategories::s_audio
        };
    }
}