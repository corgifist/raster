#include "overlay_dispatchers.h"
#include "common/transform2d.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_drag.h"
#include "font/font.h"
#include "common/workspace.h"

#include "../attributes/transform2d_attribute/transform2d_attribute.h"

#define DRAG_CIRCLE_RADIUS 8
#define ANCHOR_CIRCLE_RADIUS 6
#define ANCHOR_LOGIC_RADIUS 32

namespace Raster {

    struct Transform2DOverlayState {
        bool yDragActive;
        bool xDragActive;
        bool positionDragActive;
        bool anchorDragging;
        bool rotatorActive;
        bool ySecondaryDragActive;
        bool xSecondaryDragActive;

        Transform2DOverlayState() {
            this->yDragActive = false;
            this->xDragActive = false;
            this->positionDragActive = false;
            this->anchorDragging = false;
            this->rotatorActive = false;
            this->ySecondaryDragActive = false;
            this->xSecondaryDragActive = false;
        }
    };

    static void DrawRect(RectBounds bounds, ImVec4 color) {
        ImGui::GetWindowDrawList()->AddRectFilled(
            bounds.UL, bounds.BR, ImGui::ColorConvertFloat4ToU32(color));
    }

    static bool MouseHoveringBounds(RectBounds bounds) {
        return ImGui::IsMouseHoveringRect(bounds.UL, bounds.BR);
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

        float angle = int(transform.angle) % 360;

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

        bool& yDragActive = overlayState->yDragActive;
        bool& xDragActive = overlayState->xDragActive;
        bool& positionDragActive = overlayState->positionDragActive;
        bool& anchorDragging = overlayState->anchorDragging;
        bool& rotatorActive = overlayState->rotatorActive;
        bool& ySecondaryDragActive = overlayState->ySecondaryDragActive;
        bool& xSecondaryDragActive = overlayState->xSecondaryDragActive;

        static std::optional<glm::vec2> startPosition;

        ImVec4 outlineColor = ImVec4(1, 1, 1, 1);
        outlineColor.w = 1.0f;
        for (int i = 0; i < screenSpacePoints.size(); i++) {
            auto& point = screenSpacePoints[i];
            auto& nextPoint = i == screenSpacePoints.size() - 1 ? screenSpacePoints[0] : screenSpacePoints[i + 1];
            ImGui::GetWindowDrawList()->AddLine(canvasPos + ImVec2{point.x, point.y}, canvasPos + ImVec2{nextPoint.x, nextPoint.y}, ImGui::GetColorU32(outlineColor), 2);
        }

        auto resizeXDragNDC4 = project.GetProjectionMatrix(true) * transform.GetTransformationMatrix() * glm::vec4(1, 0, 0, 1);
        auto resizeXDragNDC = glm::vec2(resizeXDragNDC4.x, resizeXDragNDC4.y);
        auto screenXDrag = NDCToScreen(resizeXDragNDC, t_regionSize);
        ImGui::GetWindowDrawList()->AddCircleFilled(canvasPos + ImVec2{screenXDrag.x, screenXDrag.y}, DRAG_CIRCLE_RADIUS * t_zoom, ImGui::GetColorU32(outlineColor));

        auto resizeYDragNDC4 = project.GetProjectionMatrix(true) * transform.GetTransformationMatrix() * glm::vec4(0, 1, 0, 1);
        auto resizeYDragNDC = glm::vec2(resizeYDragNDC4.x, resizeYDragNDC4.y);
        auto screenYDrag = NDCToScreen(resizeYDragNDC, t_regionSize);
        ImGui::GetWindowDrawList()->AddCircleFilled(canvasPos + ImVec2{screenYDrag.x, screenYDrag.y}, DRAG_CIRCLE_RADIUS * t_zoom, ImGui::GetColorU32(outlineColor));

        auto resizeYSecondaryDragNDC4 = project.GetProjectionMatrix(true) * transform.GetTransformationMatrix() * glm::vec4(0, -1, 0, 1);
        auto resizeYSecondaryDragNDC = glm::vec2(resizeYSecondaryDragNDC4.x, resizeYSecondaryDragNDC4.y);
        auto screenYSecondaryDrag = NDCToScreen(resizeYSecondaryDragNDC, t_regionSize);
        ImGui::GetWindowDrawList()->AddCircleFilled(canvasPos + ImVec2{screenYSecondaryDrag.x, screenYSecondaryDrag.y}, DRAG_CIRCLE_RADIUS * t_zoom, ImGui::GetColorU32(outlineColor));
        
        auto resizeXSecondaryDragNDC4 = project.GetProjectionMatrix(true) * transform.GetTransformationMatrix() * glm::vec4(-1, 0, 0, 1);
        auto resizeXSecondaryDragNDC = glm::vec2(resizeXSecondaryDragNDC4.x, resizeXSecondaryDragNDC4.y);
        auto screenXSecondaryDrag = NDCToScreen(resizeXSecondaryDragNDC, t_regionSize);
        ImGui::GetWindowDrawList()->AddCircleFilled(canvasPos + ImVec2{screenXSecondaryDrag.x, screenXSecondaryDrag.y}, DRAG_CIRCLE_RADIUS * t_zoom, ImGui::GetColorU32(outlineColor));
        
        glm::mat4 anchorTransformMatrix = glm::identity<glm::mat4>();
        anchorTransformMatrix = glm::translate(anchorTransformMatrix, glm::vec3(transform.position, 0));
        anchorTransformMatrix = glm::translate(anchorTransformMatrix, glm::vec3(transform.anchor, 0));
        anchorTransformMatrix = transform.GetParentMatrix() * anchorTransformMatrix;

        glm::vec4 anchorPointNDC4 = project.GetProjectionMatrix(true) * anchorTransformMatrix * glm::vec4(0, 0, 0, 1);
        glm::vec2 anchorPointScreen = glm::vec2(anchorPointNDC4.x, anchorPointNDC4.y);
        anchorPointScreen = NDCToScreen(anchorPointScreen, t_regionSize);
        
        ImGui::GetWindowDrawList()->AddCircleFilled(canvasPos + ImVec2{anchorPointScreen.x, anchorPointScreen.y}, ANCHOR_CIRCLE_RADIUS * t_zoom, ImGui::GetColorU32(outlineColor));
        
        if (attributeCandidate.has_value()) {
            auto& attribute = attributeCandidate.value();
            if (attribute->packageName == RASTER_PACKAGED "transform2d_attribute") {
                auto transform2dAttribute = (Transform2DAttribute*) attribute.get();
                if (transform2dAttribute->m_parentAssetID > 0 && transform.parentTransform) {
                    transform = *transform.parentTransform;
                }
            }
        }

        RectBounds xCircleBounds(
            ImVec2{screenXDrag.x - DRAG_CIRCLE_RADIUS, screenXDrag.y - DRAG_CIRCLE_RADIUS},
            ImVec2(DRAG_CIRCLE_RADIUS * 2, DRAG_CIRCLE_RADIUS * 2)
        );

        glm::vec2 beginAxis;
        glm::vec2 endAxis;
        if (IsInBounds((int) angle, 0, 90)) {
            beginAxis = glm::vec2(1, 0);
            endAxis = glm::vec2(0, -1);
        } else if (IsInBounds((int) angle, 90, 180)) {
            beginAxis = glm::vec2(0, -1);
            endAxis = glm::vec2(-1, 0);
        } else if (IsInBounds((int) angle, 180, 270)) {
            beginAxis = glm::vec2(-1, 0);
            endAxis = glm::vec2(0, 1);
        } else {
            beginAxis = glm::vec2(0, 1);
            endAxis = glm::vec2(1, 0);
        }
        glm::vec2 rotateAxises = glm::mix(beginAxis, endAxis, float((int(angle) % 91) / 90.0f));

        if (!project.customData.contains("Transform2DAttributeData")) {
            project.customData["Transform2DAttributeData"] = {};
        }
        auto& customData = project.customData["Transform2DAttributeData"];
        if (!customData.contains(stringID)) {
            customData[stringID] = false;
        }
        bool linkedSize = customData[stringID];

        static bool positionDragAlreadyActive = false;

        if ((MouseHoveringBounds(xCircleBounds) || xDragActive)  && ImGui::IsWindowFocused() && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !yDragActive && !positionDragActive && !anchorDragging && !rotatorActive && !ySecondaryDragActive && !xSecondaryDragActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            transform.size.x += ImGui::GetIO().MouseDelta.x / t_regionSize.x * rotateAxises.x;
            transform.size.x += ImGui::GetIO().MouseDelta.y / t_regionSize.y * rotateAxises.y;
            if (linkedSize) {
                transform.size.y = transform.size.x;
            }
            transformChanged = true;
            xDragActive = true;
            positionDragAlreadyActive = true;
        } else {
            xDragActive = false;
        }
        if (MouseHoveringBounds(xCircleBounds)) {
            ImGui::SetTooltip(ICON_FA_LEFT_RIGHT " Increase/Decrease Width");
        }

        RectBounds yCircleBounds(
            ImVec2{screenYDrag.x - DRAG_CIRCLE_RADIUS, screenYDrag.y - DRAG_CIRCLE_RADIUS},
            ImVec2(DRAG_CIRCLE_RADIUS * 2, DRAG_CIRCLE_RADIUS * 2)
        );

        if (IsInBounds((int) angle, 0, 90)) {
            beginAxis = glm::vec2(0, -1);
            endAxis = glm::vec2(-1, 0);
        } else if (IsInBounds((int) angle, 90, 180)) {
            beginAxis = glm::vec2(-1, 0);
            endAxis = glm::vec2(0, 1);
        } else if (IsInBounds((int) angle, 180, 270)) {
            beginAxis = glm::vec2(0, 1);
            endAxis = glm::vec2(1, 0);
        } else {
            beginAxis = glm::vec2(1, 0);
            endAxis = glm::vec2(0, -1);
        }
        rotateAxises = glm::mix(beginAxis, endAxis, float((int(angle) % 91) / 90.0f));

        if ((MouseHoveringBounds(yCircleBounds) || yDragActive)  && ImGui::IsWindowFocused() && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !xDragActive && !anchorDragging && !positionDragActive && !rotatorActive && !ySecondaryDragActive && !xSecondaryDragActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            transform.size.y += ImGui::GetIO().MouseDelta.x / t_regionSize.x * rotateAxises.x;
            transform.size.y += ImGui::GetIO().MouseDelta.y / t_regionSize.y * rotateAxises.y;
            if (linkedSize) {
                transform.size.x = transform.size.y;
            }
            transformChanged = true;
            yDragActive = true;
            positionDragAlreadyActive = true;
            ImGui::SetTooltip("%s Increase/Decrease Height", ICON_FA_UP_DOWN);
        } else {
            yDragActive = false;
        }

        RectBounds ySecondaryCircleBounds(
            ImVec2{screenYSecondaryDrag.x - DRAG_CIRCLE_RADIUS, screenYSecondaryDrag.y - DRAG_CIRCLE_RADIUS},
            ImVec2(DRAG_CIRCLE_RADIUS * 2, DRAG_CIRCLE_RADIUS * 2)
        );

        if (IsInBounds((int) angle, 0, 90)) {
            beginAxis = glm::vec2(0, 1);
            endAxis = glm::vec2(1, 0);
        } else if (IsInBounds((int) angle, 90, 180)) {
            beginAxis = glm::vec2(1, 0);
            endAxis = glm::vec2(0, -1);
        } else if (IsInBounds((int) angle, 180, 270)) {
            beginAxis = glm::vec2(0, -1);
            endAxis = glm::vec2(-1, 0);
        } else {
            beginAxis = glm::vec2(-1, 0);
            endAxis = glm::vec2(0, 1);
        }
        rotateAxises = glm::mix(beginAxis, endAxis, float((int(angle) % 91) / 90.0f));

        if ((MouseHoveringBounds(ySecondaryCircleBounds) || ySecondaryDragActive)  && ImGui::IsWindowFocused() && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !xDragActive && !yDragActive && !anchorDragging && !positionDragActive && !rotatorActive && !xSecondaryDragActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            transform.size.y += ImGui::GetIO().MouseDelta.x / t_regionSize.x * rotateAxises.x;
            transform.size.y += ImGui::GetIO().MouseDelta.y / t_regionSize.y * rotateAxises.y;
            if (linkedSize) {
                transform.size.x = transform.size.y;
            }
            transformChanged = true;
            ySecondaryDragActive = true;
            positionDragAlreadyActive = true;
            ImGui::SetTooltip("%s Increase/Decrease Height", ICON_FA_UP_DOWN);
        } else {
            ySecondaryDragActive = false;
        }

        RectBounds xSecondaryCircleBounds(
            ImVec2{screenXSecondaryDrag.x - DRAG_CIRCLE_RADIUS, screenXSecondaryDrag.y - DRAG_CIRCLE_RADIUS},
            ImVec2(DRAG_CIRCLE_RADIUS * 2, DRAG_CIRCLE_RADIUS * 2)
        );

        if (IsInBounds((int) angle, 0, 90)) {
            beginAxis = glm::vec2(-1, 0);
            endAxis = glm::vec2(0, 1);
        } else if (IsInBounds((int) angle, 90, 180)) {
            beginAxis = glm::vec2(0, 1);
            endAxis = glm::vec2(1, 0);
        } else if (IsInBounds((int) angle, 180, 270)) {
            beginAxis = glm::vec2(1, -0);
            endAxis = glm::vec2(0, -1);
        } else {
            beginAxis = glm::vec2(0, -1);
            endAxis = glm::vec2(-1, 0);
        }
        rotateAxises = glm::mix(beginAxis, endAxis, float((int(angle) % 91) / 90.0f));

        if ((MouseHoveringBounds(xSecondaryCircleBounds) || xSecondaryDragActive)  && ImGui::IsWindowFocused() && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !xDragActive && !yDragActive && !anchorDragging && !positionDragActive && !rotatorActive && !ySecondaryDragActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            transform.size.x += ImGui::GetIO().MouseDelta.x / t_regionSize.x * rotateAxises.x;
            transform.size.x += ImGui::GetIO().MouseDelta.y / t_regionSize.y * rotateAxises.y;
            if (linkedSize) {
                transform.size.y = transform.size.x;
            }
            transformChanged = true;
            xSecondaryDragActive = true;
            positionDragAlreadyActive = true;
            ImGui::SetTooltip("%s Increase/Decrease Width", ICON_FA_LEFT_RIGHT);
        } else {
            xSecondaryDragActive = false;
        }

        RectBounds anchorPointBounds(
            ImVec2{anchorPointScreen.x, anchorPointScreen.y} - ImVec2(ANCHOR_LOGIC_RADIUS * t_zoom, ANCHOR_LOGIC_RADIUS * t_zoom),
            ImVec2(ANCHOR_LOGIC_RADIUS * t_zoom, ANCHOR_LOGIC_RADIUS * t_zoom)
        );
        if ((MouseHoveringBounds(anchorPointBounds) || anchorDragging)  && ImGui::IsWindowFocused() && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !xDragActive && !yDragActive && !positionDragActive && !rotatorActive && !ySecondaryDragActive && !xSecondaryDragActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            ImGui::SetTooltip("%s Move Anchor Point", ICON_FA_ANCHOR);
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            transform.anchor.x += ImGui::GetIO().MouseDelta.x / t_regionSize.x;
            transform.anchor.y -= ImGui::GetIO().MouseDelta.y / t_regionSize.y;
            transformChanged = true;
            anchorDragging = true;
            positionDragAlreadyActive = true;
        } else {
            anchorDragging = false;
        }

        glm::vec4 centerPointNDC4(0, 0, 0, 1);
        centerPointNDC4 = project.GetProjectionMatrix(true) * transform.GetTransformationMatrix() * centerPointNDC4;
        glm::vec2 centerPointScreen(centerPointNDC4.x, centerPointNDC4.y);
        centerPointScreen = NDCToScreen(centerPointScreen, t_regionSize);
        glm::vec2 transformScreenSize(transform.size.x * t_regionSize.x, transform.size.y * t_regionSize.y);

        RectBounds positionDragBounds(
            {centerPointScreen.x - transformScreenSize.x / 2.0f, centerPointScreen.y - transformScreenSize.y / 2.0f},
            {transformScreenSize.x, transformScreenSize.y}
        );
        ImGui::ItemAdd(ImRect(positionDragBounds.UL, positionDragBounds.BR), ImGui::GetID("##transformContainer"));
        if ((MouseHoveringBounds(positionDragBounds) || positionDragActive) && ImGui::IsWindowFocused() && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGui::GetIO().MouseClicked[ImGuiMouseButton_Left] && !anchorDragging && !xDragActive && !yDragActive && !rotatorActive && !ySecondaryDragActive && !xSecondaryDragActive && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            transform.position.x += ImGui::GetIO().MouseDelta.x / (t_regionSize.x / t_zoom);
            transform.position.y += -ImGui::GetIO().MouseDelta.y / (t_regionSize.y / t_zoom);
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            ImGui::SetTooltip("%s Move Position", ICON_FA_UP_DOWN_LEFT_RIGHT);

            transformChanged = true;
            positionDragActive = true;
            positionDragAlreadyActive = true;
        } else {
            positionDragActive = false;
        }

        static std::optional<float> startAngle;
        if (ImGui::IsKeyDown(ImGuiKey_R) && ImGui::IsWindowFocused()) {
            if (!startAngle.has_value()) startAngle = transform.angle;
            if (ImGui::BeginTooltip()) {
                ImGui::Text("%s Rotation", ICON_FA_ROTATE);
                ImGui::Text("%s %0.1f -> %0.1f", ICON_FA_CIRCLE_INFO, startAngle.value(), transform.angle);
                ImGui::EndTooltip();
            }
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            transform.angle += ImGui::GetIO().MouseDelta.x * 0.3f;
            if (IsInBounds(transform.angle, -2.0f, 2.0f)) transform.angle = 0;
            transformChanged = true;
        } else startAngle = std::nullopt;

        if (ImGui::IsKeyDown(ImGuiKey_P) && ImGui::IsWindowFocused()) {
            if (!startPosition.has_value()) startPosition = transform.position;
            if (ImGui::BeginTooltip()) {
                auto& position = startPosition.value();
                ImGui::Text("%s Position", ICON_FA_UP_DOWN_LEFT_RIGHT);
                ImGui::Text("%s (%0.1f; %0.1f) -> (%0.1f; %0.1f)", ICON_FA_CIRCLE_INFO, position.x, position.y, transform.position.x, transform.position.y);
                ImGui::EndTooltip();
            }
            ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
            transform.position += glm::vec2(
                ImGui::GetIO().MouseDelta.x / t_regionSize.x,
                -ImGui::GetIO().MouseDelta.y / t_regionSize.y
            );
            transformChanged = true;
        } else startPosition = std::nullopt;

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
            ImVec2 reservedCursor = ImGui::GetCursorPos();
            ImGui::SetCursorPos({0, 30});
            ImGui::SetWindowFontScale(0.8f);
            ImGui::Text("%s %s | %s Transform2D", ICON_FA_LINK, attribute->name.c_str(), ICON_FA_UP_DOWN_LEFT_RIGHT);
            if (linkedSize) ImGui::Text("%s Linked Size", ICON_FA_TRIANGLE_EXCLAMATION);
            ImGui::Text("%s Position: %0.2f; %0.2f", ICON_FA_UP_DOWN_LEFT_RIGHT, transform.position.x, transform.position.y);
            ImGui::Text("%s Size: %0.2f; %0.2f", ICON_FA_SCALE_BALANCED, transform.size.x, transform.size.y);
            ImGui::Text("%s Anchor: %0.2f; %0.2f", ICON_FA_ANCHOR, transform.anchor.x, transform.anchor.y);
            ImGui::Text("%s Angle: %0.2f", ICON_FA_ROTATE, transform.angle);
            ImGui::SetWindowFontScale(1.0f);
            ImGui::SetCursorPos(reservedCursor);
        }

