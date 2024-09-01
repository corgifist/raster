#include "bezier_easing.h"

#include "font/font.h"

#include "common/localization.h"

#include "../../ImGui/imgui.h"
#include "../../ImGui/imgui_bezier.h"
#include "../../ImGui/imgui_stdlib.h"
#include "../../ImGui/imgui_stripes.h"

namespace Raster {

    CurvePreset::CurvePreset(std::string t_name, glm::vec4 t_points) {
        this->name = t_name;
        this->points = t_points;

        this->filteredCategoryName = ReplaceString(t_name, ".*\\(|\\)", "");
        this->category = ReplaceString(t_name, " \\(\\w+\\)", "");
        this->filteredName = ReplaceString(LowerCase(t_name), "\\s|\\(|\\)", "");
    }

    static std::vector<CurvePreset> s_presets = {
        CurvePreset("Linear", glm::vec4(0, 0, 1, 1)),
        CurvePreset("Emphasized (Accelerate)", glm::vec4(0.3, 0.0, 0.8, 0.15)),
        CurvePreset("Emphasized (Decelerate)", glm::vec4(0.05, 0.7, 0.1, 1)),
        CurvePreset("Ease In (Sine)", glm::vec4(0.12, 0, 0.39, 0)),
        CurvePreset("Ease Out (Sine)", glm::vec4(0.61, 1, 0.88, 1)),
        CurvePreset("Ease In Out (Sine)", glm::vec4(0.37, 0, 0.63, 1)),
        CurvePreset("Ease In (Quad)", glm::vec4(0.11, 0, 0.5, 0)),
        CurvePreset("Ease Out (Quad)", glm::vec4(0.5, 1, 0.89, 1)),
        CurvePreset("Ease In Out (Quad)", glm::vec4(0.45, 0, 0.55, 1)),
        CurvePreset("Ease In (Cubic)", glm::vec4(0.32, 0, 0.67, 0)),
        CurvePreset("Ease Out (Cubic)", glm::vec4(0.33, 1, 0.68, 1)),
        CurvePreset("Ease In Out (Cubic)", glm::vec4(0.65, 0, 0.35, 1)),
        CurvePreset("Ease In (Quart)", glm::vec4(0.5, 0, 0.75, 0)),
        CurvePreset("Ease Out (Quart)", glm::vec4(0.25, 1, 0.5, 1)),
        CurvePreset("Ease In Out (Quart)", glm::vec4(0.76, 0, 0.24, 1)),
        CurvePreset("Ease In (Quint)", glm::vec4(0.64, 0, 0.78, 0)),
        CurvePreset("Ease Out (Quint)", glm::vec4(0.22, 1, 0.36, 1)),
        CurvePreset("Ease In Out (Quint)", glm::vec4(0.83, 0, 0.17, 1)),
        CurvePreset("Ease In (Expo)", glm::vec4(0.7, 0, 0.84, 0)),
        CurvePreset("Ease Out (Expo)", glm::vec4(0.16, 1, 0.3, 1)),
        CurvePreset("Ease In Out (Expo)", glm::vec4(0.87, 0, 0.13, 1)),
        CurvePreset("Ease In (Circ)", glm::vec4(0.55, 0, 1, 0.45)),
        CurvePreset("Ease Out (Circ)", glm::vec4(0, 0.55, 0.45, 1)),
        CurvePreset("Ease In Out (Circ)", glm::vec4(0.85, 0, 0.15, 1)),
        CurvePreset("Ease In (Back)", glm::vec4(0.36, 0, 0.66, -0.56)),
        CurvePreset("Ease Out (Back)", glm::vec4(0.34, 1.56, 0.64, 1)),
        CurvePreset("Ease In Out (Back)", glm::vec4(0.68, -0.6, 0.32, 1.6))
    };

    BezierEasing::BezierEasing() {
        EasingBase::Initialize();

        this->m_points = glm::vec4(0, 0, 1, 1);
        this->m_constrained = false;
    }

