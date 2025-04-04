#include "overlay_dispatchers.h"
#include "common/bezier_curve.h"
#include "common/localization.h"
#include "common/randomizer.h"
#include "common/transform2d.h"
#include "font/IconsFontAwesome5.h"
#include "raster.h"
#include <cfloat>
#include <cmath>
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../../ImGui/imgui.h"
#include "../../ImGui/imgui_drag.h"
#include "font/font.h"
#include "common/workspace.h"

#include "../../attributes/transform2d_attribute/transform2d_attribute.h"

#include "common/rendering.h"
#include "common/line2d.h"

#include "common/dispatchers.h"

#define DRAG_CIRCLE_RADIUS 8
#define DRAG_LOGIC_RADIUS 16
#define ANCHOR_CIRCLE_RADIUS 6
#define ANCHOR_LOGIC_RADIUS 12

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
            float compositionRelativeTime = std::floor(project.GetCorrectCurrentTime() - t_composition->GetBeginFrame());
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

    struct Line2DOverlayState {
        ObjectDrag beginDrag, endDrag;
        ObjectDrag middleDrag;

        bool AnyOtherDragActive(int t_excludeDrag = -1) {
            if (beginDrag.id != t_excludeDrag && beginDrag.isActive) return true;
            if (endDrag.id != t_excludeDrag && endDrag.isActive) return true;
            if (middleDrag.id != t_excludeDrag && middleDrag.isActive) return true;
            return false;
        }
    };

    bool OverlayDispatchers::DispatchLine2DValue(std::any& t_attribute, Composition* t_composition, int t_attributeID, float t_zoom, glm::vec2 t_regionSize) {
        static Line2DOverlayState s_primaryState;
        static std::unordered_map<std::string, Line2DOverlayState> s_attributeStates;

        Line2D line = std::any_cast<Line2D>(t_attribute);
        auto& project = Workspace::s_project.value();
        auto attributeCandidate = Workspace::GetAttributeByAttributeID(t_attributeID);
        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImVec2 cursor = ImGui::GetCursorPos();

        bool lineChanged = false;
        bool blockDragging = false;


        std::vector<glm::vec2> originalPoints = {
            line.begin, line.end
        };
        std::vector<glm::vec2> transformedPoints;
        for (auto& point : originalPoints) {
            glm::vec4 transformedPoint = project.GetProjectionMatrix(true) * glm::vec4(point, 0, 1);
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
        Line2DOverlayState* overlayState = nullptr;
        if (attributeCandidate.has_value()) {
            overlayState = &s_primaryState;
        } else {
            if (s_attributeStates.find(stringID) == s_attributeStates.end()) {
                s_attributeStates[stringID] = Line2DOverlayState();
            }
            overlayState = &s_attributeStates[stringID];
        }

        std::vector<ObjectDrag*> lineDrags = {
            &overlayState->beginDrag, &overlayState->endDrag
        };

        bool beginDrag = true;
        for (auto& dragPtr : lineDrags) {
            auto& drag = *dragPtr;
            auto dragNDC4 = project.GetProjectionMatrix(true) * glm::vec4(beginDrag ? line.begin.x : line.end.x, beginDrag ? line.begin.y : line.end.y, 0, 1);
            auto dragNDC = glm::vec2(dragNDC4.x, dragNDC4.y);
            auto screenDrag = NDCToScreen(dragNDC, t_regionSize);
            RectBounds dragBounds(
                ImVec2{screenDrag.x - DRAG_CIRCLE_RADIUS * 3 / 2.0f, screenDrag.y - DRAG_CIRCLE_RADIUS * 3 / 2.0f},
                ImVec2(DRAG_CIRCLE_RADIUS * 3, DRAG_CIRCLE_RADIUS * 3)
            );
            ImVec4 dragColor = beginDrag ? ImVec4{line.beginColor.r, line.beginColor.g, line.beginColor.b, line.beginColor.a} : ImVec4{line.endColor.r, line.endColor.g, line.endColor.b, line.endColor.a};
            ImGui::GetWindowDrawList()->AddCircleFilled(canvasPos + ImVec2{screenDrag.x, screenDrag.y}, DRAG_CIRCLE_RADIUS * t_zoom, ImGui::GetColorU32(MouseHoveringBounds(dragBounds) ? dragColor * 0.9f : dragColor));
        
            // DrawRect(dragBounds, ImVec4(1, 0, 0, 1));
            ImGui::PushID(beginDrag);
            if (!overlayState->AnyOtherDragActive(drag.id) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && MouseHoveringBounds(dragBounds)) {
                ImGui::OpenPopup("##recolorDrag");
            }
            if (ImGui::BeginPopup("##recolorDrag")) {
                ImGui::ColorEdit4("##recolorDragEditor", beginDrag ? glm::value_ptr(line.beginColor) : glm::value_ptr(line.endColor));
                if (ImGui::IsItemEdited()) {
                    lineChanged = true;
                }
                ImGui::EndPopup();
            }
            ImGui::PopID();
            if (!overlayState->AnyOtherDragActive(drag.id) && ImGui::IsMouseDragging(ImGuiMouseButton_Left) && (MouseHoveringBounds(dragBounds) || drag.isActive)) {
                if (beginDrag) {
                    line.begin.x += ImGui::GetIO().MouseDelta.x / t_regionSize.x * 2 * (project.preferredResolution.x / project.preferredResolution.y);
                    line.begin.y -= ImGui::GetIO().MouseDelta.y / t_regionSize.y * 2;
                } else {
                    line.end.x += ImGui::GetIO().MouseDelta.x / t_regionSize.x * 2 * (project.preferredResolution.x / project.preferredResolution.y);
                    line.end.y -= ImGui::GetIO().MouseDelta.y / t_regionSize.y * 2;
                }
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                lineChanged = true;
                drag.isActive = true;
            } else drag.isActive = false;
            beginDrag = false;
        }

        {
            auto& drag = overlayState->middleDrag;
            auto middlePos = glm::mix(line.begin, line.end, 0.5f);
            auto dragNDC4 = project.GetProjectionMatrix(true) * glm::vec4(middlePos, 0, 1);
            auto dragNDC = glm::vec2(dragNDC4.x, dragNDC4.y);
            auto screenDrag = NDCToScreen(dragNDC, t_regionSize);
            RectBounds dragBounds(
                ImVec2{screenDrag.x - DRAG_CIRCLE_RADIUS * 3 / 2.0f, screenDrag.y - DRAG_CIRCLE_RADIUS * 3 / 2.0f},
                ImVec2(DRAG_CIRCLE_RADIUS * 3, DRAG_CIRCLE_RADIUS * 3)
            );
            ImGui::GetWindowDrawList()->AddCircleFilled(canvasPos + ImVec2{screenDrag.x, screenDrag.y}, DRAG_CIRCLE_RADIUS * t_zoom, ImGui::GetColorU32(MouseHoveringBounds(dragBounds) ? outlineColor * 0.9f : outlineColor));
        
            // DrawRect(dragBounds, ImVec4(1, 0, 0, 1));

            if (!overlayState->AnyOtherDragActive(drag.id) && ImGui::IsMouseDragging(ImGuiMouseButton_Left) && (MouseHoveringBounds(dragBounds) || drag.isActive)) {
                line.begin.x += ImGui::GetIO().MouseDelta.x / t_regionSize.x * 2 * (project.preferredResolution.x / project.preferredResolution.y);
                line.begin.y -= ImGui::GetIO().MouseDelta.y / t_regionSize.y * 2;
                line.end.x += ImGui::GetIO().MouseDelta.x / t_regionSize.x * 2 * (project.preferredResolution.x / project.preferredResolution.y);
                line.end.y -= ImGui::GetIO().MouseDelta.y / t_regionSize.y * 2;
                lineChanged = true;
                drag.isActive = true;
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            } else drag.isActive = false;
            beginDrag = false; 
        }

        if (lineChanged && attributeCandidate.has_value()) {
            auto& attribute = attributeCandidate.value();
            float compositionRelativeTime = std::floor(project.GetCorrectCurrentTime() - t_composition->GetBeginFrame());
            if (attribute->keyframes.size() == 1) {
                attribute->keyframes[0].value = line;
            } else if (attribute->KeyframeExists(compositionRelativeTime) && attribute->keyframes.size() > 1) {
                auto keyframe = attribute->GetKeyframeByTimestamp(compositionRelativeTime).value();
                keyframe->value = line;
            } else {
                attribute->keyframes.push_back(AttributeKeyframe(compositionRelativeTime, line));
            }
        }

        t_attribute = line;
        if (lineChanged) {
            Rendering::ForceRenderFrame();
        }
        return !lineChanged; 
    }

    struct ROIOverlayState {
        ObjectDrag beginDrag, endDrag;
        ObjectDrag middleDrag;
        ObjectDrag positionDrag;

        bool AnyOtherDragActive(int t_excludeDrag = -1) {
            if (positionDrag.id != t_excludeDrag && positionDrag.isActive) return true;
            if (beginDrag.id != t_excludeDrag && beginDrag.isActive) return true;
            if (endDrag.id != t_excludeDrag && endDrag.isActive) return true;
            if (middleDrag.id != t_excludeDrag && middleDrag.isActive) return true;
            return false;
        }
    };

    bool OverlayDispatchers::DispatchROIValue(std::any& t_attribute, Composition* t_composition, int t_attributeID, float t_zoom, glm::vec2 t_regionSize) {
        static ROIOverlayState s_primaryState;
        static std::unordered_map<std::string, ROIOverlayState> s_attributeStates;

        ROI roi = std::any_cast<ROI>(t_attribute);
        auto& project = Workspace::s_project.value();
        auto attributeCandidate = Workspace::GetAttributeByAttributeID(t_attributeID);
        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImVec2 cursor = ImGui::GetCursorPos();

        bool roiChanged = false;
        bool blockDragging = false;

        std::vector<glm::vec2> originalPoints = {
            roi.upperLeft, glm::vec2(roi.bottomRight.x, roi.upperLeft.y),
            roi.bottomRight, glm::vec2(roi.upperLeft.x, roi.bottomRight.y),

        };
        std::vector<glm::vec2> transformedPoints;
        for (auto& point : originalPoints) {
            glm::vec4 transformedPoint = project.GetProjectionMatrix(true) * glm::vec4(point, 0, 1);
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
        ROIOverlayState* overlayState = nullptr;
        if (attributeCandidate.has_value()) {
            overlayState = &s_primaryState;
        } else {
            if (s_attributeStates.find(stringID) == s_attributeStates.end()) {
                s_attributeStates[stringID] = ROIOverlayState();
            }
            overlayState = &s_attributeStates[stringID];
        }

        std::vector<ObjectDrag*> lineDrags = {
            &overlayState->beginDrag, &overlayState->endDrag
        };

        bool beginDrag = true;
        for (auto& dragPtr : lineDrags) {
            auto& drag = *dragPtr;
            auto dragNDC4 = project.GetProjectionMatrix(true) * glm::vec4(beginDrag ? roi.upperLeft.x : roi.bottomRight.x, beginDrag ? roi.upperLeft.y : roi.bottomRight.y, 0, 1);
            auto dragNDC = glm::vec2(dragNDC4.x, dragNDC4.y);
            auto screenDrag = NDCToScreen(dragNDC, t_regionSize);
            RectBounds dragBounds(
                ImVec2{screenDrag.x - DRAG_CIRCLE_RADIUS * 3 / 2.0f, screenDrag.y - DRAG_CIRCLE_RADIUS * 3 / 2.0f},
                ImVec2(DRAG_CIRCLE_RADIUS * 3, DRAG_CIRCLE_RADIUS * 3)
            );
            ImVec4 dragColor = ImVec4(1.0f);
            ImGui::GetWindowDrawList()->AddCircleFilled(canvasPos + ImVec2{screenDrag.x, screenDrag.y}, DRAG_CIRCLE_RADIUS * t_zoom, ImGui::GetColorU32(MouseHoveringBounds(dragBounds) ? dragColor * 0.9f : dragColor));
        
            if (!overlayState->AnyOtherDragActive(drag.id) && ImGui::IsMouseDragging(ImGuiMouseButton_Left) && (MouseHoveringBounds(dragBounds) || drag.isActive)) {
                if (beginDrag) {
                    roi.upperLeft.x += ImGui::GetIO().MouseDelta.x / t_regionSize.x * 2 * (project.preferredResolution.x / project.preferredResolution.y);
                    roi.upperLeft.y -= ImGui::GetIO().MouseDelta.y / t_regionSize.y * 2;
                } else {
                    roi.bottomRight.x += ImGui::GetIO().MouseDelta.x / t_regionSize.x * 2 * (project.preferredResolution.x / project.preferredResolution.y);
                    roi.bottomRight.y -= ImGui::GetIO().MouseDelta.y / t_regionSize.y * 2;
                }
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                roiChanged = true;
                drag.isActive = true;
            } else drag.isActive = false;
            beginDrag = false;
        }

        {
            auto& drag = overlayState->middleDrag;
            auto middlePos = glm::mix(roi.upperLeft, roi.bottomRight, 0.5f);
            auto dragNDC4 = project.GetProjectionMatrix(true) * glm::vec4(middlePos, 0, 1);
            auto dragNDC = glm::vec2(dragNDC4.x, dragNDC4.y);
            auto screenDrag = NDCToScreen(dragNDC, t_regionSize);
            RectBounds dragBounds(
                ImVec2{screenDrag.x - DRAG_CIRCLE_RADIUS * 3 / 2.0f, screenDrag.y - DRAG_CIRCLE_RADIUS * 3 / 2.0f},
                ImVec2(DRAG_CIRCLE_RADIUS * 3, DRAG_CIRCLE_RADIUS * 3)
            );
            ImGui::GetWindowDrawList()->AddCircleFilled(canvasPos + ImVec2{screenDrag.x, screenDrag.y}, DRAG_CIRCLE_RADIUS * t_zoom, ImGui::GetColorU32(MouseHoveringBounds(dragBounds) ? outlineColor * 0.9f : outlineColor));
        
            // DrawRect(dragBounds, ImVec4(1, 0, 0, 1));

            if (!overlayState->AnyOtherDragActive(drag.id) && ImGui::IsMouseDragging(ImGuiMouseButton_Left) && (MouseHoveringBounds(dragBounds) || drag.isActive)) {
                roi.upperLeft.x += ImGui::GetIO().MouseDelta.x / t_regionSize.x * 2 * (project.preferredResolution.x / project.preferredResolution.y);
                roi.upperLeft.y -= ImGui::GetIO().MouseDelta.y / t_regionSize.y * 2;
                roi.bottomRight.x += ImGui::GetIO().MouseDelta.x / t_regionSize.x * 2 * (project.preferredResolution.x / project.preferredResolution.y);
                roi.bottomRight.y -= ImGui::GetIO().MouseDelta.y / t_regionSize.y * 2;
                roiChanged = true;
                drag.isActive = true;
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            } else drag.isActive = false;
            beginDrag = false; 
        }

        {
            auto aspectRatio = t_regionSize.x / t_regionSize.y;
            glm::vec4 centerPointNDC4(glm::mix(roi.upperLeft, roi.bottomRight, 0.5f), 0, 1);
            centerPointNDC4 = project.GetProjectionMatrix(true) * centerPointNDC4;
            glm::vec2 centerPointScreen(centerPointNDC4.x, centerPointNDC4.y); 
            centerPointScreen = NDCToScreen(centerPointScreen, t_regionSize);
            auto ndcUpperLeft = NDCToScreen(roi.upperLeft, glm::vec2(1)); 
            auto ndcBottomRight = NDCToScreen(roi.bottomRight, glm::vec2(1));
            glm::vec2 transformScreenSize(glm::abs(ndcBottomRight.x - ndcUpperLeft.x) * t_regionSize.x, glm::abs(ndcBottomRight.y - ndcUpperLeft.y) * t_regionSize.y);
            // DUMP_VAR(transformScreenSize.x);

            RectBounds positionDragBounds(
                {centerPointScreen.x - transformScreenSize.x / 2.0f, centerPointScreen.y - transformScreenSize.y / 2.0f},
                {transformScreenSize.x, transformScreenSize.y}
            );
            // DrawRect(positionDragBounds, ImVec4(1, 0, 0, 1));
            if (MouseHoveringBounds(positionDragBounds) && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                Dispatchers::s_blockPopups = true;
                ImGui::OpenPopup(FormatString("##roiPopup%s", stringID.c_str()).c_str());
            }

            if ((MouseHoveringBounds(positionDragBounds) || overlayState->positionDrag.isActive) && !overlayState->AnyOtherDragActive(overlayState->positionDrag.id) && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                roi.upperLeft.x += ImGui::GetIO().MouseDelta.x / t_regionSize.x * 2 * (project.preferredResolution.x / project.preferredResolution.y);
                roi.upperLeft.y -= ImGui::GetIO().MouseDelta.y / t_regionSize.y * 2;
                roi.bottomRight.x += ImGui::GetIO().MouseDelta.x / t_regionSize.x * 2 * (project.preferredResolution.x / project.preferredResolution.y);
                roi.bottomRight.y -= ImGui::GetIO().MouseDelta.y / t_regionSize.y * 2;
                roiChanged = true;
                overlayState->positionDrag.isActive = true;
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            } else overlayState->positionDrag.isActive = false;
        }

        if (ImGui::BeginPopup(FormatString("##roiPopup%s", stringID.c_str()).c_str())) {
            ImGui::SeparatorText(FormatString("%s %s", ICON_FA_EXPAND, Localization::GetString("REGION_OF_INTEREST").c_str()).c_str());
            Dispatchers::s_blockPopups = true;
            if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_IMAGE, Localization::GetString("RESIZE_TO_MATCH_PROJECT_RESOLUTION").c_str()).c_str())) {
                auto aspectRatio = t_regionSize.x / t_regionSize.y;
                roi.upperLeft = glm::vec2(aspectRatio, -1);
                roi.bottomRight = glm::vec2(-aspectRatio, 1);
                roiChanged = true;
                blockDragging = true;
            }
            if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_GEARS, Localization::GetString("RESET_TO_DEFAULT").c_str()).c_str())) {
                roi = ROI();
                roiChanged = true;
                blockDragging = true;
            }
            ImGui::EndPopup();
        }

        t_attribute = roi;
        if (roiChanged) {
            Rendering::ForceRenderFrame();
        }
        return !roiChanged; 
    }

    struct BezierCurveOverlayState {
        std::vector<ObjectDrag> drags;
        ObjectDrag middleDrag;

        bool AnyOtherDragActive(int t_excludeDrag = -1) {
            if (middleDrag.id != t_excludeDrag && middleDrag.isActive) return true;
            for (auto& drag : drags) {
                if (drag.id != t_excludeDrag && drag.isActive) return true;
            }
            return false;
        }
    };

    static void AddLineDashed(const ImVec2& a, const ImVec2& b, ImU32 col, float thickness, unsigned int num_segments, unsigned int on_segments, unsigned int off_segments)
    {
        if ((col >> 24) == 0)
            return;
        int on = 0, off = 0;
        ImVec2 dir = (b - a) / num_segments;
        for (int i = 0; i <= num_segments; i++)
        {
            ImVec2 point(a + dir * i);
            if(on < on_segments) {
                ImGui::GetWindowDrawList()->PathLineTo(point);
                on++;
            } else if(on == on_segments && off == 0) {
                ImGui::GetWindowDrawList()->PathLineTo(point);
                ImGui::GetWindowDrawList()->PathStroke(col, false, thickness);
                off++;
            } else if(on == on_segments && off < off_segments) {
                off++;
            } else {
                ImGui::GetWindowDrawList()->PathClear();
                ImGui::GetWindowDrawList()->PathLineTo(point);
                on=1;
                off=0;
            }
        }
        ImGui::GetWindowDrawList()->PathStroke(col, false, thickness);
    }

    bool OverlayDispatchers::DispatchBezierCurve(std::any& t_attribute, Composition* t_composition, int t_attributeID, float t_zoom, glm::vec2 t_regionSize) {
        static BezierCurveOverlayState s_primaryState;
        static std::unordered_map<std::string, BezierCurveOverlayState> s_attributeStates;

        BezierCurve bezier = std::any_cast<BezierCurve>(t_attribute);
        auto& project = Workspace::s_project.value();
        auto attributeCandidate = Workspace::GetAttributeByAttributeID(t_attributeID);
        ImVec2 canvasPos = ImGui::GetCursorScreenPos();
        ImVec2 cursor = ImGui::GetCursorPos();

        bool bezierChanged = false;
        bool blockDragging = false;

        std::vector<glm::vec2> originalPoints = {};
#define BEZIER_PRECISION 64
        for (int i = 0; i < BEZIER_PRECISION + 1; i++) {
            originalPoints.push_back(bezier.Get((float) i / (float) BEZIER_PRECISION));
        }
        std::vector<glm::vec2> transformedPoints;
        for (auto& point : originalPoints) {
            glm::vec4 transformedPoint = project.GetProjectionMatrix(true) * glm::vec4(point, 0, 1);
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
            if (i == screenSpacePoints.size() - 1) break;
            auto& point = screenSpacePoints[i];
            auto& nextPoint = i == screenSpacePoints.size() - 1 ? screenSpacePoints[0] : screenSpacePoints[i + 1];
            ImGui::GetWindowDrawList()->AddLine(canvasPos + ImVec2{point.x, point.y}, canvasPos + ImVec2{nextPoint.x, nextPoint.y}, ImGui::GetColorU32(outlineColor), 3);
            // ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(point.x, point.y), DRAG_CIRCLE_RADIUS, 0xFF);
        }

        originalPoints = bezier.points;
        transformedPoints = {};
        for (auto& point : originalPoints) {
            glm::vec4 transformedPoint = project.GetProjectionMatrix(true) * glm::vec4(point, 0, 1);
            transformedPoints.push_back(
                {transformedPoint.x, transformedPoint.y}
            );
        }

        screenSpacePoints = {};
        for (auto& point : transformedPoints) {
            screenSpacePoints.push_back(NDCToScreen(point, t_regionSize));
        }

        for (int i = 0; i < screenSpacePoints.size(); i++) {
            if (i == screenSpacePoints.size() - 1) break;
            auto& point = screenSpacePoints[i];
            auto& nextPoint = i == screenSpacePoints.size() - 1 ? screenSpacePoints[0] : screenSpacePoints[i + 1];
            AddLineDashed(canvasPos + ImVec2{point.x, point.y}, canvasPos + ImVec2{nextPoint.x, nextPoint.y}, ImGui::GetColorU32(outlineColor), 1.0f, 30, 1, 1);
        }
        auto stringID = attributeCandidate.has_value() ? std::to_string(t_attributeID) : std::to_string(t_attributeID) + s_attributeName;
        BezierCurveOverlayState* overlayState = nullptr;
        if (attributeCandidate.has_value()) {
            overlayState = &s_primaryState;
        } else {
            if (s_attributeStates.find(stringID) == s_attributeStates.end()) {
                s_attributeStates[stringID] = BezierCurveOverlayState();
            }
            overlayState = &s_attributeStates[stringID];
        }

        if (overlayState->drags.size() != bezier.points.size()) {
            overlayState->drags = std::vector<ObjectDrag>(bezier.points.size());
        }

        std::vector<ObjectDrag*> lineDrags = {};
        for (auto& drag : overlayState->drags) {
            lineDrags.push_back(&drag);
        }

        bool beginDrag = true;
        int dragIndex = 0;
        for (auto& dragPtr : lineDrags) {
            auto& drag = *dragPtr;
            auto dragNDC4 = project.GetProjectionMatrix(true) * glm::vec4(bezier.points[dragIndex].x, bezier.points[dragIndex].y, 0, 1);
            auto dragNDC = glm::vec2(dragNDC4.x, dragNDC4.y);
            auto screenDrag = NDCToScreen(dragNDC, t_regionSize);
            RectBounds dragBounds(
                ImVec2{screenDrag.x - DRAG_CIRCLE_RADIUS * 3 / 2.0f, screenDrag.y - DRAG_CIRCLE_RADIUS * 3 / 2.0f},
                ImVec2(DRAG_CIRCLE_RADIUS * 3, DRAG_CIRCLE_RADIUS * 3)
            );
            ImVec4 dragColor = ImVec4(1);
            ImGui::GetWindowDrawList()->AddCircleFilled(canvasPos + ImVec2{screenDrag.x, screenDrag.y}, DRAG_CIRCLE_RADIUS * t_zoom, ImGui::GetColorU32(MouseHoveringBounds(dragBounds) ? dragColor * 0.9f : dragColor));
        
            // DrawRect(dragBounds, ImVec4(1, 0, 0, 1));
            ImGui::PushID(beginDrag);
            if (!overlayState->AnyOtherDragActive(drag.id) && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && MouseHoveringBounds(dragBounds)) {
                ImGui::OpenPopup("##recolorDrag");
            }

            ImGui::PopID();
            if (!overlayState->AnyOtherDragActive(drag.id) && ImGui::IsMouseDragging(ImGuiMouseButton_Left) && (MouseHoveringBounds(dragBounds) || drag.isActive)) {
                bezier.points[dragIndex].x += ImGui::GetIO().MouseDelta.x / t_regionSize.x * 2 * (project.preferredResolution.x / project.preferredResolution.y);
                bezier.points[dragIndex].y -= ImGui::GetIO().MouseDelta.y / t_regionSize.y * 2;
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                bezierChanged = true;
                drag.isActive = true;
            } else drag.isActive = false;
            beginDrag = false;
            dragIndex++;
        }

        {
            auto& drag = overlayState->middleDrag;
            auto middlePos = bezier.Get(0.5);
            auto dragNDC4 = project.GetProjectionMatrix(true) * glm::vec4(middlePos, 0, 1);
            auto dragNDC = glm::vec2(dragNDC4.x, dragNDC4.y);
            auto screenDrag = NDCToScreen(dragNDC, t_regionSize);
            RectBounds dragBounds(
                ImVec2{screenDrag.x - DRAG_CIRCLE_RADIUS * 3 / 2.0f, screenDrag.y - DRAG_CIRCLE_RADIUS * 3 / 2.0f},
                ImVec2(DRAG_CIRCLE_RADIUS * 3, DRAG_CIRCLE_RADIUS * 3)
            );
            ImGui::GetWindowDrawList()->AddCircle(canvasPos + ImVec2{screenDrag.x, screenDrag.y}, DRAG_CIRCLE_RADIUS * t_zoom, ImGui::GetColorU32(MouseHoveringBounds(dragBounds) ? outlineColor * 0.9f : outlineColor));
            ImGui::GetWindowDrawList()->AddCircle(canvasPos + ImVec2{screenDrag.x, screenDrag.y}, DRAG_CIRCLE_RADIUS * t_zoom * 0.5f, ImGui::GetColorU32(MouseHoveringBounds(dragBounds) ? outlineColor * 0.9f : outlineColor));
            // DrawRect(dragBounds, ImVec4(1, 0, 0, 1));

            if (!overlayState->AnyOtherDragActive(drag.id) && ImGui::IsMouseDragging(ImGuiMouseButton_Left) && (MouseHoveringBounds(dragBounds) || drag.isActive)) {
                for (auto& point : bezier.points) {
                    point.x += ImGui::GetIO().MouseDelta.x / t_regionSize.x * 2 * (project.preferredResolution.x / project.preferredResolution.y);
                    point.y -= ImGui::GetIO().MouseDelta.y / t_regionSize.y * 2;
                }
                bezierChanged = true;
                drag.isActive = true;
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            } else drag.isActive = false;
            beginDrag = false; 
        }

        if (t_attributeID > 0) {
            ImGui::SetCursorPos(ImVec2(0, 30));
            if (ImGui::Button(ICON_FA_PLUS)) {
                bezier.points.push_back(bezier.Get(0.5f));
                bezierChanged = true;
            }
            ImGui::SameLine();
            if (ImGui::Button(FormatString("%s", bezier.smoothCurve ? ICON_FA_LINES_LEANING : ICON_FA_BEZIER_CURVE).c_str())) {
                bezier.smoothCurve = !bezier.smoothCurve;
                bezierChanged = true;
            }
            ImGui::SetItemTooltip("%s %s: %s", bezier.smoothCurve ? ICON_FA_LINES_LEANING : ICON_FA_BEZIER_CURVE, Localization::GetString("MODE").c_str(), Localization::GetString(bezier.smoothCurve ? "SMOOTH_CURVE" : "BEZIER_CURVE").c_str());
        }

        if (bezierChanged && attributeCandidate.has_value()) {
            auto& attribute = attributeCandidate.value();
            float compositionRelativeTime = std::floor(project.GetCorrectCurrentTime() - t_composition->GetBeginFrame());
            if (attribute->keyframes.size() == 1) {
                attribute->keyframes[0].value = bezier;
            } else if (attribute->KeyframeExists(compositionRelativeTime) && attribute->keyframes.size() > 1) {
                auto keyframe = attribute->GetKeyframeByTimestamp(compositionRelativeTime).value();
                keyframe->value = bezier;
            } else {
                attribute->keyframes.push_back(AttributeKeyframe(compositionRelativeTime, bezier));
            }
        }

        t_attribute = bezier;
        if (bezierChanged) {
            Rendering::ForceRenderFrame();
        }
        return !bezierChanged; 
    }
};