#include "reverb_effect.h"

 #define REVERB_BLOCK_SIZE 8192

namespace Raster {

    ReverbPrivate::ReverbPrivate() {
        this->initialized = false;
        this->wet = std::vector<float*>(2);
        
        this->wetGain = this->roomSize = this->reverberance = this->hfDamping =
        this->preDelay = this->stereoWidth = this->toneHigh = this->toneLow = 0;
    }

    ReverbContext::ReverbContext() {
        this->m_cachedSamples = std::make_shared<std::vector<float>>(4096 * Audio::GetChannelCount());
        this->health = MAX_BUFFER_LIFESPAN;
    }

    ReverbEffect::ReverbEffect() {
        NodeBase::Initialize();

        SetupAttribute("Samples", AudioSamples());
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

    AbstractPinMap ReverbEffect::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};

        auto samplesCandidate = GetAttribute<AudioSamples>("Samples");
        auto roomSizeCandidate = GetAttribute<float>("RoomSize");
        auto preDelayCandidate = GetAttribute<float>("PreDelay");
        auto reverberanceCandidate = GetAttribute<float>("Reverb");
        auto hfDampingCandidate = GetAttribute<float>("HfDamping");
        auto toneLowCandidate = GetAttribute<float>("ToneLow");
        auto toneHighCandidate = GetAttribute<float>("ToneHigh");
        auto wetGainCandidate = GetAttribute<float>("WetGain");
        auto dryGainCandidate = GetAttribute<float>("DryGain");
        auto stereoWidthCandidate = GetAttribute<float>("StereoWidth");
        auto wetOnlyCandidate = GetAttribute<bool>("WetOnly");
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

            auto& reverbBuffer = GetReverbContext();
            if (reverbBuffer.m_reverbs.empty()) {
                for (int i = 0; i < Audio::GetChannelCount(); i++) {
                    reverbBuffer.m_reverbs.push_back(std::make_shared<ManagedReverbPrivate>());

                    auto& reverb = reverbBuffer.m_reverbs[i];
                    reverb_create(&reverb.get()->reverb,
                        Audio::GetSampleRate(),
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
            for (int channel = 0; channel < Audio::GetChannelCount(); channel++) {
                auto& reverb = reverbBuffer.m_reverbs[channel];
                if (reverb->initialized) {
                    auto& r = reverb->reverb;
                    if (  reverb->wetGain != wetGain || reverb->roomSize != roomSize
                       || reverb->reverberance != reverberance || reverb->hfDamping != hfDamping
                       || reverb->preDelay != preDelay || reverb->stereoWidth != stereoWidth
                       || reverb->toneLow != toneLow || reverb->toneHigh != toneHigh) {
                        reverb_init(&r, Audio::GetSampleRate(), wetGain, roomSize, reverberance, hfDamping, preDelay, stereoWidth, toneLow, toneHigh);
                        
                        reverb->wetGain = wetGain; reverb->roomSize = roomSize;
                        reverb->reverberance = reverberance; reverb->hfDamping = hfDamping;
                        reverb->preDelay = preDelay; reverb->stereoWidth = stereoWidth;
                        reverb->toneLow = toneLow; reverb->toneHigh = toneHigh;
                        std::cout << "updating reverb" << std::endl;
                    }
                }
            }

            std::vector<float*> ichans(Audio::GetChannelCount());
            std::vector<float*> ochans(Audio::GetChannelCount());

            for (int i = 0; i < Audio::GetChannelCount(); i++) {
                ichans[i] = samples.samples->data() + i;
                ochans[i] = reverbBuffer.m_cachedSamples->data() + i;
            }

            float dryMult = wetOnly ? 0 : DecibelToLinear(dryGain);
            int remaining = Audio::s_samplesCount * Audio::GetChannelCount();
            while (remaining > 0) {
                int len = std::min(remaining, REVERB_BLOCK_SIZE);
                for (int c = 0; c < Audio::GetChannelCount(); c++) {
                    auto& reverb = reverbBuffer.m_reverbs[c];
                    reverb->dry = (float*) fifo_write(&reverb->reverb.input_fifo, len, ichans[c]);
                    reverb_process(&reverb->reverb, len);
                }

                if (Audio::GetChannelCount() == 2) {
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

                for (int c = 0; c < Audio::GetChannelCount(); c++) {
                    ichans[c] += len;
                    ochans[c] += len;
                }
            }

            AudioSamples reverbSamples = samples;
            reverbSamples.samples = reverbBuffer.m_cachedSamples;
            TryAppendAbstractPinMap(result, "Output", reverbSamples);
        }

        return result;
    }

    ReverbContext& ReverbEffect::GetReverbContext() {
        std::vector<float> deadReverbs;
        for (auto& reverb : m_reverbContexts) {
            reverb.second.health--;
            if (reverb.second.health < 0) {
                deadReverbs.push_back(reverb.first);
            }
        }

        for (auto& deadReverb : deadReverbs) {
            m_reverbContexts.erase(deadReverb);
        }

        auto& project = Workspace::GetProject();
        float offset = project.GetTimeTravelOffset();
        if (m_reverbContexts.find(offset) != m_reverbContexts.end()) {
            auto& reverb = m_reverbContexts[offset];
            reverb.health = MAX_BUFFER_LIFESPAN;
            return reverb;
        }

        ReverbContext context;
        m_reverbContexts[offset] = context;
        return m_reverbContexts[offset];
    }

    void ReverbEffect::AbstractRenderProperties() {
        RenderAttributeProperty("RoomSize");
        RenderAttributeProperty("PreDelay");
        RenderAttributeProperty("Reverb");
        RenderAttributeProperty("HfDamping");
        RenderAttributeProperty("ToneLow");
        RenderAttributeProperty("ToneHigh");
        RenderAttributeProperty("WetGain");
        RenderAttributeProperty("DryGain");
        RenderAttributeProperty("StereoWidth");
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