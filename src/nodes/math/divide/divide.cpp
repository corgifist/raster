#include "divide.h"
#include "../../../ImGui/imgui.h"
#include "dynamic_math/dynamic_math.h"

namespace Raster {

    Divide::Divide() {
        NodeBase::Initialize();

        SetupAttribute("A", 1.0f);
        SetupAttribute("B", 1.0f);

        AddOutputPin("Value");
    }

    AbstractPinMap Divide::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};
        auto aCandidate = GetDynamicAttribute("A");
        auto bCandidate = GetDynamicAttribute("B");
        if (aCandidate.has_value() && bCandidate.has_value()) {
            auto& a = aCandidate.value();
            auto& b = bCandidate.value();
            auto divisionCandidate = DynamicMath::Divide(a, b);
            if (divisionCandidate.has_value()) {
                TryAppendAbstractPinMap(result, "Value", divisionCandidate.value());
            }
        }

        return result;
    }

    void Divide::AbstractRenderProperties() {
        RenderAttributeProperty("A");
        RenderAttributeProperty("B");
    }

    void Divide::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json Divide::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool Divide::AbstractDetailsAvailable() {
        return false;
    }

    std::string Divide::AbstractHeader() {
        return "Divide";
    }

    std::string Divide::Icon() {
        return ICON_FA_DIVIDE;
    }

    std::optional<std::string> Divide::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::Divide>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Divide",
            .packageName = RASTER_PACKAGED "divide",
            .category = Raster::DefaultNodeCategories::s_math
        };
    }
}