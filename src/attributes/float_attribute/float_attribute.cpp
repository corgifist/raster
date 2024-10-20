#include "float_attribute.h"

namespace Raster {
    FloatAttribute::FloatAttribute() {
        AttributeBase::Initialize();

        keyframes.push_back(
            AttributeKeyframe(
                0,
                0.0f
            )
        );
    }

    std::any FloatAttribute::AbstractInterpolate(std::any t_beginValue, std::any t_endValue, float t_percentage, float t_frame, Composition* composition) {
        float a = std::any_cast<float>(t_beginValue);
        float b = std::any_cast<float>(t_endValue);
        float t = t_percentage;

        return a + t * (b - a);
    }

    void FloatAttribute::RenderKeyframes() {
        for (auto& keyframe : keyframes) {
            RenderKeyframe(keyframe);
        }
    }

    Json FloatAttribute::SerializeKeyframeValue(std::any t_value) {
        return std::any_cast<float>(t_value);
    }  

    std::any FloatAttribute::LoadKeyframeValue(Json t_value) {
        return t_value.get<float>();
    }

    std::any FloatAttribute::AbstractRenderLegend(Composition* t_composition, std::any t_originalValue, bool& isItemEdited) {
        float fValue = std::any_cast<float>(t_originalValue);
        bool isOpacityAttribute = t_composition->opacityAttributeID == id;
        if (!isOpacityAttribute) {
            ImGui::DragFloat("##dragFloat", &fValue, 0.01);
        } else {
            ImGui::SliderFloat("##sliderFloat", &fValue, 0, 1);
        }
        isItemEdited = ImGui::IsItemEdited();
        return fValue;
    }

    void FloatAttribute::AbstractRenderDetails() {
        auto& project = Workspace::s_project.value();
        auto parentComposition = Workspace::GetCompositionByAttributeID(id).value();
        ImGui::PushID(id);
            ImGui::PlotVar(name.c_str(), std::any_cast<float>(Get(project.currentFrame - parentComposition->beginFrame, parentComposition)));
        ImGui::PopID();
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractAttribute SpawnAttribute() {
        return (Raster::AbstractAttribute) std::make_shared<Raster::FloatAttribute>();
    }

    RASTER_DL_EXPORT Raster::AttributeDescription GetDescription() {
        return Raster::AttributeDescription{
            .packageName = RASTER_PACKAGED "float_attribute",
            .prettyName = ICON_FA_DIVIDE " Float"
        };
    }
}