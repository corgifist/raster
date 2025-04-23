#include "dummy.h"

namespace Raster {

    Dummy::Dummy() {
        NodeBase::Initialize();

        SetupAttribute("Input", std::nullopt);
        AddInputPin("Input");
        AddOutputPin("     ");

        SetAttributeAlias("Input", "     ");
    }

    AbstractPinMap Dummy::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};
        auto inputCandidate = GetDynamicAttribute("Input", t_contextData);
        if (inputCandidate) {
            TryAppendAbstractPinMap(result, "     ", *inputCandidate);
        }
        return result;
    }

    void Dummy::AbstractRenderProperties() {
    }

    void Dummy::AbstractLoadSerialized(Json t_data) {
    }

    Json Dummy::AbstractSerialize() {
        return {};
    }

    bool Dummy::AbstractDetailsAvailable() {
        return false;
    }

    std::string Dummy::AbstractHeader() {
        return "";
    }

    std::string Dummy::Icon() {
        return "";
    }

    std::optional<std::string> Dummy::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::Dummy>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Dummy",
            .packageName = RASTER_PACKAGED "dummy",
            .category = Raster::DefaultNodeCategories::s_utilities
        };
    }
}