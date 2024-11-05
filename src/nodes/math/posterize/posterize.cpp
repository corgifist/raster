#include "posterize.h"
#include "../../../ImGui/imgui.h"
#include "common/dynamic_math.h"

namespace Raster {

    Posterize::Posterize() {
        NodeBase::Initialize();

        SetupAttribute("A", 1.0f);
        SetupAttribute("Levels", 1.0f);
        SetupAttribute("Intensity", 1.0f);

        AddOutputPin("Value");
    }

    AbstractPinMap Posterize::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};
        auto aCandidate = GetDynamicAttribute("A", t_contextData);
        auto levelsCandidate = GetDynamicAttribute("Levels", t_contextData);
        auto intensityCandidate = GetDynamicAttribute("Intensity", t_contextData);
        if (aCandidate.has_value() && levelsCandidate.has_value()) {
            auto& a = aCandidate.value();
            auto& b = levelsCandidate.value();
            auto& intensity = intensityCandidate.value();
            auto intensityCalculationCandidate = DynamicMath::Divide(a, intensity);
            if (intensityCalculationCandidate.has_value()) {
                auto& intensityCalculation = intensityCalculationCandidate.value();
                auto posterizationCandidate = DynamicMath::Posterize(intensityCalculation, b);
                if (posterizationCandidate.has_value()) {
                    auto resultCandidate = DynamicMath::Multiply(posterizationCandidate.value(), intensity);
                    if (resultCandidate.has_value()) {
                        TryAppendAbstractPinMap(result, "Value", resultCandidate.value());
                    }
                }
            }
        }

        return result;
    }

    void Posterize::AbstractRenderProperties() {
        RenderAttributeProperty("A");
        RenderAttributeProperty("Levels");
        RenderAttributeProperty("Intensity");
    }

    void Posterize::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json Posterize::AbstractSerialize() {
        return SerializeAllAttributes();
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