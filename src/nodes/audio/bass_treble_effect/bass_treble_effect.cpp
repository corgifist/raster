#include "bass_treble_effect.h"
#include "common/generic_audio_decoder.h"

#define SLOPE 0.4f
#define BASS_HZ 250.0f
#define TREBLE_HZ 4000.0f

#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

namespace Raster {

    BassTrebleCachedData::BassTrebleCachedData() {
        samplerate = AudioInfo::s_sampleRate;
        slope = 0.4f;   // same slope for both filters
        hzBass = 250.0f;   // could be tunable in a more advanced version
        hzTreble = 4000.0f;   // could be tunable in a more advanced version

        a0Bass = 1;
        a1Bass = 0;
        a2Bass = 0;
        b0Bass = 0;
        b1Bass = 0;
        b2Bass = 0;

        a0Treble = 1;
        a1Treble = 0;
        a2Treble = 0;
        b0Treble = 0;
        b1Treble = 0;
        b2Treble = 0;

        xn1Bass = 0;
        xn2Bass = 0;
        yn1Bass = 0;
        yn2Bass = 0;

        xn1Treble = 0;
        xn2Treble = 0;
        yn1Treble = 0;
        yn2Treble = 0;

        bass = -1;
        treble = -1;
        gain = 0;
    }

    BassTrebleEffect::BassTrebleEffect() {
        NodeBase::Initialize();

        SetupAttribute("Samples", GenericAudioDecoder());
        SetupAttribute("Bass", 0.0f);
        SetupAttribute("Treble", 0.0f);
        SetupAttribute("Gain", 0.0f);

        AddInputPin("Samples");
        AddOutputPin("Output");
    }

