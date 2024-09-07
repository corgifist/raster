#include "add.h"
#include "../../../ImGui/imgui.h"
#include "dynamic_math/dynamic_math.h"

namespace Raster {

    Add::Add() {
        NodeBase::Initialize();

        SetupAttribute("A", 1.0f);
        SetupAttribute("B", 1.0f);

        AddOutputPin("Value");
    }

    AbstractPinMap Add::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};
        auto aCandidate = GetDynamicAttribute("A");
        auto bCandidate = GetDynamicAttribute("B");
        if (aCandidate.has_value() && bCandidate.has_value()) {
            auto& a = aCandidate.value();
            auto& b = bCandidate.value();
            auto additionCandidate = DynamicMath::Add(a, b);
            if (additionCandidate.has_value()) {
                TryAppendAbstractPinMap(result, "Value", additionCandidate.value());
            }
        }

        return result;
    }

    void Add::AbstractRenderProperties() {
        RenderAttributeProperty("A");
        RenderAttributeProperty("B");
    }

    void Add::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json Add::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool Add::AbstractDetailsAvailable() {
        return false;
    }

    std::string Add::AbstractHeader() {
        return "Add";
    }

    std::string Add::Icon() {
        return ICON_FA_PLUS;
    }

    std::optional<std::string> Add::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::Add>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Add",
            .packageName = RASTER_PACKAGED "add",
            .category = Raster::DefaultNodeCategories::s_math
        };
    }
}