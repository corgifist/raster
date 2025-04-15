#include "camera_attribute.h"
#include "common/ui_helpers.h"
#include "common/asset_id.h"
#include "common/dispatchers.h"
#include "common/camera.h"
#include "common/item_aligner.h"

namespace Raster {
    CameraAttribute::CameraAttribute() {
        AttributeBase::Initialize();

        keyframes.push_back(
            AttributeKeyframe(
                0,
                Camera()
            )
        );
    }

    std::any CameraAttribute::AbstractInterpolate(std::any t_beginValue, std::any t_endValue, float t_percentage, float t_frame, Composition* composition) {
        Camera beginCamera = std::any_cast<Camera>(t_beginValue);
        Camera endCamera = std::any_cast<Camera>(t_endValue);
        Camera newCamera = beginCamera;
        if (beginCamera.persp && endCamera.persp) {
            newCamera.perspNear = glm::mix(beginCamera.perspNear, endCamera.perspNear, t_percentage);
            newCamera.perspFar = glm::mix(beginCamera.perspFar, endCamera.perspFar, t_percentage);
            newCamera.f = glm::mix(beginCamera.f, endCamera.f, t_percentage);
            newCamera.fSize = glm::mix(beginCamera.fSize, endCamera.fSize, t_percentage);
            newCamera.fLength = glm::mix(beginCamera.fLength, endCamera.fLength, t_percentage);
        } else if (!beginCamera.persp && !endCamera.persp) {
            newCamera.orthoWidth = glm::mix(beginCamera.orthoWidth, endCamera.orthoWidth, t_percentage);
        }
        return newCamera;
    }

    void CameraAttribute::RenderKeyframes() {
        for (auto& keyframe : keyframes) {
            RenderKeyframe(keyframe);
        }
    }

    Json CameraAttribute::SerializeKeyframeValue(std::any t_value) {
        return std::any_cast<Camera>(t_value).Serialize();
    }  

    std::any CameraAttribute::LoadKeyframeValue(Json t_value) {
        return Camera(t_value);
    }

