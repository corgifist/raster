#include "overlay_dispatchers.h"
#include "common/randomizer.h"
#include "common/transform2d.h"
#include <cfloat>
#include <cmath>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_drag.h"
#include "font/font.h"
#include "common/workspace.h"

#include "../attributes/transform2d_attribute/transform2d_attribute.h"

#include "common/rendering.h"

#define DRAG_CIRCLE_RADIUS 8
#define ANCHOR_CIRCLE_RADIUS 6
#define ANCHOR_LOGIC_RADIUS 16

namespace Raster {

    struct ObjectDrag {
        int id;
        glm::vec2 position;
        glm::vec2 dragAxis;
        float angle;
        bool isActive;
        bool xDrag;

        ObjectDrag() {
            this->id = Randomizer::GetRandomInteger();
            this->position = glm::vec2(0.5, 0);
            this->dragAxis = glm::vec2(0);
            this->isActive = false;
            this->xDrag = true;
        }

        ObjectDrag(glm::vec2 t_position, glm::vec2 t_dragAxis, float t_angle, bool t_xDrag) : ObjectDrag() {
            this->position = t_position;
            this->dragAxis = t_dragAxis;
            this->isActive = false;
            this->angle = t_angle;
            this->xDrag = t_xDrag;
        }
    };

    struct Transform2DOverlayState {

        std::vector<ObjectDrag> drags;
        std::vector<ObjectDrag> angleDrags;
        ObjectDrag anchorDrag;
        ObjectDrag positionDrag;

        Transform2DOverlayState() {
            this->drags.push_back(ObjectDrag(glm::vec2(1, 0), glm::vec2(1, 0), 0, true));
            this->drags.push_back(ObjectDrag(glm::vec2(0, 1), glm::vec2(0, -1), 90, false));
            this->drags.push_back(ObjectDrag(glm::vec2(-1, 0), glm::vec2(-1, 0), 180, true));
            this->drags.push_back(ObjectDrag(glm::vec2(0, -1), glm::vec2(0, 1), 270, false));

            this->angleDrags.push_back(ObjectDrag(glm::vec2(1, -1), glm::vec2(0, 0), FLT_MIN, false));
            this->angleDrags.push_back(ObjectDrag(glm::vec2(1, 1), glm::vec2(0, 0), FLT_MIN, false));
            this->angleDrags.push_back(ObjectDrag(glm::vec2(-1, 1), glm::vec2(0, 0), FLT_MIN, false));
            this->angleDrags.push_back(ObjectDrag(glm::vec2(-1, -1), glm::vec2(0, 0), FLT_MIN, false));

            this->anchorDrag = ObjectDrag();
            this->positionDrag = ObjectDrag();
        }

        bool AnyOtherDragActive(int t_excludeDrag = -1) {
            if (anchorDrag.id != t_excludeDrag && anchorDrag.isActive) return true;
            if (positionDrag.id != t_excludeDrag && positionDrag.isActive) return true;
            for (auto& drag : drags) {
                if (drag.id == t_excludeDrag) continue;
                if (drag.isActive) return true;
            }
            for (auto& drag : angleDrags) {
                if (drag.id == t_excludeDrag) continue;
                if (drag.isActive) return true;
            }
            return false;
        }
    };

    static void DrawRect(RectBounds bounds, ImVec4 color) {
        ImGui::GetWindowDrawList()->AddRectFilled(
            bounds.UL, bounds.BR, ImGui::ColorConvertFloat4ToU32(color));
    }

    static bool MouseHoveringBounds(RectBounds bounds) {
        return ImGui::IsMouseHoveringRect(bounds.UL, bounds.BR);
    }

    glm::mat2 rotate2d(float _angle){
        return glm::mat2(cos(_angle),-sin(_angle),
                    sin(_angle),cos(_angle));
    }

    std::string OverlayDispatchers::s_attributeName = "";

