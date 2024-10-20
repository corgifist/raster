#include "multiply.h"
#include "../../../ImGui/imgui.h"
#include "dynamic_math/dynamic_math.h"

namespace Raster {

    Multiply::Multiply() {
        NodeBase::Initialize();

        SetupAttribute("A", 1.0f);
        SetupAttribute("B", 1.0f);

        AddOutputPin("Value");
    }

    AbstractPinMap Multiply::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};
        auto aCandidate = GetDynamicAttribute("A");
        auto bCandidate = GetDynamicAttribute("B");
        if (aCandidate.has_value() && bCandidate.has_value()) {
            auto& a = aCandidate.value();
            auto& b = bCandidate.value();
            auto multiplicationCandidate = DynamicMath::Multiply(a, b);
            if (multiplicationCandidate.has_value()) {
                TryAppendAbstractPinMap(result, "Value", multiplicationCandidate.value());
            }
        }

        return result;
    }

    void Multiply::AbstractRenderProperties() {
        RenderAttributeProperty("A");
        RenderAttributeProperty("B");
    }

    void Multiply::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json Multiply::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool Multiply::AbstractDetailsAvailable() {
        return false;
    }

    std::string Multiply::AbstractHeader() {
        return "Multiply";
    }

    std::string Multiply::Icon() {
        return ICON_FA_XMARK;
    }

    std::optional<std::string> Multiply::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::Multiply>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Multiply",
            .packageName = RASTER_PACKAGED "multiply",
            .category = Raster::DefaultNodeCategories::s_math
        };
    }
}