    float BezierEasing::Get(float t_percentage) {
        float p1x = m_points[0];
        float p1y = m_points[1];
        float p2x = m_points[2];
        float p2y = m_points[3];

        this->m_cx = 3.0 * p1x;
        this->m_bx = 3.0 * (p2x - p1x) - m_cx;
        this->m_ax = 1.0 - m_cx - m_bx;

        this->m_cy = 3.0 * p1y;
        this->m_by = 3.0 * (p2y - p1y) - m_cy;
        this->m_ay = 1.0 - m_cy - m_by;

        return Solve(t_percentage, EPSILON);
    }

    void BezierEasing::AbstractLoad(Json t_data) {
        this->m_points = glm::vec4(
            t_data[0], t_data[1], t_data[2], t_data[3]
        );
        this->m_constrained = t_data[4];
    }

    Json BezierEasing::AbstractSerialize() {
        return {
            m_points[0], m_points[1], m_points[2], m_points[3], m_constrained
        };
    }

    void BezierEasing::AbstractRenderDetails() {
        ImGui::PushID(id);
        ImVec2 bezierEditorSize(230, 230);
        float constraintsPower = 0.3;

        ImVec2 processedBezierSize = bezierEditorSize;
        if (m_constrained) {
            std::optional<float> sizeCandidate = std::nullopt;
            float overshootMultiplier = 0.5f;
            for (int i = 0; i < 4; i++) {
                auto point = m_points[i];
                if (IsInBounds(point, 0.0f, 1.0f)) continue;
                float overshootAmount;
                if (point > 1) overshootAmount = point - 1;
                else if (point < 0) overshootAmount = +point;
                
                if (sizeCandidate.value_or(-100) < overshootAmount * overshootMultiplier) {
                    sizeCandidate = std::abs(overshootAmount * overshootMultiplier);
                }
            }

            if (sizeCandidate.has_value()) {
                processedBezierSize = bezierEditorSize * (1 - std::clamp(std::abs(sizeCandidate.value() * 2), 0.3f, 1.0f));
            }
        }

        ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - bezierEditorSize.x / 2.0f);
        ImGui::BeginChild("##bezierEditor", bezierEditorSize);
            ImGui::Stripes(ImVec4(0.05f, 0.05f, 0.05f, 1), ImVec4(0.1f, 0.1f, 0.1f, 1), 40, 14, bezierEditorSize);
            ImGui::SetCursorPos(ImGui::GetWindowSize() / 2.0f - processedBezierSize / 2.0f);
            ImGui::Bezier("Bezier Curve", glm::value_ptr(m_points), processedBezierSize.x, !m_constrained);
        ImGui::EndChild();    