    bool OverlayDispatchers::DispatchTransform2DValue(std::any& t_attribute, Composition* t_composition, int t_attributeID, float t_zoom, glm::vec2 t_regionSize) {
        static Transform2DOverlayState s_primaryState;
        static std::unordered_map<std::string, Transform2DOverlayState> s_attributeStates;

        Transform2D transform = std::any_cast<Transform2D>(t_attribute);
        Transform2D reservedTransform = transform;
        auto& project = Workspace::s_project.value();
        auto attributeCandidate = Workspace::GetAttributeByAttributeID(t_attributeID);
        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImVec2 cursor = ImGui::GetCursorPos();

        bool transformChanged = false;
        bool blockDragging = false;

        static std::vector<glm::vec2> rectPoints = {
            glm::vec2(-1, 1),
            glm::vec2(-1, -1),
            glm::vec2(1, -1),
            glm::vec2(1, 1)
        };

        auto originalTransformationMatrix = transform.GetTransformationMatrix();

        std::vector<glm::vec2> transformedPoints;
        for (auto& point : rectPoints) {
            glm::vec4 transformedPoint = project.GetProjectionMatrix(true) * transform.GetTransformationMatrix() * glm::vec4(point, 0, 1);
            transformedPoints.push_back(
                {transformedPoint.x, transformedPoint.y}
            );
        }

        std::vector<glm::vec2> screenSpacePoints;
        for (auto& point : transformedPoints) {
            screenSpacePoints.push_back(NDCToScreen(point, t_regionSize));
        }

        ImVec4 outlineColor = ImVec4(1, 1, 1, 1);
        outlineColor.w = 1.0f;
        for (int i = 0; i < screenSpacePoints.size(); i++) {
            auto& point = screenSpacePoints[i];
            auto& nextPoint = i == screenSpacePoints.size() - 1 ? screenSpacePoints[0] : screenSpacePoints[i + 1];
            ImGui::GetWindowDrawList()->AddLine(canvasPos + ImVec2{point.x, point.y}, canvasPos + ImVec2{nextPoint.x, nextPoint.y}, ImGui::GetColorU32(outlineColor), 2);
        }

        auto stringID = attributeCandidate.has_value() ? std::to_string(t_attributeID) : std::to_string(t_attributeID) + s_attributeName;
        Transform2DOverlayState* overlayState = nullptr;
        if (attributeCandidate.has_value()) {
            overlayState = &s_primaryState;
        } else {
            if (s_attributeStates.find(stringID) == s_attributeStates.end()) {
                s_attributeStates[stringID] = Transform2DOverlayState();
            }
            overlayState = &s_attributeStates[stringID];
        }

        if (attributeCandidate.has_value()) {
            auto& attribute = attributeCandidate.value();
            if (attribute->packageName == RASTER_PACKAGED "transform2d_attribute") {
                auto transform2dAttribute = (Transform2DAttribute*) attribute.get();
                if (transform2dAttribute->m_parentAssetID > 0 && transform.parentTransform) {
                    transform = *transform.parentTransform;
                }
            }
        }

        if (!project.customData.contains("Transform2DAttributeData")) {
            project.customData["Transform2DAttributeData"] = {};
        }
        auto& customData = project.customData["Transform2DAttributeData"];
        if (!customData.contains(stringID)) {
            customData[stringID] = false;
        }
        bool linkedSize = customData[stringID];

        for (auto& drag : overlayState->drags) {
            auto dragNDC4 = project.GetProjectionMatrix(true) * originalTransformationMatrix * glm::vec4(drag.position.x, drag.position.y, 0, 1);
            auto dragNDC = glm::vec2(dragNDC4.x, dragNDC4.y);
            auto screenDrag = NDCToScreen(dragNDC, t_regionSize);
            RectBounds dragBounds(
                ImVec2{screenDrag.x - DRAG_CIRCLE_RADIUS * 3 / 2.0f, screenDrag.y - DRAG_CIRCLE_RADIUS * 3 / 2.0f},
                ImVec2(DRAG_CIRCLE_RADIUS * 3, DRAG_CIRCLE_RADIUS * 3)
            );
            ImGui::GetWindowDrawList()->AddCircleFilled(canvasPos + ImVec2{screenDrag.x, screenDrag.y}, DRAG_CIRCLE_RADIUS * t_zoom, ImGui::GetColorU32(MouseHoveringBounds(dragBounds) ? outlineColor * 0.9f : outlineColor));
        
            // DrawRect(dragBounds, ImVec4(1, 0, 0, 1));

            glm::vec2 rotateAxises = rotate2d(transform.DecomposeRotation()) * drag.dragAxis;
            if (!overlayState->AnyOtherDragActive(drag.id) && ImGui::IsMouseDragging(ImGuiMouseButton_Left) && (MouseHoveringBounds(dragBounds) || drag.isActive)) {
                auto reservedSize = transform.size;
                if (drag.xDrag) {
                    transform.size.x += ImGui::GetIO().MouseDelta.x / t_regionSize.x * rotateAxises.x * 2 * (project.preferredResolution.x / project.preferredResolution.y);
                    transform.size.x += ImGui::GetIO().MouseDelta.y / t_regionSize.y * rotateAxises.y * 2;
                } else {
                    transform.size.y += ImGui::GetIO().MouseDelta.x / t_regionSize.x * rotateAxises.x * 2 * (project.preferredResolution.x / project.preferredResolution.y);
                    transform.size.y += ImGui::GetIO().MouseDelta.y / t_regionSize.y * rotateAxises.y * 2;
                }

                if (linkedSize) {
                    if (reservedSize.x != transform.size.x) {
                        transform.size.y = transform.size.x;
                    } else if (reservedSize.y != transform.size.y) {
                        transform.size.x = transform.size.y;
                    }
                }
                transformChanged = true;
                drag.isActive = true;
            } else drag.isActive = false;
            if (MouseHoveringBounds(dragBounds) && ImGui::IsWindowFocused()) {
                ImGui::SetMouseCursor(glm::abs(rotateAxises.x) >= glm::abs(rotateAxises.y) ? ImGuiMouseCursor_ResizeEW : ImGuiMouseCursor_ResizeNS);
            }
        }

        glm::mat4 anchorTransformMatrix = glm::identity<glm::mat4>();
        anchorTransformMatrix = glm::translate(anchorTransformMatrix, glm::vec3(transform.position, 0));
        anchorTransformMatrix = glm::translate(anchorTransformMatrix, glm::vec3(transform.anchor, 0));
        anchorTransformMatrix = transform.GetParentMatrix() * anchorTransformMatrix;

        glm::vec4 anchorPointNDC4 = project.GetProjectionMatrix(true) * anchorTransformMatrix * glm::vec4(0, 0, 0, 1);
        glm::vec2 anchorPointScreen = glm::vec2(anchorPointNDC4.x, anchorPointNDC4.y);
        anchorPointScreen = NDCToScreen(anchorPointScreen, t_regionSize);
        
        RectBounds anchorPointBounds(
            ImVec2{anchorPointScreen.x, anchorPointScreen.y} - ImVec2(ANCHOR_LOGIC_RADIUS * t_zoom * 3 / 2.0f, ANCHOR_LOGIC_RADIUS * t_zoom * 3 / 2.0f),
            ImVec2(ANCHOR_LOGIC_RADIUS * 3 * t_zoom, ANCHOR_LOGIC_RADIUS * 3 * t_zoom)
        );

        ImGui::GetWindowDrawList()->AddCircleFilled(canvasPos + ImVec2{anchorPointScreen.x, anchorPointScreen.y}, ANCHOR_CIRCLE_RADIUS * t_zoom, ImGui::GetColorU32(MouseHoveringBounds(anchorPointBounds) ? outlineColor * 0.9f : outlineColor));

        if ((MouseHoveringBounds(anchorPointBounds) || overlayState->anchorDrag.isActive) && !overlayState->AnyOtherDragActive(overlayState->anchorDrag.id) && ImGui::IsMouseDragging(ImGuiMouseButton_Left) ) {
            transform.anchor.x += ImGui::GetIO().MouseDelta.x / t_regionSize.x * 2 * (project.preferredResolution.x / project.preferredResolution.y);
            transform.anchor.y -= ImGui::GetIO().MouseDelta.y / t_regionSize.y * 2;
            transformChanged = true;
            overlayState->anchorDrag.isActive = true;
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);
        } else overlayState->anchorDrag.isActive = false;

