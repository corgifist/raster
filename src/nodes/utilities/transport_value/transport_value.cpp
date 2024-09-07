#include "transport_value.h"

namespace Raster {

    TransportValue::TransportValue() {
        NodeBase::Initialize();

        SetupAttribute("Input", std::nullopt);

        AddInputPin("Input");
        AddOutputPin("Output");
    }

    AbstractPinMap TransportValue::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};
        auto inputCandidate = GetDynamicAttribute("Input");
        if (inputCandidate.has_value() && inputCandidate.value().type() != typeid(std::nullopt)) {
            TryAppendAbstractPinMap(result, "Output", inputCandidate.value());
        }
        return result;
    }

    void TransportValue::AbstractRenderProperties() {
        RenderAttributeProperty("Input");
    }

    void TransportValue::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json TransportValue::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool TransportValue::AbstractDetailsAvailable() {
        return false;
    }

    std::string TransportValue::AbstractHeader() {
        return "Transport Value";
    }

    std::string TransportValue::Icon() {
        return ICON_FA_BOX;
    }

    std::optional<std::string> TransportValue::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::TransportValue>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Transport Value",
            .packageName = RASTER_PACKAGED "transport_value",
            .category = Raster::DefaultNodeCategories::s_utilities
        };
    }
}