        if (!project.selectedAttributes.empty() && t_attributeID == project.selectedAttributes.back()) {
            for (int i = project.compositions.size(); i --> 0;) {
                auto& composition = project.compositions[i];
                bool exitLoop = false;
                for (auto& attribute : composition.attributes) {
                    if (attribute->packageName != RASTER_PACKAGED "transform2d_attribute") continue;
                    auto value = attribute->Get(project.currentFrame - composition.beginFrame, &composition);
                    auto hitTransform = std::any_cast<Transform2D>(value);
                    glm::vec4 centerPointNDC4(0, 0, 0, 1);
                    centerPointNDC4 = project.GetProjectionMatrix(true) * hitTransform.GetTransformationMatrix() * centerPointNDC4;
                    glm::vec2 centerPointScreen(centerPointNDC4.x, centerPointNDC4.y);
                    centerPointScreen = NDCToScreen(centerPointScreen, t_regionSize);
                    glm::vec2 transformScreenSize(hitTransform.size.x * t_regionSize.x, hitTransform.size.y * t_regionSize.y);

                    RectBounds hitBounds(
                        {centerPointScreen.x - transformScreenSize.x / 2.0f, centerPointScreen.y - transformScreenSize.y / 2.0f},
                        {transformScreenSize.x, transformScreenSize.y}
                    );
                    if (MouseHoveringBounds(hitBounds) && ImGui::GetIO().MouseClicked[ImGuiMouseButton_Left] && ImGui::IsWindowFocused() && !positionDragAlreadyActive && ImGui::GetIO().MouseDelta == ImVec2(0, 0)) {
                        if (!ImGui::GetIO().KeyCtrl) {
                            project.selectedAttributes = {attribute->id};
                        } else {
                            auto& selectedAttributes = project.selectedAttributes;
                            auto attributeIterator = std::find(selectedAttributes.begin(), selectedAttributes.end(), attribute->id);
                            if (attributeIterator == selectedAttributes.end()) {
                                selectedAttributes.push_back(attribute->id);
                            } else {
                                selectedAttributes.erase(attributeIterator);
                            }
                        }
                        exitLoop = true;
                        break;
                    }
                }
                if (exitLoop) break;
            }
            positionDragAlreadyActive = false;
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
        return !transformChanged || !positionDragAlreadyActive; 
    }
};