        float aspectRatio = t_regionSize.x / t_regionSize.y;
        glm::vec4 centerPointNDC4(0, 0, 0, 1);
        centerPointNDC4 = project.GetProjectionMatrix(true) * originalTransformationMatrix * centerPointNDC4;
        glm::vec2 centerPointScreen(centerPointNDC4.x, centerPointNDC4.y);
        centerPointScreen = NDCToScreen(centerPointScreen, t_regionSize);
        glm::vec2 transformScreenSize(transform.size.x / aspectRatio * t_regionSize.x, transform.size.y * t_regionSize.y);

        RectBounds positionDragBounds(
            {centerPointScreen.x - transformScreenSize.x / 2.0f, centerPointScreen.y - transformScreenSize.y / 2.0f},
            {transformScreenSize.x, transformScreenSize.y}
        );
        // DrawRect(positionDragBounds, ImVec4(1, 0, 0, 1));

        if ((MouseHoveringBounds(positionDragBounds) || overlayState->positionDrag.isActive) && !overlayState->AnyOtherDragActive(overlayState->positionDrag.id) && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            transform.position.x += ImGui::GetIO().MouseDelta.x / t_regionSize.x * 2 * (project.preferredResolution.x / project.preferredResolution.y);
            transform.position.y -= ImGui::GetIO().MouseDelta.y / t_regionSize.y * 2;
            transformChanged = true;
            overlayState->positionDrag.isActive = true;
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNESW);
        } else overlayState->positionDrag.isActive = false;

        for (auto& drag : overlayState->angleDrags) {
            auto dragNDC4 = project.GetProjectionMatrix(true) * originalTransformationMatrix * glm::vec4(drag.position.x, drag.position.y, 0, 1);
            auto dragNDC = glm::vec2(dragNDC4.x, dragNDC4.y);
            auto screenDrag = NDCToScreen(dragNDC, t_regionSize);
            RectBounds dragBounds(
                ImVec2{screenDrag.x - DRAG_CIRCLE_RADIUS * 3 / 2.0f, screenDrag.y - DRAG_CIRCLE_RADIUS * 3 / 2.0f},
                ImVec2(DRAG_CIRCLE_RADIUS * 3, DRAG_CIRCLE_RADIUS * 3)
            );
            ImVec2 mousePos = ImGui::GetMousePos();
            glm::vec4 angleCenterPoint4 = project.GetProjectionMatrix(true) * originalTransformationMatrix * glm::vec4(0, 0, 0, 1);
            glm::vec2 angleCenterPoint = glm::vec2(angleCenterPoint4.x, angleCenterPoint4.y);
            angleCenterPoint = NDCToScreen(angleCenterPoint, t_regionSize);
            angleCenterPoint.x += ImGui::GetCursorScreenPos().x;
            angleCenterPoint.y += ImGui::GetCursorScreenPos().y;
            if (!overlayState->AnyOtherDragActive(drag.id) && ImGui::IsMouseDragging(ImGuiMouseButton_Left) && (MouseHoveringBounds(dragBounds) || drag.isActive)) {
                auto angle = glm::degrees(std::atan2(angleCenterPoint.y - mousePos.y, angleCenterPoint.x - mousePos.x));
                if (drag.angle == FLT_MIN) {
                    drag.angle = angle;
                }

                transform.angle += angle - drag.angle;

                drag.angle = angle;
                transformChanged = true;
                drag.isActive = true;
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            } else {
                drag.isActive = false;
                drag.angle = FLT_MIN;
            }
        }

        if (transformChanged && attributeCandidate.has_value()) {
            auto& attribute = attributeCandidate.value();
            float compositionRelativeTime = std::floor(project.currentFrame - t_composition->beginFrame);
            if (attribute->keyframes.size() == 1) {
                attribute->keyframes[0].value = transform;
            } else if (attribute->KeyframeExists(compositionRelativeTime) && attribute->keyframes.size() > 1) {
                auto keyframe = attribute->GetKeyframeByTimestamp(compositionRelativeTime).value();
                keyframe->value = transform;
            } else {
                attribute->keyframes.push_back(AttributeKeyframe(compositionRelativeTime, transform));
            }
        }


        if (attributeCandidate.has_value()) {
            auto& attribute = attributeCandidate.value();
            if (attribute->packageName == RASTER_PACKAGED "transform2d_attribute") {
                auto transform2dAttribute = (Transform2DAttribute*) attribute.get();
                if (transform2dAttribute->m_parentAssetID > 0 && transform.parentTransform) {
                    Transform2D correctedTransform = reservedTransform;
                    correctedTransform.parentTransform = std::make_shared<Transform2D>(transform);
                    transform = correctedTransform;
                }
            }
        }
        t_attribute = transform;
        if (transformChanged) {
            Rendering::ForceRenderFrame();
        }
        return !transformChanged; 
    }
};