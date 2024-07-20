#include "common/overlay_dispatchers.h"
#include "common/transform2d.h"
#include "../ImGui/imgui.h"
#include "../ImGui/imgui_drag.h"

namespace Raster {

    static void DrawRect(RectBounds bounds, ImVec4 color) {
        ImGui::GetWindowDrawList()->AddRectFilled(
            bounds.UL, bounds.BR, ImGui::ColorConvertFloat4ToU32(color));
    }

    static void PushClipRect(RectBounds bounds) {
        ImGui::GetWindowDrawList()->PushClipRect(bounds.UL, bounds.BR, true);
    }

    static void PopClipRect() { ImGui::GetWindowDrawList()->PopClipRect(); }

    static bool MouseHoveringBounds(RectBounds bounds) {
        return ImGui::IsMouseHoveringRect(bounds.UL, bounds.BR);
    }


    bool OverlayDispatchers::DispatchTransform2DValue(std::any& t_attribute, Composition* t_composition, int t_attributeID, float t_zoom, glm::vec2 t_regionSize) {
        Transform2D transform = std::any_cast<Transform2D>(t_attribute);
        auto& project = Workspace::s_project.value();
        auto attribute = Workspace::GetAttributeByAttributeID(t_attributeID).value();
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
            glm::vec4 transformedPoint = project.GetProjectionMatrix() * transform.GetTransformationMatrix() * glm::vec4(point, 0, 1);
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
            ImGui::GetWindowDrawList()->AddLine(canvasPos + ImVec2{point.x, point.y}, canvasPos + ImVec2{nextPoint.x, nextPoint.y}, ImGui::GetColorU32(outlineColor), 4);
        }

