#include "abs.h"
#include "../../../ImGui/imgui.h"
#include "dynamic_math/dynamic_math.h"

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
                    if (sine.has_value()) std::cout << sine.type().name() << std::endl;
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
        SetAttributeValue("Input", t_data["Input"].get<float>());    
    }

    Json Abs::AbstractSerialize() {
        return {
            {"Input", std::any_cast<float>(m_attributes["Input"])}
        };
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
    Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::Abs>();
    }

    Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Abs",
            .packageName = RASTER_PACKAGED "abs",
            .category = Raster::DefaultNodeCategories::s_math
        };
    }
}