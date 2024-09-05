#include "posterize.h"
#include "../../../ImGui/imgui.h"
#include "dynamic_math/dynamic_math.h"

namespace Raster {

    Posterize::Posterize() {
        NodeBase::Initialize();

        SetupAttribute("A", 1.0f);
        SetupAttribute("Levels", 1.0f);

        AddOutputPin("Value");
    }

    AbstractPinMap Posterize::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};
        auto aCandidate = GetDynamicAttribute("A");
        auto levelsCandidate = GetDynamicAttribute("Levels");
        if (aCandidate.has_value() && levelsCandidate.has_value()) {
            auto& a = aCandidate.value();
            auto& b = levelsCandidate.value();
            auto posterizationCandidate = DynamicMath::Posterize(a, b);
            if (posterizationCandidate.has_value()) {
                TryAppendAbstractPinMap(result, "Value", posterizationCandidate.value());
            }
        }

        return result;
    }

    void Posterize::AbstractRenderProperties() {
        RenderAttributeProperty("A");
        RenderAttributeProperty("Levels");
    }

    void Posterize::AbstractLoadSerialized(Json t_data) {
        RASTER_DESERIALIZE_WRAPPER(float, "A");
        RASTER_DESERIALIZE_WRAPPER(float, "Levels");
    }

    Json Posterize::AbstractSerialize() {
        return {
            RASTER_SERIALIZE_WRAPPER(float, "A"),
            RASTER_SERIALIZE_WRAPPER(float, "Levels")  
        };
    }

    bool Posterize::AbstractDetailsAvailable() {
        return false;
    }

    std::string Posterize::AbstractHeader() {
        return "Posterize";
    }

    std::string Posterize::Icon() {
        return ICON_FA_CHART_LINE;
    }

    std::optional<std::string> Posterize::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::Posterize>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Posterize",
            .packageName = RASTER_PACKAGED "posterize",
            .category = Raster::DefaultNodeCategories::s_math
        };
    }
}