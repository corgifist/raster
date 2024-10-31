#include "export_to_audio_bus.h"
#include "common/audio_samples.h"
#include "audio/audio.h"
#include "common/generic_audio_decoder.h"
#include "common/audio_info.h"

namespace Raster {

    ExportToAudioBus::ExportToAudioBus() {
        NodeBase::Initialize();
        NodeBase::GenerateFlowPins();

        SetupAttribute("BusID", -1);
        SetupAttribute("Samples", GenericAudioDecoder());

        this->m_lastUsedAudioBusID = -1;

        AddInputPin("Samples");
    }

    AbstractPinMap ExportToAudioBus::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};
        auto& project = Workspace::GetProject();
        auto contextData = GetContextData();

        auto busIDCandidate = GetAttribute<int>("BusID");
        auto samplesCandidate = GetAttribute<AudioSamples>("Samples");
        
        if (contextData.find("AUDIO_PASS") == contextData.end()) return {};
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

            if (busCandidate.has_value()) {
                auto& bus = busCandidate.value();
                if (bus->samples.size() != 4096 * AudioInfo::s_channels) {
                    bus->samples.resize(4096 * AudioInfo::s_channels);
                }
                auto rawSamples = samples.samples->data();
                memcpy(bus->samples.data(), rawSamples, sizeof(float) * AudioInfo::s_periodSize * AudioInfo::s_channels);
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