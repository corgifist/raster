#include "timeline.h"
#include "font/font.h"

#define SPLITTER_RULLER_WIDTH 8
#define TIMELINE_RULER_WIDTH 4
#define TICKS_BAR_HEIGHT 30
#define TICK_SMALL_WIDTH 3
#define LAYER_HEIGHT 44

#define ROUND_EVEN(x) std::round( (x) * 0.5f ) * 2.0f

namespace Raster {

    static float s_splitterState = 0.3f;
    static DragStructure s_dragStructure;

    static std::vector<float> s_layerSeparators;

    static float s_pixelsPerFrame = 4;

    static bool s_timelineRulerDragged = false;
    static bool s_anyLayerDragged = false;
    static bool s_scrollbarActive = true;

    static DragStructure s_timelineDrag;

    static float precision(float f, int places) {
        float n = std::pow(10.0f, places ) ;
        return std::round(f * n) / n ;
    }

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

    void TimelineUI::Render() {
        PushStyleVars();
        ImGui::Begin(FormatString("%s %s", ICON_FA_TIMELINE, Localization::GetString("TIMELINE").c_str()).c_str());
            if (ImGui::IsKeyPressed(ImGuiKey_KeypadAdd)  && ImGui::GetIO().KeyCtrl) {
                s_pixelsPerFrame += 1;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract) && ImGui::GetIO().KeyCtrl) {
                s_pixelsPerFrame -= 1;
            }
            s_pixelsPerFrame = std::clamp((int) s_pixelsPerFrame, 1, 10);

