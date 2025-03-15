#include "common/bezier_curve.h"
#include "common/localization.h"
#include "common/ui_helpers.h"
#include "font/IconsFontAwesome5.h"
#include "bezier_attribute.h"
#include "common/dispatchers.h"
#include "raster.h"
#include <glm/gtc/type_ptr.hpp>

namespace Raster {
    BezierCurveAttribute::BezierCurveAttribute() {
        AttributeBase::Initialize();

        this->interpretAsColor = true;

        keyframes.push_back(
            AttributeKeyframe(
                0,
                BezierCurve()
            )
        );
    }


    std::any BezierCurveAttribute::AbstractInterpolate(std::any t_beginValue, std::any t_endValue, float t_percentage, float t_frame, Composition* composition) {
        auto a = std::any_cast<BezierCurve>(t_beginValue);
        auto b = std::any_cast<BezierCurve>(t_endValue);
        float t = t_percentage;

        BezierCurve newBezier = a;
        if (a.points.size() == b.points.size()) {
            for (int i = 0; i < a.points.size(); i++) {
                newBezier.points[i] = glm::mix(a.points[i], b.points[i], t_percentage);
            }
        }

        return newBezier;
    }

    void BezierCurveAttribute::RenderKeyframes() {
        for (auto& keyframe : keyframes) {
            RenderKeyframe(keyframe);
        }
    }


    std::any BezierCurveAttribute::AbstractRenderLegend(Composition* t_composition, std::any t_originalValue, bool& isItemEdited) {
        auto bezier = std::any_cast<BezierCurve>(t_originalValue);
        auto originalLine = bezier;

        auto buttonText = FormatString("%s %i %s", ICON_FA_BEZIER_CURVE, (int) bezier.points.size(), Localization::GetString("POINTS").c_str());
        if (ImGui::Button(buttonText.c_str(), ImVec2(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x, 0))) {
            ImGui::OpenPopup("##editBezier");
        }
        if (ImGui::BeginPopup("##editBezier")) {
            ImGui::SeparatorText(FormatString("%s %s", ICON_FA_BEZIER_CURVE, name.c_str()).c_str());
            int targetDeletePoint = -1;
            for (int i = 0; i < bezier.points.size(); i++) {
                ImGui::PushID(i);
                if (ImGui::Button(ICON_FA_TRASH_CAN)) {
                    targetDeletePoint = i;
                }
                ImGui::SameLine();
                ImGui::AlignTextToFramePadding();
                ImGui::Text(FormatString("%s P%i", ICON_FA_BEZIER_CURVE, i).c_str());
                ImGui::SameLine();
                ImGui::DragFloat2("##pDrag", glm::value_ptr(bezier.points[i]), 0.01f, 0.0f, 0.0f, "%0.2f");
                isItemEdited = isItemEdited || ImGui::IsItemEdited();
                ImGui::PopID();
            }

            if (UIHelpers::CenteredButton(FormatString("%s %s", ICON_FA_PLUS, Localization::GetString("NEW_POINT").c_str()).c_str())) {
                bezier.points.push_back(bezier.Get(0.5f));
                isItemEdited = true;
            }

            if (targetDeletePoint >= 0) {
                bezier.points.erase(bezier.points.begin() + targetDeletePoint);
                isItemEdited = true;
            }

            ImGui::EndPopup();
        }

        return bezier;
    }

    void BezierCurveAttribute::AbstractRenderDetails() {
        auto& project = Workspace::s_project.value();
        auto parentComposition = Workspace::GetCompositionByAttributeID(id).value();
        ImGui::PushID(id);
            auto currentValue = Get(project.currentFrame - parentComposition->beginFrame, parentComposition);
            Dispatchers::DispatchString(currentValue);
        ImGui::PopID();
    }
    Json BezierCurveAttribute::SerializeKeyframeValue(std::any t_value) {
        return std::any_cast<BezierCurve>(t_value).Serialize();
    }  

    std::any BezierCurveAttribute::LoadKeyframeValue(Json t_value) {
        return BezierCurve(t_value);
    }

}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractAttribute SpawnAttribute() {
        return (Raster::AbstractAttribute) std::make_shared<Raster::BezierCurveAttribute>();
    }

    RASTER_DL_EXPORT Raster::AttributeDescription GetDescription() {
        return Raster::AttributeDescription{
            .packageName = RASTER_PACKAGED "bezier_attribute",
            .prettyName = ICON_FA_BEZIER_CURVE " Bezier Curve"
        };
    }
}