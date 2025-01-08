#include "export_to_audio_bus.h"
#include "common/audio_samples.h"
#include "audio/audio.h"
#include "common/generic_audio_decoder.h"
#include "common/audio_info.h"
#include "common/waveform_manager.h"
#include "raster.h"

namespace Raster {

    ExportToAudioBus::ExportToAudioBus() {
        NodeBase::Initialize();
        NodeBase::GenerateFlowPins();

        SetupAttribute("BusID", -1);
        SetupAttribute("Samples", GenericAudioDecoder());

        this->m_lastUsedAudioBusID = -1;

        AddInputPin("Samples");
    }

    AbstractPinMap ExportToAudioBus::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};
        auto samplesCandidate = GetAttribute<AudioSamples>("Samples", t_contextData);
        if (RASTER_GET_CONTEXT_VALUE(t_contextData, "WAVEFORM_PASS", bool) && samplesCandidate && samplesCandidate->samples) {
            auto& samples = *samplesCandidate;
            WaveformManager::PushWaveformSamples(samples.samples);
            // RASTER_LOG("pushing waveform samples");
            return {};
        }
        if (!Audio::IsAudioInstanceActive()) return result;
        auto& project = Workspace::GetProject();

        auto busIDCandidate = GetAttribute<int>("BusID", t_contextData);
        
        if (t_contextData.find("AUDIO_PASS") == t_contextData.end()) return {};
        if (busIDCandidate.has_value() && samplesCandidate.has_value() && samplesCandidate.value().samples && project.playing) {
            auto busID = busIDCandidate.value();
            auto busCandidate = Workspace::GetAudioBusByID(busID);
            if (!busCandidate.has_value()) {
                for (auto& bus : project.audioBuses) {
                    if (bus.main) {
                        busID = bus.id;
                        break;
                    }
                }
                busCandidate = Workspace::GetAudioBusByID(busID);
            }
            m_lastUsedAudioBusID = busID;
            auto& samples = samplesCandidate.value();

            if (!RASTER_GET_CONTEXT_VALUE(t_contextData, "WAVEFORM_PASS", bool)) {
                if (busCandidate.has_value()) {
                    auto& bus = busCandidate.value();
                    bus->ValidateBuffers();
                    auto rawSamples = samples.samples->data();
                    for (int i = 0; i < AudioInfo::s_periodSize * AudioInfo::s_channels; i++) {
                        bus->samples[i] += rawSamples[i];
                    }
                }
            } else if (!samples.samples->empty()) {
                WaveformManager::PushWaveformSamples(samples.samples);
            }

        }

        return result;
    }

    std::vector<int> ExportToAudioBus::AbstractGetUsedAudioBuses() {
        if (m_lastUsedAudioBusID > 0) {
            return {m_lastUsedAudioBusID};
        }
        return {};
    }

    bool ExportToAudioBus::AbstractDoesAudioMixing() {
        return true;
    }

    void ExportToAudioBus::AbstractRenderProperties() {
        RenderAttributeProperty("Samples", {
            IconMetadata(ICON_FA_WAVE_SQUARE)
        });
    }

    void ExportToAudioBus::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);   
    }

    Json ExportToAudioBus::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool ExportToAudioBus::AbstractDetailsAvailable() {
        return false;
    }

    std::string ExportToAudioBus::AbstractHeader() {
        return "Export to Audio Bus";
    }

    std::string ExportToAudioBus::Icon() {
        return ICON_FA_VOLUME_HIGH;
    }

    std::optional<std::string> ExportToAudioBus::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::ExportToAudioBus>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Export to Audio Bus",
            .packageName = RASTER_PACKAGED "export_to_audio_bus",
            .category = Raster::DefaultNodeCategories::s_audio
        };
    }
}