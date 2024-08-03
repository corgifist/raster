#include "transport_value.h"

namespace Raster {

    TransportValue::TransportValue() {
        NodeBase::Initialize();

        this->m_attributes["Input"] = std::nullopt;

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
    Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::TransportValue>();
    }

    Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Transport Value",
            .packageName = RASTER_PACKAGED "transport_value",
            .category = Raster::DefaultNodeCategories::s_utilities
        };
    }
}