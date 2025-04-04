#include "dummy_audio_mixer.h"

namespace Raster {

    DummyAudioMixer::DummyAudioMixer() {
        NodeBase::Initialize();
        NodeBase::GenerateFlowPins();

        SetupAttribute("Samples", std::nullopt);
    }

    AbstractPinMap DummyAudioMixer::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};
        GetDynamicAttribute("Samples", t_contextData);
        return result;
    }

    void DummyAudioMixer::AbstractRenderProperties() {
    }

    void DummyAudioMixer::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);   
    }

    Json DummyAudioMixer::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool DummyAudioMixer::AbstractDetailsAvailable() {
        return false;
    }

    bool DummyAudioMixer::AbstractDoesAudioMixing() {
        return true;
    }

    std::string DummyAudioMixer::AbstractHeader() {
        return "Dummy Audio Mixer";
    }

    std::string DummyAudioMixer::Icon() {
        return ICON_FA_VOLUME_HIGH;
    }

    std::optional<std::string> DummyAudioMixer::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::DummyAudioMixer>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Dummy Audio Mixer",
            .packageName = RASTER_PACKAGED "dummy_audio_mixer",
            .category = Raster::DefaultNodeCategories::s_other
        };
    }
}