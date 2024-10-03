#include "export_to_audio_bus.h"
#include "common/audio_samples.h"
#include "audio/audio.h"

namespace Raster {

    ExportToAudioBus::ExportToAudioBus() {
        NodeBase::Initialize();
        NodeBase::GenerateFlowPins();

        SetupAttribute("BusID", -1);
        SetupAttribute("Samples", AudioSamples());

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
            for (auto& bus : project.audioBuses) {
                if (bus.main) {
                    busID = bus.id;
                    break;
                }
            }
            auto& samples = samplesCandidate.value();
            auto busCandidate = Workspace::GetAudioBusByID(busID);
            if (busCandidate.has_value()) {
                auto& bus = busCandidate.value();
                if (bus->samples.size() != 4096 * Audio::GetChannelCount()) {
                    bus->samples.resize(4096 * Audio::GetChannelCount());
                }
                auto rawSamples = samples.samples->data();
                memcpy(bus->samples.data(), rawSamples, sizeof(float) * Audio::s_samplesCount * Audio::GetChannelCount());
            }

        }

        return result;
    }

    bool ExportToAudioBus::AbstractDoesAudioMixing() {
        return true;
    }

    void ExportToAudioBus::AbstractRenderProperties() {
        
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