        auto resizeXDragNDC4 = project.GetProjectionMatrix() * transform.GetTransformationMatrix() * glm::vec4(1, 0, 0, 1);
        auto resizeXDragNDC = glm::vec2(resizeXDragNDC4.x, resizeXDragNDC4.y);
        auto screenXDrag = NDCToScreen(resizeXDragNDC, t_regionSize);
        ImGui::GetWindowDrawList()->AddCircleFilled(canvasPos + ImVec2{screenXDrag.x, screenXDrag.y}, 10.0f * t_zoom, ImGui::GetColorU32(outlineColor));
        RectBounds xCircleBounds(
            ImVec2{screenXDrag.x - 10, screenXDrag.y - 10},
            ImVec2(20, 20)
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
        static bool xDragActive = false;

        if (!project.customData.contains("Transform2DAttributeData")) {
            project.customData["Transform2DAttributeData"] = {};
        }
        auto& customData = project.customData["Transform2DAttributeData"];
        auto stringID = std::to_string(t_attributeID);
        if (!customData.contains(stringID)) {
            customData[stringID] = false;
        }
        bool linkedSize = customData[stringID];

        if ((MouseHoveringBounds(xCircleBounds) || xDragActive) && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            transform.size.x += ImGui::GetIO().MouseDelta.x / t_regionSize.x * rotateAxises.x;
            transform.size.x += ImGui::GetIO().MouseDelta.y / t_regionSize.y * rotateAxises.y;
            if (linkedSize) {
                transform.size.y = transform.size.x;
            }
            transformChanged = true;
            xDragActive = true;
        } else {
            xDragActive = false;
        }
        if (MouseHoveringBounds(xCircleBounds)) {
            ImGui::SetTooltip(ICON_FA_LEFT_RIGHT " Increase/Decrease Width");
        }

        auto resizeYDragNDC4 = project.GetProjectionMatrix() * transform.GetTransformationMatrix() * glm::vec4(0, 1, 0, 1);
        auto resizeYDragNDC = glm::vec2(resizeYDragNDC4.x, resizeYDragNDC4.y);
        auto screenYDrag = NDCToScreen(resizeYDragNDC, t_regionSize);
        ImGui::GetWindowDrawList()->AddCircleFilled(canvasPos + ImVec2{screenYDrag.x, screenYDrag.y}, 10.0f * t_zoom, ImGui::GetColorU32(outlineColor));
        RectBounds yCircleBounds(
            ImVec2{screenYDrag.x - 10, screenYDrag.y - 10},
            ImVec2(20, 20)
        );

        static bool yDragActive = false;

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

        if ((MouseHoveringBounds(yCircleBounds) || yDragActive) && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            transform.size.y += ImGui::GetIO().MouseDelta.x / t_regionSize.x * rotateAxises.x;
            transform.size.y += ImGui::GetIO().MouseDelta.y / t_regionSize.y * rotateAxises.y;
            if (linkedSize) {
                transform.size.x = transform.size.y;
            }
            transformChanged = true;
            yDragActive = true;
            ImGui::SetTooltip("%s Increase/Decrease Height", ICON_FA_UP_DOWN);
        } else {
            yDragActive = false;
        }

        
        glm::vec4 anchorPointNDC4 = project.GetProjectionMatrix() * glm::translate(glm::identity<glm::mat4>(), glm::vec3(transform.anchor, 0)) * glm::vec4(0, 0, 0, 1);
        glm::vec2 anchorPointScreen = glm::vec2(anchorPointNDC4.x, anchorPointNDC4.y);
        anchorPointScreen = NDCToScreen(anchorPointScreen, t_regionSize);
        
        ImGui::GetWindowDrawList()->AddCircleFilled(canvasPos + ImVec2{anchorPointScreen.x, anchorPointScreen.y} - ImVec2(4.0f * t_zoom, 4.0f * t_zoom), 8 * t_zoom, ImGui::GetColorU32(outlineColor));
        RectBounds anchorPointBounds(
            ImVec2{anchorPointScreen.x, anchorPointScreen.y} - ImVec2(32 * t_zoom, 32 * t_zoom),
            ImVec2(32 * t_zoom, 32 * t_zoom)
        );
        static bool anchorDragging = false;
        if ((MouseHoveringBounds(anchorPointBounds) || anchorDragging) && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            ImGui::SetTooltip("%s Move Anchor Point", ICON_FA_ANCHOR);
            transform.anchor.x += ImGui::GetIO().MouseDelta.x / t_regionSize.x;
            transform.anchor.y -= ImGui::GetIO().MouseDelta.y / t_regionSize.y;
            transformChanged = true;
            anchorDragging = true;
        } else {
            anchorDragging = false;
        }


        if (transformChanged) {
            float compositionRelativeTime = std::floor(project.currentFrame - t_composition->beginFrame);
            if (attribute->KeyframeExists(compositionRelativeTime)) {
                auto keyframe = attribute->GetKeyframeByTimestamp(compositionRelativeTime).value();
                keyframe->value = transform;
            } else {
                attribute->keyframes.push_back(AttributeKeyframe(compositionRelativeTime, transform));
            }
        }

        ImGui::SetCursorPos({0, 20});
        ImGui::SetWindowFontScale(0.8f);
        ImGui::Text("%s %s | %s Transform2D", ICON_FA_LINK, attribute->name.c_str(), ICON_FA_UP_DOWN_LEFT_RIGHT);
        if (linkedSize) ImGui::Text("%s Linked Size", ICON_FA_TRIANGLE_EXCLAMATION);
        ImGui::Text("%s Position: %0.2f; %0.2f", ICON_FA_UP_DOWN_LEFT_RIGHT, transform.position.x, transform.position.y);
        ImGui::Text("%s Size: %0.2f; %0.2f", ICON_FA_SCALE_BALANCED, transform.size.x, transform.size.y);
        ImGui::Text("%s Anchor: %0.2f; %0.2f", ICON_FA_ANCHOR, transform.anchor.x, transform.anchor.y);
        ImGui::Text("%s Angle: %0.2f", ICON_FA_ROTATE, transform.angle);
        ImGui::SetWindowFontScale(1.0f);

        return !transformChanged; 
    }
};