            RenderLegend();
            RenderCompositionsEditor();
            RenderSplitter();
        ImGui::End();
        PopStyleVars();
    }

    void TimelineUI::PushStyleVars() {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
    }


    void TimelineUI::PopStyleVars() {
        ImGui::PopStyleVar(2);
    }

    void TimelineUI::RenderTicksBar() {
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        RectBounds backgroundBounds = RectBounds(
            ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY()),
            ImVec2(ImGui::GetWindowSize().x, TICKS_BAR_HEIGHT)
        );

        DrawRect(backgroundBounds, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
        if (MouseHoveringBounds(backgroundBounds) && !s_anyLayerDragged) {
            s_timelineDrag.Activate();
        }
    }

    void TimelineUI::RenderTicks() {
        RenderTicksBar();

        RectBounds backgroundBounds = RectBounds(
            ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY()),
            ImVec2(ImGui::GetWindowSize().x, TICKS_BAR_HEIGHT)
        );

        auto& project = Workspace::s_project.value();

        int desiredTicksCount = ROUND_EVEN(5 * s_pixelsPerFrame / 2);
        int tickStep =
            project.framerate / desiredTicksCount;
        int tickPositionStep = tickStep * s_pixelsPerFrame;
        int tickPositionAccumulator = 0;
        int previousTickPositionAccumulator = 0;
        int tickAccumulator = 0;
        int ticksMajorAccumulator = 0;
        while (tickAccumulator <= project.GetProjectLength()) {
            bool majorTick = remainder((int) tickAccumulator,
                                    (int) project.framerate) == 0.0f;
            bool mediumTick = remainder(tickAccumulator, project.framerate / 2) == 0;
            float tickHeight = majorTick ? ImGui::GetWindowSize().y : TICKS_BAR_HEIGHT / 4;
            if (!majorTick && mediumTick) {
                tickHeight = TICKS_BAR_HEIGHT / 2;
            }
            RectBounds tickBounds = RectBounds(
                ImVec2(tickPositionAccumulator, 0 + ImGui::GetScrollY()),
                ImVec2(TICK_SMALL_WIDTH, tickHeight));
            
            std::string timestampFormattedText = project.FormatFrameToTime(tickAccumulator);
            ImVec2 timestampSize = ImGui::CalcTextSize(timestampFormattedText.c_str());

            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->ChannelsSetCurrent(0);
            DrawRect(tickBounds, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));

            ticksMajorAccumulator++;

            if (majorTick) {
                drawList->ChannelsSetCurrent(1);
                drawList->AddText(
                    ImGui::GetCursorScreenPos() + ImVec2(tickBounds.pos.x + 5, 6.0f),
                    IM_COL32(255, 255, 255, 255), timestampFormattedText.c_str()
                );
                previousTickPositionAccumulator = tickPositionAccumulator;
                ticksMajorAccumulator = 0;
            }
            drawList->ChannelsSetCurrent(0);

            tickPositionAccumulator += tickPositionStep;
            tickAccumulator += tickStep;
        }
    }

    void TimelineUI::RenderCompositionsEditor() {
        ImGui::SameLine();

        auto& project = Workspace::s_project.value();

        RectBounds backgroundBounds = RectBounds(
            ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY()),
            ImVec2(ImGui::GetWindowSize().x, TICKS_BAR_HEIGHT)
        );

        ImVec2 windowSize = ImGui::GetWindowSize();
        static bool showHorizontalScrollbar = false;
        static bool showVerticalScrollbar = false;
        ImGuiWindowFlags timelineFlags = 0;
        if (showHorizontalScrollbar)
            timelineFlags |= ImGuiWindowFlags_AlwaysHorizontalScrollbar;
        if (showVerticalScrollbar)
            timelineFlags |= ImGuiWindowFlags_AlwaysVerticalScrollbar;
        ImGui::BeginChild("##timelineCompositions", ImVec2(ImGui::GetWindowSize().x * (1 - s_splitterState), ImGui::GetContentRegionAvail().y), 0, timelineFlags);
            ImVec2 windowMouseCoords = GetRelativeMousePos();
            bool previousVerticalBar = showVerticalScrollbar;
            bool previousHorizontalBar = showHorizontalScrollbar;
            showVerticalScrollbar = (windowMouseCoords.x - ImGui::GetScrollX() >= (ImGui::GetWindowSize().x - 20)) && windowMouseCoords.x - ImGui::GetScrollY() < ImGui::GetWindowSize().x + 5;
            showHorizontalScrollbar = (windowMouseCoords.y - ImGui::GetScrollY() >= (ImGui::GetWindowSize().y - 20)) && windowMouseCoords.y - ImGui::GetScrollY() < ImGui::GetWindowSize().y + 5;
            if (previousVerticalBar && ImGui::GetIO().MouseDown[ImGuiMouseButton_Left])
                showVerticalScrollbar = true;
            if (previousHorizontalBar && ImGui::GetIO().MouseDown[ImGuiMouseButton_Left])
                showHorizontalScrollbar = true;

            ImDrawList* drawList = ImGui::GetWindowDrawList();
            drawList->ChannelsSplit(2);
            RenderTicks();

            ImGui::SetCursorPosY(backgroundBounds.size.y);
            for (auto& composition : project.compositions) {
                RenderComposition(composition.id);
            }

            RenderTimelineRuler();
        ImGui::EndChild();
    }

    void TimelineUI::RenderComposition(int t_id) {
        auto compositionCandidate = Workspace::GetCompositionByID(t_id);
        if (compositionCandidate.has_value()) {
            auto& composition = compositionCandidate.value();
            ImGui::SetCursorPosX(composition->beginFrame * s_pixelsPerFrame);
            ImVec4 buttonColor = ImGui::ColorConvertU32ToFloat4(ImGui::GetColorU32(ImGuiCol_Button));
            buttonColor.w = 1.0f;
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
            ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
            ImVec2 buttonCursor = ImGui::GetCursorPos();
            ImVec2 buttonSize = ImVec2((composition->endFrame - composition->beginFrame) * s_pixelsPerFrame, LAYER_HEIGHT);
            bool compositionPressed = ImGui::Button(FormatString("%s %s", ICON_FA_LAYER_GROUP, composition->name.c_str()).c_str(), buttonSize);
            bool compositionHovered = ImGui::IsItemHovered();
            ImGui::PopStyleVar();
            ImGui::PopStyleColor();
            auto& io = ImGui::GetIO();
            if (compositionPressed && !io.KeyCtrl) {
                Workspace::s_selectedCompositions = {composition->id};
            }
            static DragStructure s_layerDrag;
            static DragStructure s_forwardBoundsDrag, s_backwardBoundsDrag;
            s_anyLayerDragged = s_layerDrag.isActive || s_backwardBoundsDrag.isActive || s_forwardBoundsDrag.isActive;

            ImVec2 dragSize = ImVec2((composition->endFrame - composition->beginFrame) * s_pixelsPerFrame / 10, LAYER_HEIGHT);
            dragSize.x = std::clamp(dragSize.x, 1.0f, 30.0f);

            ImGui::SetCursorPos({0, 0});
            RectBounds forwardBoundsDrag(
                ImVec2(buttonCursor.x + buttonSize.x - dragSize.x, buttonCursor.y),
                dragSize
            );

            RectBounds backwardBoundsDrag(
                buttonCursor, dragSize
            );

            ImVec4 dragColor = buttonColor * 0.7f;
            dragColor.w = 1.0f;
            DrawRect(forwardBoundsDrag, dragColor);
            DrawRect(backwardBoundsDrag, dragColor);

            if ((MouseHoveringBounds(forwardBoundsDrag) || s_forwardBoundsDrag.isActive) && !s_timelineRulerDragged && !s_layerDrag.isActive && !s_backwardBoundsDrag.isActive) {
                s_forwardBoundsDrag.Activate();
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

                float boundsDragDistance;
                if (s_forwardBoundsDrag.GetDragDistance(boundsDragDistance)) {
                    composition->endFrame += boundsDragDistance / s_pixelsPerFrame;
                } else s_forwardBoundsDrag.Deactivate();

                float scrollAmount = ProcessLayerScroll();
                composition->endFrame += scrollAmount / s_pixelsPerFrame;
            }

            if ((MouseHoveringBounds(backwardBoundsDrag) || s_backwardBoundsDrag.isActive) && !s_timelineRulerDragged && !s_layerDrag.isActive) {
                s_backwardBoundsDrag.Activate();
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

                float boundsDragDistance;
                if (s_backwardBoundsDrag.GetDragDistance(boundsDragDistance)) {
                    composition->beginFrame += boundsDragDistance / s_pixelsPerFrame;
                } else s_backwardBoundsDrag.Deactivate();

                float scrollAmount = ProcessLayerScroll();
                composition->beginFrame += scrollAmount / s_pixelsPerFrame;
            }
 
            if ((ImGui::IsItemHovered() || s_layerDrag.isActive) && !s_timelineRulerDragged && !s_backwardBoundsDrag.isActive && !s_forwardBoundsDrag.isActive) {
                s_layerDrag.Activate();

                float layerDragDistance;
                if (s_layerDrag.GetDragDistance(layerDragDistance) && !s_timelineRulerDragged) {
                    ImVec2 reservedBounds = ImVec2(composition->beginFrame, composition->endFrame);

                    composition->beginFrame += layerDragDistance / s_pixelsPerFrame;
                    composition->endFrame += layerDragDistance / s_pixelsPerFrame;

                    float scrollAmount = ProcessLayerScroll();
                    composition->beginFrame += scrollAmount / s_pixelsPerFrame;
                    composition->endFrame += scrollAmount / s_pixelsPerFrame;

                    if (composition->beginFrame < 0) {
                        composition->beginFrame = reservedBounds.x;
                        composition->endFrame = reservedBounds.y;
                    }

                    composition->beginFrame = std::max(composition->beginFrame, 0.0f);
                    composition->endFrame = std::max(composition->endFrame, 0.0f);
                    
                } else s_layerDrag.Deactivate();
            }

            if (compositionHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                ImGui::OpenPopup(FormatString("##compositionPopup%i", composition->id).c_str());
            }

            PopStyleVars();
            if (ImGui::BeginPopup(FormatString("##compositionPopup%i", composition->id).c_str())) {
                RenderCompositionPopup(composition);
                ImGui::EndPopup();
            }
            PushStyleVars();
        }
    }

    void TimelineUI::RenderCompositionPopup(Composition* composition) {
        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_TRASH_CAN, Localization::GetString("DELETE_COMPOSITION").c_str()).c_str())) {
            auto& project = Workspace::s_project.value();
            Workspace::s_selectedCompositions = {};
            int targetCompositionIndex = 0;
            for (auto& iterationComposition : project.compositions) {
                if (composition->id == iterationComposition.id) break;
                targetCompositionIndex++;
            }
            project.compositions.erase(project.compositions.begin() + targetCompositionIndex);
        }
    }
 
    void TimelineUI::RenderTimelineRuler() {
        ImGui::SetCursorPos({0, 0});
        auto& project = Workspace::s_project.value();
        s_timelineRulerDragged = s_timelineDrag.isActive;

        RectBounds timelineBounds(
            ImVec2(project.currentFrame * s_pixelsPerFrame, ImGui::GetScrollY()),
            ImVec2(TIMELINE_RULER_WIDTH, ImGui::GetWindowSize().y)
        );
        DrawRect(timelineBounds, ImVec4(1, 0, 0, 1));

        if (MouseHoveringBounds(timelineBounds)) {
            s_timelineDrag.Activate();
        }

        float timelineDragDistance;
        if (s_timelineDrag.GetDragDistance(timelineDragDistance) && !s_anyLayerDragged) {
            project.currentFrame = GetRelativeMousePos().x / s_pixelsPerFrame;
        } else s_timelineDrag.Deactivate();
    }

    void TimelineUI::RenderLegend() {
        ImGui::BeginChild("##timelineLegend", ImVec2(ImGui::GetWindowSize().x * s_splitterState, ImGui::GetContentRegionAvail().y));
            RenderTicksBar();
            RectBounds backgroundBounds = RectBounds(
                ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY()),
                ImVec2(ImGui::GetWindowSize().x, TICKS_BAR_HEIGHT)
            );
            auto& project = Workspace::s_project.value();
            ImGui::PushFont(Font::s_denseFont);
            ImGui::SetWindowFontScale(1.5f);
            std::string formattedTimestamp = project.FormatFrameToTime(project.currentFrame);
            ImVec2 timestampSize = ImGui::CalcTextSize(formattedTimestamp.c_str());
            ImGui::SetCursorPos(ImVec2(
                backgroundBounds.size.x / 2.0f - timestampSize.x / 2.0f,
                backgroundBounds.size.y / 2.0f - timestampSize.y / 2.0f
            ));
            ImGui::Text("%s", formattedTimestamp.c_str());
            ImGui::PopFont();
            ImGui::SetWindowFontScale(1.0f);
        ImGui::EndChild();
    }

    void TimelineUI::RenderSplitter() {
        ImGui::SetCursorPos({0, 0});
        RectBounds splitterBounds(
            ImVec2(ImGui::GetWindowSize().x * s_splitterState - SPLITTER_RULLER_WIDTH / 2.0f, 0),
            ImVec2(SPLITTER_RULLER_WIDTH, ImGui::GetWindowSize().y)
        );

        RectBounds splitterLogicBounds(
            ImVec2(ImGui::GetWindowSize().x * s_splitterState - SPLITTER_RULLER_WIDTH, 0),
            ImVec2(SPLITTER_RULLER_WIDTH * 2, ImGui::GetWindowSize().y)
        );

        static bool splitterDragging = false;

        if (MouseHoveringBounds(splitterLogicBounds)) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
            DrawRect(splitterBounds, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
                splitterDragging = true;   
            }
        }
        if (splitterDragging && ImGui::IsMouseDown(ImGuiMouseButton_Left) &&ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
            s_splitterState = GetRelativeMousePos().x / ImGui::GetWindowSize().x;
        } else splitterDragging = false;

        s_splitterState = std::clamp(s_splitterState, 0.2f, 0.6f);
    }

    float TimelineUI::ProcessLayerScroll() {
        ImGui::SetCursorPos({0, 0});
        float mouseX = GetRelativeMousePos().x;
        float eventZone = ImGui::GetWindowSize().x / 10.0f;
        if (mouseX > ImGui::GetWindowSize().x - eventZone) {
            ImGui::SetScrollX(ImGui::GetScrollX() + 5);
            return 5;
        }

        if (mouseX < eventZone) {
            if (ImGui::GetScrollX() - 5 > 0) {
                ImGui::SetScrollX(ImGui::GetScrollX() - 5);
            }
            return -5;
        }

        return 0;
    }

    ImVec2 TimelineUI::GetRelativeMousePos() {
        return ImGui::GetIO().MousePos - ImGui::GetCursorScreenPos();
    }
}