#include "gradient1d_attribute.h"
#include "common/workspace.h"
#include "common/ui_helpers.h"

namespace Raster {
    Gradient1DAttribute::Gradient1DAttribute() {
        AttributeBase::Initialize();

        keyframes.push_back(
            AttributeKeyframe(
                0,
                Gradient1D()
            )
        );
    }

    std::any Gradient1DAttribute::AbstractInterpolate(std::any t_beginValue, std::any t_endValue, float t_percentage, float t_frame, Composition* composition) {
        auto a = std::any_cast<Gradient1D>(t_beginValue);
        auto b = std::any_cast<Gradient1D>(t_endValue);
        float& t = t_percentage;

        if (a.stops.size() > b.stops.size()) {
            b = b.MatchStopsCount(a);
        } else if (a.stops.size() < b.stops.size()) {
            a = a.MatchStopsCount(b);
        }
        for (int i = 0; i < b.stops.size(); i++) {
            b.stops[i].color = glm::mix(a.stops[i].color, b.stops[i].color, t);
        }
        return b;
    }

    Json Gradient1DAttribute::SerializeKeyframeValue(std::any t_value) {
        return std::any_cast<Gradient1D>(t_value).Serialize();
    }  

    std::any Gradient1DAttribute::LoadKeyframeValue(Json t_value) {
        return Gradient1D(t_value);
    }

    void Gradient1DAttribute::RenderKeyframes() {
        for (auto& keyframe : keyframes) {
            RenderKeyframe(keyframe);
        }
    }

    std::any Gradient1DAttribute::AbstractRenderLegend(Composition* t_composition, std::any t_originalValue, bool& isItemEdited) {
        auto& project = Workspace::GetProject();
        auto gradient = std::any_cast<Gradient1D>(t_originalValue);

        UIHelpers::RenderGradient1D(gradient, ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x, ImGui::CalcTextSize(" ").y  + ImGui::GetStyle().FramePadding.y * 2, m_gradientHovered ? 0.7f : 1.0f);
        m_gradientHovered = ImGui::IsItemHovered();
        if (ImGui::IsItemClicked()) {
            ImGui::OpenPopup("##gradientEditor");
            m_persistentGradient = gradient;
        }
        bool editingGradient = false;
        if (ImGui::BeginPopup("##gradientEditor")) {
            ImGui::SeparatorText(FormatString("%s %s", ICON_FA_DROPLET, name.c_str()).c_str());
            isItemEdited = UIHelpers::RenderGradient1DEditor(m_persistentGradient, 350);
            ImGui::EndPopup();
            editingGradient = true;
        }

        return editingGradient ? m_persistentGradient : gradient;
    }

    void Gradient1DAttribute::AbstractRenderDetails() {
        auto& project = Workspace::s_project.value();
        auto parentComposition = Workspace::GetCompositionByAttributeID(id).value();
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractAttribute SpawnAttribute() {
        return (Raster::AbstractAttribute) std::make_shared<Raster::Gradient1DAttribute>();
    }

    RASTER_DL_EXPORT Raster::AttributeDescription GetDescription() {
        return Raster::AttributeDescription{
            .packageName = RASTER_PACKAGED "gradient1d_attribute",
            .prettyName = ICON_FA_DROPLET " Gradient1D"
        };
    }
}