        static float pointsChildWidth = 150;
        ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - pointsChildWidth / 2.0f);
        ImGui::BeginChild("##dragsChild", ImVec2(bezierEditorSize.x, 0), ImGuiChildFlags_AutoResizeY);
            ImGui::AlignTextToFramePadding();
            ImGui::Text(ICON_FA_BEZIER_CURVE " P1");
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_RECYCLE "##p1")) {
                m_points[0] = 0;
                m_points[1] = 0;
            }
            ImGui::SetItemTooltip("%s %s", ICON_FA_CIRCLE_INFO, Localization::GetString("RESET_POINT_POSITION").c_str());
            ImGui::SameLine();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::DragFloat2("##p1Drag", glm::value_ptr(m_points), 0.05f);
            ImGui::PopItemWidth();

            ImGui::AlignTextToFramePadding();
            ImGui::Text(ICON_FA_BEZIER_CURVE " P2");
            ImGui::SameLine();
            if (ImGui::Button(ICON_FA_RECYCLE "##p2")) {
                m_points[2] = 1;
                m_points[3] = 1;
            }
            ImGui::SetItemTooltip("%s %s", ICON_FA_CIRCLE_INFO, Localization::GetString("RESET_POINT_POSITION").c_str());
            ImGui::SameLine();
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::DragFloat2("##p2Drag", glm::value_ptr(m_points) + 2, 0.05f);
            ImGui::PopItemWidth();
            if (ImGui::Button(FormatString("%s %s", ICON_FA_LIST, Localization::GetString("CURVE_PRESETS").c_str()).c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                ImGui::OpenPopup("##curvePresets");
            }

            if (ImGui::BeginPopup("##curvePresets")) {
                ImGui::SeparatorText(FormatString("%s %s", ICON_FA_LIST, Localization::GetString("CURVE_PRESETS").c_str()).c_str());
                std::optional<CurvePreset> hoveredPreset;
                std::vector<std::string> categories;
                static std::string previousSearchFilter = "";
                static std::string searchFilter = "";
                static std::string searchableFilter = "";
                ImGui::InputTextWithHint("##searchFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &searchFilter);
                if (previousSearchFilter != searchFilter) {
                    searchableFilter = ReplaceString(LowerCase(searchFilter), " ", "");
                }
                previousSearchFilter = searchFilter;
                for (auto& preset : s_presets) {
                    std::string category = preset.category;
                    bool searchFilterSatisfied = false;
                    for (auto& preset : s_presets) {
                        if (preset.category != category) continue;
                        searchFilterSatisfied = !(
                            !searchFilter.empty() && preset.filteredName.find(searchableFilter) == std::string::npos
                        );
                        if (searchFilterSatisfied) break;
                    }
                    if (std::find(categories.begin(), categories.end(), category) == categories.end() && searchFilterSatisfied) {
                        categories.push_back(category);
                    }
                }
                ImGui::BeginChild("##internalCurvePresetsContainer", ImVec2(ImGui::GetContentRegionAvail().x, 320));
                for (auto& category : categories) {
                    if (ImGui::TreeNodeEx(FormatString("%s %s", ICON_FA_LIST, category.c_str()).c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
                        for (auto& preset : s_presets) {
                            if (preset.category != category) continue;
                            if ((
                                !searchFilter.empty() && preset.filteredName.find(searchableFilter) == std::string::npos
                            )) continue;
                            if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_BEZIER_CURVE, preset.filteredCategoryName.c_str()).c_str())) {
                                this->m_points = preset.points;
                                ImGui::CloseCurrentPopup();
                            }
                            if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNone) && ImGui::BeginTooltip()) {
                                ImVec2 curvePreviewProcessedSize = ImVec2(220, 220);
                                glm::vec4 previewPoints = preset.points;
                                static glm::vec4 actualTargetPoints = glm::vec4(0, 0, 1, 1);
                                static ImVec2 actualBezierSize = curvePreviewProcessedSize;
                                static glm::vec4 beginPoints, endPoints;
                                static ImVec2 beginBezierSize, endBezierSize;
                                static float animationPercentage;
                                static bool mustAnimate = false;

                                if (previewPoints != actualTargetPoints && beginPoints != actualTargetPoints && beginBezierSize != actualBezierSize) {
                                    beginPoints = actualTargetPoints;
                                    endPoints = previewPoints;

                                    beginBezierSize = actualBezierSize;

                                    std::optional<float> sizeCandidate = std::nullopt;
                                    for (int i = 0; i < 4; i++) {
                                        auto point = previewPoints[i];
                                        if (IsInBounds(point, 0.0f, 1.0f)) continue;
                                        float overshootAmount;
                                        if (point > 1) overshootAmount = point - 1;
                                        else if (point < 0) overshootAmount = +point;
                                        
                                        float overshootMultiplier = 0.7f;
                                        if (sizeCandidate.value_or(-100) < overshootAmount * overshootMultiplier) {
                                            sizeCandidate = std::abs(overshootAmount * overshootMultiplier);
                                        }
                                    }

                                    if (sizeCandidate.has_value()) {
                                        float size = sizeCandidate.value();
                                        endBezierSize = curvePreviewProcessedSize * (1.0f - std::abs(sizeCandidate.value()));
                                    } else {
                                        endBezierSize = curvePreviewProcessedSize;
                                    }
                                    

                                    animationPercentage = 0.0f;
                                    mustAnimate = true;
                                }

                                if (mustAnimate) {
                                    actualTargetPoints = glm::mix(beginPoints, endPoints, std::clamp(animationPercentage, 0.0f, 1.0f));
                                    actualBezierSize = ImLerp(beginBezierSize, endBezierSize, std::clamp(animationPercentage, 0.0f, 1.0f));
                                    animationPercentage += ImGui::GetIO().DeltaTime * 20;
                                }

                                if (animationPercentage > 1) {
                                    beginPoints = glm::vec4(-1);
                                    beginBezierSize = ImVec2(-1, -1);
                                    mustAnimate = false;
                                }
                                

                                ImGui::SeparatorText(FormatString("%s %s", ICON_FA_BEZIER_CURVE, preset.name.c_str()).c_str());


                                ImGui::BeginChild("##presetPreviewCurve", curvePreviewProcessedSize, ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY);
                                    ImGui::Stripes(ImVec4(0.05f, 0.05f, 0.05f, 1), ImVec4(0.1f, 0.1f, 0.1f, 1), 40, 14, curvePreviewProcessedSize);
                                    auto currentCursor = ImGui::GetCursorPos();
                                    ImGui::SetCursorPos(ImGui::GetWindowSize() / 2.0f - actualBezierSize / 2.0f);
                                    ImGui::Bezier("curvePreview", glm::value_ptr(actualTargetPoints), actualBezierSize.x, false, std::nullopt, false);
                                ImGui::EndChild();

                                std::string pointsInfo = FormatString("%s P1: (%0.2f; %0.2f)\n%s P2: (%0.2f; %0.2f)", ICON_FA_BEZIER_CURVE, actualTargetPoints[0], actualTargetPoints[1], ICON_FA_BEZIER_CURVE, actualTargetPoints[2], actualTargetPoints[3]);
                                ImVec2 infoSize = ImGui::CalcTextSize(pointsInfo.c_str());
                                ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - infoSize.x / 2.0f);
                                ImGui::Text("%s", pointsInfo.c_str());

                                ImGui::EndTooltip();
                            }
                        }
                        ImGui::TreePop();
                    }
                }
                ImGui::EndChild();
                ImGui::Separator();
                ImGui::Text("%s %s: %i", ICON_FA_CIRCLE_INFO, Localization::GetString("TOTAL_PRESETS_COUNT").c_str(), (int) s_presets.size());

                ImGui::EndPopup();
            }

            if (ImGui::Button(FormatString("%s %s", m_constrained ? ICON_FA_TOGGLE_ON : ICON_FA_TOGGLE_OFF, Localization::GetString("OVERSHOOT").c_str()).c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                m_constrained = !m_constrained;
            }
            pointsChildWidth = ImGui::GetWindowSize().x;
        ImGui::EndChild();

        ImGui::PopID();
    }

    float BezierEasing::SampleCurveX(double t) {
        return ((m_ax * t + m_bx) * t + m_cx) * t;
    }

    float BezierEasing::SampleCurveY(double t) {
        return ((m_ay * t + m_by) * t + m_cy) * t;
    }

    float BezierEasing::SampleCurveDerivativeX(double t) {
        return (3.0 * m_ax * t + 2.0 * m_bx) * t + m_cx;
    }

    float BezierEasing::SolveCurveX(double x, double epsilon) {
        float t0 = 0, t1 = 0, t2 = 0, x2 = 0, d2 = 0;
        int i = 0;

        for (t2 = x, i = 0; i < 8; i++) {
            x2 = SampleCurveX(t2) - x;
            if (glm::abs(x2) < epsilon)
                return t2;
            d2 = SampleCurveDerivativeX(t2);
            if (glm::abs(d2) < epsilon) break;
            t2 = t2 - x2 / d2;
        }

        t0 = 0.0;
        t1 = 1.0;
        t2 = x;

        if (t2 < t0) return t0;
        if (t2 > t1) return t1;

        while (t0 < t1) {
            x2 = SampleCurveX(t2);
            if (glm::abs(x2 - x) < epsilon) return t2;
            if (x > x2) t0 = t2;
            else t1 = t2;

            t2 = (t1 - t0) * 0.5 + t0;
        }

        return t2;
    }

    float BezierEasing::Solve(double x, double epsilon) {
        return SampleCurveY(SolveCurveX(x, epsilon));
    }
};

extern "C" {
    Raster::AbstractEasing SpawnEasing() {
        return (Raster::AbstractEasing) std::make_shared<Raster::BezierEasing>();
    }

    Raster::EasingDescription GetDescription() {
        return Raster::EasingDescription{
            .prettyName = "Bezier Easing",
            .packageName = RASTER_PACKAGED "bezier_easing"
        };
    }
}