    std::any CameraAttribute::AbstractRenderLegend(Composition* t_composition, std::any t_originalValue, bool& isItemEdited) {
        auto& project = Workspace::GetProject();
        auto camera = std::any_cast<Camera>(t_originalValue);
        static ItemAligner s_aligner;
        s_aligner.ClearCursors();
        if (ImGui::Button(FormatString("%s %s", ICON_FA_CAMERA, Localization::GetString(camera.persp ? "PERSP_PROJECTION" : "ORTHO_PROJECTION").c_str()).c_str(), ImVec2(ImGui::GetContentRegionAvail().x - ImGui::GetStyle().WindowPadding.x, 0))) {
            ImGui::OpenPopup("##settingsPopup");
        }
        if (ImGui::BeginPopup("##settingsPopup")) {
            ImGui::SeparatorText(FormatString("%s %s: %s", ICON_FA_CAMERA, Localization::GetString("EDIT_VALUE").c_str(), name.c_str()).c_str());
            if (ImGui::Button(camera.enabled ? ICON_FA_TOGGLE_ON : ICON_FA_TOGGLE_OFF)) {
                camera.enabled = !camera.enabled;
                isItemEdited = true;
            }
            ImGui::SetItemTooltip("%s %s", camera.enabled ? ICON_FA_TOGGLE_ON : ICON_FA_TOGGLE_OFF, Localization::GetString("ENABLE_DISABLE_CAMERA").c_str());
            ImGui::SameLine();
            if (ImGui::Button(FormatString("%s %s", ICON_FA_CAMERA, Localization::GetString(camera.persp ? "PERSP_PROJECTION" : "ORTHO_PROJECTION").c_str()).c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                camera.persp = !camera.persp;
                isItemEdited = true;
            }
            if (camera.persp && ImGui::Button(FormatString("%s%s %s", camera.customF ? ICON_FA_CHECK : ICON_FA_XMARK, ICON_FA_EYE, Localization::GetString("USE_CUSTOM_FOV").c_str()).c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                camera.customF = !camera.customF;
                isItemEdited = true;
            }

            if (camera.persp) {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s %s", ICON_FA_LEFT_RIGHT, Localization::GetString("NEAR_PLANE").c_str());
                ImGui::SameLine();
                s_aligner.AlignCursor();
                ImGui::DragFloat("##nearPlane", &camera.perspNear, 0.001);
                if (ImGui::IsItemEdited()) isItemEdited = true;
    
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s %s", ICON_FA_LEFT_RIGHT, Localization::GetString("FAR_PLANE").c_str());
                ImGui::SameLine();
                s_aligner.AlignCursor();
                ImGui::DragFloat("##farPlane", &camera.perspFar, 0.001);
                if (ImGui::IsItemEdited()) isItemEdited = true;

                if (camera.customF) {
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s %s", ICON_FA_EYE, Localization::GetString("FOV").c_str());
                    ImGui::SameLine();
                    s_aligner.AlignCursor();
                    ImGui::SliderFloat("##fSlider", &camera.f, 1, 180);
                    if (ImGui::IsItemEdited()) isItemEdited = true;
                } else {
                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s %s", ICON_FA_EXPAND, Localization::GetString("CAMERA_FILM_SIZE").c_str());
                    ImGui::SameLine();
                    s_aligner.AlignCursor();
                    ImGui::DragFloat("##sizeSlider", &camera.fSize, 1.0f, 0.0f, 0.0f, "%0.1f mm");
                    if (ImGui::IsItemEdited()) isItemEdited = true;

                    ImGui::AlignTextToFramePadding();
                    ImGui::Text("%s %s", ICON_FA_EXPAND, Localization::GetString("CAMERA_FILM_LENGTH").c_str());
                    ImGui::SameLine();
                    s_aligner.AlignCursor();
                    ImGui::DragFloat("##lengthSlider", &camera.fLength, 1.0f, 0.0f, 0.0f, "%0.1f mm");
                    if (ImGui::IsItemEdited()) isItemEdited = true;

                    ImGui::Text("%s %s: %0.2f", ICON_FA_GEARS, Localization::GetString("ESTIMATED_CAMERA_FOV").c_str(), camera.GetF());
                }
            } else {
                ImGui::AlignTextToFramePadding();
                ImGui::Text("%s %s", ICON_FA_LEFT_RIGHT, Localization::GetString("ORTHO_WIDTH").c_str());
                ImGui::SameLine();
                s_aligner.AlignCursor();
                ImGui::SliderFloat("##orthoWidth", &camera.orthoWidth, 0.1, 15);
                if (ImGui::IsItemEdited()) isItemEdited = true;
            }

            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s Position", ICON_FA_UP_DOWN_LEFT_RIGHT);
            ImGui::SameLine();
            s_aligner.AlignCursor();
            ImGui::DragFloat3("##dragPosition", glm::value_ptr(camera.position), 0.05f);
            isItemEdited = isItemEdited || ImGui::IsItemEdited();

            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s Rotation", ICON_FA_ROTATE);
            ImGui::SameLine();
            s_aligner.AlignCursor();
            ImGui::DragFloat3("##rotationDrag", glm::value_ptr(camera.rotation), 0.05f);
            isItemEdited = isItemEdited || ImGui::IsItemEdited();

            ImGui::AlignTextToFramePadding();
            ImGui::Text("%s Anchor", ICON_FA_ANCHOR);
            ImGui::SameLine();
            s_aligner.AlignCursor();
            ImGui::DragFloat3("##anchorDrag", glm::value_ptr(camera.anchor), 0.05f);
            isItemEdited = isItemEdited || ImGui::IsItemEdited();

            std::string buttonText = FormatString("%s %s", ICON_FA_CUBE " " ICON_FA_UP_DOWN_LEFT_RIGHT, Localization::GetString("SELECT_PARENT_ATTRIBUTE").c_str());
            auto attributeCandidate = Workspace::GetAttributeByAttributeID(parentAttributeID);
            if (attributeCandidate) {
                buttonText = FormatString("%s %s", ICON_FA_CUBE " " ICON_FA_UP_DOWN_LEFT_RIGHT, FormatString(Localization::GetString("PARENT_ATTRIBUTE"), (*attributeCandidate)->name.c_str()).c_str());
            }
            if (ImGui::Button(buttonText.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0))) {
                UIHelpers::OpenSelectAttributePopup();
            }
            UIHelpers::SelectAttribute(t_composition, parentAttributeID, FormatString("%s %s", ICON_FA_CUBE " " ICON_FA_UP_DOWN_LEFT_RIGHT, Localization::GetString("SELECT_PARENT_ATTRIBUTE").c_str()).c_str());
            ImGui::EndPopup();
        }

        return camera;
    }

    void CameraAttribute::AbstractRenderDetails() {
        auto& project = Workspace::s_project.value();
        auto parentComposition = Workspace::GetCompositionByAttributeID(id).value();
        ImGui::PushID(id);
            auto currentValue = Get(project.GetCorrectCurrentTime() - parentComposition->GetBeginFrame(), parentComposition);
            Dispatchers::DispatchString(currentValue);
        ImGui::PopID();
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractAttribute SpawnAttribute() {
        return (Raster::AbstractAttribute) std::make_shared<Raster::CameraAttribute>();
    }

    RASTER_DL_EXPORT Raster::AttributeDescription GetDescription() {
        return Raster::AttributeDescription{
            .packageName = RASTER_PACKAGED "camera_attribute",
            .prettyName = ICON_FA_CAMERA " Camera"
        };
    }
}