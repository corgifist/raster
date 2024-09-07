#include "mix.h"
#include "../../../ImGui/imgui.h"
#include "dynamic_math/dynamic_math.h"

namespace Raster {

    Mix::Mix() {
        NodeBase::Initialize();

        SetupAttribute("A", 0.0f);
        SetupAttribute("B", 1.0f);
        SetupAttribute("Phase", 0.5f);

        AddOutputPin("Value");
    }

    AbstractPinMap Mix::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};
        auto aCandidate = GetDynamicAttribute("A");
        auto bCandidate = GetDynamicAttribute("B");
        auto phaseCandidate = GetDynamicAttribute("Phase");
        if (aCandidate.has_value() && bCandidate.has_value() && phaseCandidate.has_value()) {
            auto& a = aCandidate.value();
            auto& b = bCandidate.value();
            auto& phase = phaseCandidate.value();
            auto multiplicationCandidate = DynamicMath::Mix(a, b, phase);
            if (multiplicationCandidate.has_value()) {
                TryAppendAbstractPinMap(result, "Value", multiplicationCandidate.value());
            }
        }

        return result;
    }

    void Mix::AbstractRenderProperties() {
        RenderAttributeProperty("A");
        RenderAttributeProperty("B");
        RenderAttributeProperty("Phase");
    }

    void Mix::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json Mix::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool Mix::AbstractDetailsAvailable() {
        return false;
    }

    std::string Mix::AbstractHeader() {
        return "Mix";
    }

    std::string Mix::Icon() {
        return ICON_FA_DROPLET;
    }

    std::optional<std::string> Mix::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::Mix>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Mix",
            .packageName = RASTER_PACKAGED "mix",
            .category = Raster::DefaultNodeCategories::s_math
        };
    }
}