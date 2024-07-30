#include "sine.h"
#include "../../../ImGui/imgui.h"
#include "dynamic_math/dynamic_math.h"

namespace Raster {

    Sine::Sine() {
        NodeBase::Initialize();

        this->m_attributes["Input"] = 0.0f;
        this->m_attributes["MultiplyBy"] = 1.0f;

        AddOutputPin("Value");
    }

    AbstractPinMap Sine::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};
        auto sineCandidate = ComputeSine();
        if (sineCandidate.has_value()) {
            auto sine = sineCandidate.value();
            TryAppendAbstractPinMap(result, "Value", sine);
        }

        return result;
    }

    void Sine::AbstractRenderProperties() {
        RenderAttributeProperty("Input");
        RenderAttributeProperty("MultiplyBy");
    }

    bool Sine::AbstractDetailsAvailable() {
        return true;
    }

    void Sine::AbstractRenderDetails() {
        ImGui::PushID(nodeID);
            auto sineCandidate = ComputeSine();
            if (sineCandidate.has_value()) {
                auto& sine = sineCandidate.value();
                if (sine.has_value() && sine.type() == typeid(float)) {
                    float f = std::any_cast<float>(sine);
                    ImGui::PlotVar(FormatString("%s %s", ICON_FA_WAVE_SQUARE, "Sine").c_str(), f);
                } else {
                    if (sine.has_value()) std::cout << sine.type().name() << std::endl;
                    ImGui::Text("%s", Localization::GetString("NO_SINE_PREVIEW").c_str());
                }
            }
        ImGui::PopID();
    }

    std::optional<std::any> Sine::ComputeSine() {
        auto inputCandidate = GetDynamicAttribute("Input");
        if (inputCandidate.has_value()) {
            auto& input = inputCandidate.value();
            auto multiplyByCandidate = GetDynamicAttribute("MultiplyBy");
            auto dynamicSineCandidate = DynamicMath::Sine(input);
            if (multiplyByCandidate.has_value() && dynamicSineCandidate.has_value()) {
                auto sine = DynamicMath::Multiply(dynamicSineCandidate.value(), multiplyByCandidate.value());
                if (sine.has_value()) {
                    return sine.value();
                }
            }
        }
        return std::nullopt;
    }

    std::string Sine::AbstractHeader() {
        return "Sine";
    }

    std::string Sine::Icon() {
        return ICON_FA_WAVE_SQUARE;
    }

    std::optional<std::string> Sine::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::Sine>();
    }

    Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Sine",
            .packageName = RASTER_PACKAGED "sine",
            .category = Raster::DefaultNodeCategories::s_math
        };
    }
}