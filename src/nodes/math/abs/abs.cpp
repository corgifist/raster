#include "abs.h"
#include "../../../ImGui/imgui.h"
#include "common/dynamic_math.h"

namespace Raster {

    Abs::Abs() {
        NodeBase::Initialize();

        SetupAttribute("Input", 0.0f);

        AddOutputPin("Value");
    }

    AbstractPinMap Abs::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};
        auto absCandidate = ComputeAbs();
        if (absCandidate.has_value()) {
            auto sine = absCandidate.value();
            TryAppendAbstractPinMap(result, "Value", sine);
        }

        return result;
    }

    void Abs::AbstractRenderProperties() {
        RenderAttributeProperty("Input");
    }

    bool Abs::AbstractDetailsAvailable() {
        return true;
    }

    void Abs::AbstractRenderDetails() {
        ImGui::PushID(nodeID);
            auto absCandidate = ComputeAbs();
            if (absCandidate.has_value()) {
                auto& sine = absCandidate.value();
                if (sine.has_value() && sine.type() == typeid(float)) {
                    float f = std::any_cast<float>(sine);
                    ImGui::PlotVar(FormatString("%s %s", ICON_FA_PLUS, "Abs").c_str(), f);
                } else {
                    ImGui::Text("%s", Localization::GetString("NO_ABS_PREVIEW").c_str());
                }
            }
        ImGui::PopID();
    }

    std::optional<std::any> Abs::ComputeAbs() {
        auto inputCandidate = GetDynamicAttribute("Input");
        if (inputCandidate.has_value()) {
            auto& input = inputCandidate.value();
            auto absCandidate = DynamicMath::Abs(input);
            if (absCandidate.has_value()) {
                return absCandidate.value();
            }
        }
        return std::nullopt;
    }

    void Abs::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data); 
    }

    Json Abs::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    std::string Abs::AbstractHeader() {
        return "Abs";
    }

    std::string Abs::Icon() {
        return ICON_FA_PLUS;
    }

    std::optional<std::string> Abs::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::Abs>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Abs",
            .packageName = RASTER_PACKAGED "abs",
            .category = Raster::DefaultNodeCategories::s_math
        };
    }
}