    AbstractPinMap BassTrebleEffect::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};

        auto samplesCandidate = GetAttribute<AudioSamples>("Samples", t_contextData);
        auto bassCandidate = GetAttribute<float>("Bass", t_contextData);
        auto trebleCandidate = GetAttribute<float>("Treble", t_contextData);
        auto gainCandidate = GetAttribute<float>("Gain", t_contextData);

        auto& project = Workspace::GetProject();
        if (RASTER_GET_CONTEXT_VALUE(t_contextData, "AUDIO_PASS", bool)) {
            auto cacheCandidate = m_cache.GetCachedSamples();
            if (cacheCandidate.has_value()) {
                TryAppendAbstractPinMap(result, "Output", cacheCandidate.value());
            }
            return result;
        }
        if (samplesCandidate.has_value() && bassCandidate.has_value() && trebleCandidate.has_value() && gainCandidate.has_value()) {
            auto& samples = samplesCandidate.value();
            auto& bass = bassCandidate.value();
            auto& treble = trebleCandidate.value();
            auto& gain = gainCandidate.value();

            if (samples.samples) {
                auto linearBass = DecibelToLinear(bass);
                auto linearTreble = DecibelToLinear(treble);
                auto linearGain = DecibelToLinear(gain);

                m_data.gain = linearGain;
                if (m_data.bass != linearBass) {
                    BassCoefficients(m_data.hzBass, m_data.slope, bass, m_data.samplerate,
                                m_data.a0Bass, m_data.a1Bass, m_data.a2Bass,
                                m_data.b0Bass, m_data.b1Bass, m_data.b2Bass);
                }

                if (m_data.treble != linearTreble) {
                    TrebleCoefficients(m_data.hzTreble, m_data.slope, treble, m_data.samplerate,
                                m_data.a0Treble, m_data.a1Treble, m_data.a2Treble,
                                m_data.b0Treble, m_data.b1Treble, m_data.b2Treble);
                }

                auto rawSamples = samples.samples;
                auto rawSamplesVector = AudioInfo::MakeRawAudioSamples();
                auto rawCachedSamples = rawSamplesVector;
                for (int i = 0; i < AudioInfo::s_periodSize * AudioInfo::s_channels; i++) {
                    float in = rawSamples[i];
                    // Bass filter
                    float out = (m_data.b0Bass * in + m_data.b1Bass * m_data.xn1Bass + m_data.b2Bass * m_data.xn2Bass -
                            m_data.a1Bass * m_data.yn1Bass - m_data.a2Bass * m_data.yn2Bass) / m_data.a0Bass;
                    m_data.xn2Bass = m_data.xn1Bass;
                    m_data.xn1Bass = in;
                    m_data.yn2Bass = m_data.yn1Bass;
                    m_data.yn1Bass = out;

                    // Treble filter
                    in = out;
                    out = (m_data.b0Treble * in + m_data.b1Treble * m_data.xn1Treble + m_data.b2Treble * m_data.xn2Treble -
                            m_data.a1Treble * m_data.yn1Treble - m_data.a2Treble * m_data.yn2Treble) / m_data.a0Treble;
                    m_data.xn2Treble = m_data.xn1Treble;
                    m_data.xn1Treble = in;
                    m_data.yn2Treble = m_data.yn1Treble;
                    m_data.yn1Treble = out;

                    rawCachedSamples[i] = out * m_data.gain;
                }
                
                AudioSamples resultSamples = samples;
                resultSamples.samples = rawSamplesVector;
                m_cache.SetCachedSamples(resultSamples);
                TryAppendAbstractPinMap(result, "Output", resultSamples);
            }
        }

        return result;
    }

    void BassTrebleEffect::AbstractRenderProperties() {
        RenderAttributeProperty("Samples", {
            IconMetadata(ICON_FA_WAVE_SQUARE)
        });
        RenderAttributeProperty("Bass", {
            FormatStringMetadata("dB"),
            SliderStepMetadata(0.05f)
        });
        RenderAttributeProperty("Treble", {
            FormatStringMetadata("dB"),
            SliderStepMetadata(0.05f)
        });
        RenderAttributeProperty("Gain", {
            FormatStringMetadata("dB"),
            SliderStepMetadata(0.05f)
        });
    }

    void BassTrebleEffect::BassCoefficients(double hz, double slope, double gain, double samplerate,
                                   double& a0, double& a1, double& a2,
                                   double& b0, double& b1, double& b2) {
        double w = 2 * M_PI * hz / samplerate;
        double a = exp(log(10.0) * gain / 40);
        double b = sqrt((a * a + 1) / slope - (pow((a - 1), 2)));

        b0 = a * ((a + 1) - (a - 1) * cos(w) + b * sin(w));
        b1 = 2 * a * ((a - 1) - (a + 1) * cos(w));
        b2 = a * ((a + 1) - (a - 1) * cos(w) - b * sin(w));
        a0 = ((a + 1) + (a - 1) * cos(w) + b * sin(w));
        a1 = -2 * ((a - 1) + (a + 1) * cos(w));
        a2 = (a + 1) + (a - 1) * cos(w) - b * sin(w);
    }

    void BassTrebleEffect::TrebleCoefficients(double hz, double slope, double gain, double samplerate, 
                                   double& a0, double& a1, double& a2,
                                   double& b0, double& b1, double& b2) {
        double w = 2 * M_PI * hz / samplerate;
        double a = exp(log(10.0) * gain / 40);
        double b = sqrt((a * a + 1) / slope - (pow((a - 1), 2)));

        b0 = a * ((a + 1) + (a - 1) * cos(w) + b * sin(w));
        b1 = -2 * a * ((a - 1) + (a + 1) * cos(w));
        b2 = a * ((a + 1) + (a - 1) * cos(w) - b * sin(w));
        a0 = ((a + 1) - (a - 1) * cos(w) + b * sin(w));
        a1 = 2 * ((a - 1) - (a + 1) * cos(w));
        a2 = (a + 1) - (a - 1) * cos(w) - b * sin(w);
    }

    void BassTrebleEffect::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);   
    }

    Json BassTrebleEffect::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool BassTrebleEffect::AbstractDetailsAvailable() {
        return false;
    }

    std::string BassTrebleEffect::AbstractHeader() {
        return "Bass / Treble Effect";
    }

    std::string BassTrebleEffect::Icon() {
        return ICON_FA_VOLUME_HIGH;
    }

    std::optional<std::string> BassTrebleEffect::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::BassTrebleEffect>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Bass / Treble Effect",
            .packageName = RASTER_PACKAGED "bass_treble_effect",
            .category = Raster::DefaultNodeCategories::s_audio
        };
    }
}