#include "echo_effect.h"

namespace Raster {

    EchoBuffer::EchoBuffer() {
        this->samples = nullptr;
        this->outputBuffer = nullptr;
        this->health = MAX_BUFFER_LIFESPAN;
        this->pos = 0;
        this->len = 0;
    }

    EchoEffect::EchoEffect() {
        NodeBase::Initialize();

        SetupAttribute("Samples", AudioSamples());
        SetupAttribute("Delay", 0.5f);
        SetupAttribute("Decay", 0.5f);

        AddInputPin("Samples");
        AddOutputPin("Output");
    }

    AbstractPinMap EchoEffect::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};

        auto samplesCandidate = GetAttribute<AudioSamples>("Samples");
        auto delayCandidate = GetAttribute<float>("Delay");
        auto decayCandidate = GetAttribute<float>("Decay");

        if (samplesCandidate.has_value() && delayCandidate.has_value() && decayCandidate.has_value() && samplesCandidate.value().samples) {
            auto& samples = samplesCandidate.value();
            auto delay = (1 + delayCandidate.value());
            auto& decay = decayCandidate.value();
            auto& reverbBuffer = GetEchoBuffer(delay);

            auto inputBuffer = samples.samples->data();
            auto outputBuffer = reverbBuffer.outputBuffer->data();
            auto historyBuffer = reverbBuffer.samples->data();
            auto& pos = reverbBuffer.pos;
            for (int i = 0; i < Audio::s_samplesCount * Audio::GetChannelCount(); i++, pos++) {
                if (pos == reverbBuffer.len) {
                    pos = 0;
                }
                historyBuffer[pos] = outputBuffer[i] = inputBuffer[i] + historyBuffer[pos] * decay;
            }

            AudioSamples resultSamples = samples;
            resultSamples.samples = reverbBuffer.outputBuffer;
            TryAppendAbstractPinMap(result, "Output", resultSamples);
        } 

        return result;
    }

    void EchoEffect::AbstractRenderProperties() {
        RenderAttributeProperty("Delay");
        RenderAttributeProperty("Decay");
    }

    EchoBuffer& EchoEffect::GetEchoBuffer(float t_delay) {
        std::vector<float> deadBuffers;
        for (auto& buffer : m_reverbBuffers) {
            buffer.second.health--;
            if (buffer.second.health < 0) {
                deadBuffers.push_back(buffer.first);
            }
        }

        for (auto& deadBuffer : deadBuffers) {
            m_reverbBuffers.erase(deadBuffer);
        }

        if (m_reverbBuffers.find(t_delay) != m_reverbBuffers.end()) {
            auto& buffer = m_reverbBuffers[t_delay];
            buffer.health = MAX_BUFFER_LIFESPAN;
            return buffer;
        }

        EchoBuffer buffer;
        buffer.samples = std::make_shared<std::vector<float>>(Audio::GetSampleRate() * Audio::GetChannelCount() * t_delay);
        buffer.outputBuffer = std::make_shared<std::vector<float>>(4096 * Audio::GetChannelCount());
        buffer.pos = 0;
        buffer.len = Audio::GetSampleRate() * Audio::GetChannelCount() * t_delay;
        buffer.health = MAX_BUFFER_LIFESPAN;
        m_reverbBuffers[t_delay] = buffer;
        return m_reverbBuffers[t_delay];
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