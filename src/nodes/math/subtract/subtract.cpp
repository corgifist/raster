#include "subtract.h"
#include "../../../ImGui/imgui.h"
#include "common/dynamic_math.h"

namespace Raster {

    Subtract::Subtract() {
        NodeBase::Initialize();

        SetupAttribute("A", 1.0f);
        SetupAttribute("B", 1.0f);

        AddOutputPin("Value");
    }

    AbstractPinMap Subtract::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};
        auto aCandidate = GetDynamicAttribute("A");
        auto bCandidate = GetDynamicAttribute("B");
        if (aCandidate.has_value() && bCandidate.has_value()) {
            auto& a = aCandidate.value();
            auto& b = bCandidate.value();
            auto multiplicationCandidate = DynamicMath::Subtract(a, b);
            if (multiplicationCandidate.has_value()) {
                TryAppendAbstractPinMap(result, "Value", multiplicationCandidate.value());
            }
        }

        return result;
    }

    void Subtract::AbstractRenderProperties() {
        RenderAttributeProperty("A");
        RenderAttributeProperty("B");
    }

    void Subtract::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json Subtract::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool Subtract::AbstractDetailsAvailable() {
        return false;
    }

    std::string Subtract::AbstractHeader() {
        return "Subtract";
    }

    std::string Subtract::Icon() {
        return ICON_FA_MINUS;
    }

    std::optional<std::string> Subtract::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::Subtract>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Subtract",
            .packageName = RASTER_PACKAGED "subtract",
            .category = Raster::DefaultNodeCategories::s_math
        };
    }
}