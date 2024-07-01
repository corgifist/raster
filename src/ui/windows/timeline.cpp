#include "timeline.h"
#include "font/font.h"
#include "attributes/attributes.h"
#include "common/ui_shared.h"

#define SPLITTER_RULLER_WIDTH 8
#define TIMELINE_RULER_WIDTH 4
#define TICKS_BAR_HEIGHT 30
#define TICK_SMALL_WIDTH 3
#define LAYER_HEIGHT 44
#define LAYER_SEPARATOR 1.5f

#define ROUND_EVEN(x) std::round( (x) * 0.5f ) * 2.0f

namespace Raster {

    enum class TimelineChannels {
        Separators,
        Compositions,
        Timestamps,
        Count
    };

    static void SplitDrawList() {
        ImGui::GetWindowDrawList()->ChannelsSplit(static_cast<int>(TimelineChannels::Count));
    }

    static void SetDrawListChannel(TimelineChannels channel) {
        ImGui::GetWindowDrawList()->ChannelsSetCurrent(static_cast<int>(channel));
    }

    static float s_splitterState = 0.3f;
    static DragStructure s_dragStructure;

    static std::vector<float> s_layerSeparators;

    static std::unordered_map<int, float> s_attributeYCursors;

    static float s_pixelsPerFrame = 4;

    static bool s_timelineRulerDragged = false;
    static bool s_anyLayerDragged = false;
    static bool s_scrollbarActive = true;
    static bool s_layerPopupActive = false;
    static bool s_anyCompositionWasPressed = false;

    static float s_timelineScrollY = 0;

    static float s_compositionsEditorCursorX = 0;

    static DragStructure s_timelineDrag;

    static std::unordered_map<int, float> s_legendOffsets;

    static std::vector<Composition> s_copyCompositions;

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

    static bool ClampedButton(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags, bool& hovered)
    {
        ImVec2 baseCursor = ImGui::GetCursorPos();
        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext& g = *GImGui;
        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);
        const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);

        ImVec2 pos = window->DC.CursorPos;
        if ((flags & ImGuiButtonFlags_AlignTextBaseLine) && style.FramePadding.y < window->DC.CurrLineTextBaseOffset) // Try to vertically align buttons that are smaller/have no padding so that text baseline matches (bit hacky, since it shouldn't be a flag)
            pos.y += window->DC.CurrLineTextBaseOffset - style.FramePadding.y;
        ImVec2 size = ImGui::CalcItemSize(size_arg, label_size.x + style.FramePadding.x * 2.0f, label_size.y + style.FramePadding.y * 2.0f);

        const ImRect bb(pos, pos + size);
        ImGui::ItemSize(size, style.FramePadding.y);
        if (!ImGui::ItemAdd(bb, id))
            return false;

        bool held;
        bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, flags);

        // Render
        const ImU32 col = ImGui::GetColorU32((held && hovered) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
        ImGui::RenderNavHighlight(bb, id);
        ImGui::RenderFrame(bb.Min, bb.Max, col, true, style.FrameRounding);

        if (g.LogEnabled)
            ImGui::LogSetNextTextDecoration("[", "]");

        ImVec2 textMin = bb.Min + style.FramePadding;
        ImVec2 textMax = bb.Max - style.FramePadding;

        ImVec2 reservedCursor = ImGui::GetCursorPos();

        ImVec2 labelCursor = {
            baseCursor.x + size.x / 2.0f - label_size.x / 2.0f,
            baseCursor.y + size.y / 2.0f - label_size.y / 2.0f
        };
        if (labelCursor.x - ImGui::GetScrollX() <= style.FramePadding.x) {
            labelCursor.x = ImGui::GetScrollX() + style.FramePadding.x;
        }
        if (labelCursor.x - ImGui::GetScrollX() >= ImGui::GetWindowSize().x - style.FramePadding.x - label_size.x) {
            labelCursor.x = ImGui::GetScrollX() + ImGui::GetWindowSize().x - style.FramePadding.x - label_size.x;
        }

        ImGui::SetCursorPos(labelCursor);
        ImGui::Text("%s", label);
        ImGui::SetCursorPos(reservedCursor);

        // Automatically close popups
        //if (pressed && !(flags & ImGuiButtonFlags_DontClosePopups) && (window->Flags & ImGuiWindowFlags_Popup))
        //    CloseCurrentPopup();

        IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
        return pressed;
    }

    void TimelineUI::Render() {
        PushStyleVars();

        s_layerPopupActive = false;
        s_anyCompositionWasPressed = false;

        UIShared::s_timelinePixelsPerFrame = s_pixelsPerFrame;
        ImGui::Begin(FormatString("%s %s", ICON_FA_TIMELINE, Localization::GetString("TIMELINE").c_str()).c_str());
            if (ImGui::IsKeyPressed(ImGuiKey_KeypadAdd)  && ImGui::GetIO().KeyCtrl) {
                s_pixelsPerFrame += 1;
            }
            if (ImGui::IsKeyPressed(ImGuiKey_KeypadSubtract) && ImGui::GetIO().KeyCtrl) {
                s_pixelsPerFrame -= 1;
            }
            s_pixelsPerFrame = std::clamp((int) s_pixelsPerFrame, 1, 10);
            UIShared::s_timelineDragged = s_timelineRulerDragged;

            RenderLegend();
            RenderCompositionsEditor();
            RenderSplitter();
            s_legendOffsets.clear();
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
            SetDrawListChannel(TimelineChannels::Compositions);
            DrawRect(tickBounds, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));

            ticksMajorAccumulator++;

            if (majorTick) {
                SetDrawListChannel(TimelineChannels::Timestamps);
                drawList->AddText(
                    ImGui::GetCursorScreenPos() + ImVec2(tickBounds.pos.x + 5, 6.0f),
                    IM_COL32(255, 255, 255, 255), timestampFormattedText.c_str()
                );
                previousTickPositionAccumulator = tickPositionAccumulator;
                ticksMajorAccumulator = 0;
            }
            SetDrawListChannel(TimelineChannels::Compositions);

            tickPositionAccumulator += tickPositionStep;
            tickAccumulator += tickStep;
        }
    }

    void TimelineUI::RenderCompositionsEditor() {
        ImGui::SameLine();
        s_compositionsEditorCursorX = ImGui::GetCursorPosX();

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
            s_timelineScrollY = ImGui::GetScrollY();
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
            SplitDrawList();
            RenderTicks();

            float layerAccumulator = 0;
            for (int i = project.compositions.size(); i --> 0;) {
                auto& composition = project.compositions[i];
                ImGui::SetCursorPosY(backgroundBounds.size.y + layerAccumulator);
                RenderComposition(composition.id);
                float legendOffset = 0;
                if (s_legendOffsets.find(composition.id) != s_legendOffsets.end()) {
                    legendOffset = s_legendOffsets[composition.id];
                }
                layerAccumulator += LAYER_HEIGHT + legendOffset;
            }
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && Workspace::s_selectedCompositions.size() > 1 && !s_anyCompositionWasPressed) {
                Workspace::s_selectedCompositions = {Workspace::s_selectedCompositions[0]};
                std::cout << "overriding compositions" << std::endl;
            }
            
            RenderTimelinePopup();

            RenderTimelineRuler();
        ImGui::EndChild();
    }

    void TimelineUI::RenderComposition(int t_id) {
        auto compositionCandidate = Workspace::GetCompositionByID(t_id);
        auto& project = Workspace::s_project.value();
        if (compositionCandidate.has_value()) {
            auto& composition = compositionCandidate.value();
            auto& selectedComposoitions = Workspace::s_selectedCompositions;
            ImGui::PushID(composition->id);
            ImGui::SetCursorPosX(composition->beginFrame * s_pixelsPerFrame);
            ImVec4 buttonColor = ImGui::ColorConvertU32ToFloat4(ImGui::GetColorU32(ImGuiCol_Button));
            bool isCompositionSelected = false;
            if (std::find(selectedComposoitions.begin(), selectedComposoitions.end(), t_id) != selectedComposoitions.end()) {
                buttonColor = 1.1f * buttonColor;
                isCompositionSelected = true;
            }
            buttonColor.w = 1.0f;
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
            ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, buttonColor * 1.1f);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, buttonColor * 1.2f);
            ImVec2 buttonCursor = ImGui::GetCursorPos();
            ImVec2 buttonSize = ImVec2((composition->endFrame - composition->beginFrame) * s_pixelsPerFrame, LAYER_HEIGHT);
            bool compositionHovered;
            bool compositionPressed = ClampedButton(FormatString("%s %s", ICON_FA_LAYER_GROUP, composition->name.c_str()).c_str(), buttonSize, 0, compositionHovered);
            if (compositionHovered) {
                s_anyCompositionWasPressed = true;
            }
            bool mustDeleteComposition = false;

            static std::vector<DragStructure> s_layerDrags, s_forwardBoundsDrags, s_backwardBoundsDrags;
            s_layerDrags.resize(project.compositions.size());
            s_forwardBoundsDrags.resize(project.compositions.size());
            s_backwardBoundsDrags.resize(project.compositions.size());

            int compositionIndex = 0;
            for (auto& compositionIterable : project.compositions) {
                if (compositionIterable.id == t_id) break;
                compositionIndex++;
            }

            DragStructure& s_layerDrag = s_layerDrags[compositionIndex];
            DragStructure& s_forwardBoundsDrag = s_forwardBoundsDrags[compositionIndex];
            DragStructure& s_backwardBoundsDrag = s_backwardBoundsDrags[compositionIndex];

            ImGui::PopStyleVar();
            ImGui::PopStyleColor(3);
            auto& io = ImGui::GetIO();
            s_anyLayerDragged = s_layerDrag.isActive || s_backwardBoundsDrag.isActive || s_forwardBoundsDrag.isActive;

            if (compositionPressed && !io.KeyCtrl && std::abs(io.MouseDelta.x) < 0.1f) {
                Workspace::s_selectedCompositions = {composition->id};
            }

            if (ImGui::GetIO().KeyCtrl && compositionHovered && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                if (!isCompositionSelected) {
                    AppendSelectedCompositions(composition);
                } else if (!s_layerDrag.isActive && !s_forwardBoundsDrag.isActive && !s_backwardBoundsDrag.isActive) {
                    mustDeleteComposition = true;
                }
            }

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

            if ((MouseHoveringBounds(forwardBoundsDrag) || s_forwardBoundsDrag.isActive) && !s_timelineRulerDragged && !s_layerDrag.isActive && !s_backwardBoundsDrag.isActive && !UIShared::s_timelineAnykeyframeDragged) {
                s_forwardBoundsDrag.Activate();
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                for (auto& selectedComposoitionID : selectedComposoitions) {
                    auto selectedCompositionCandidate = Workspace::GetCompositionByID(selectedComposoitionID);
                    if (selectedCompositionCandidate.has_value()) {
                        auto& selectedComposition = selectedCompositionCandidate.value();

                        float boundsDragDistance;
                        if (s_forwardBoundsDrag.GetDragDistance(boundsDragDistance)) {
                            selectedComposition->endFrame += boundsDragDistance / s_pixelsPerFrame;
                        } else s_forwardBoundsDrag.Deactivate();

                        float scrollAmount = ProcessLayerScroll();
                        selectedComposition->endFrame += scrollAmount / s_pixelsPerFrame;
                    }
                }
            }

            if ((MouseHoveringBounds(backwardBoundsDrag) || s_backwardBoundsDrag.isActive) && !s_timelineRulerDragged && !s_layerDrag.isActive && !s_forwardBoundsDrag.isActive && !UIShared::s_timelineAnykeyframeDragged) {
                s_backwardBoundsDrag.Activate();
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

                for (auto& selectedComposoitionID : selectedComposoitions) {
                    auto selectedCompositionCandidate = Workspace::GetCompositionByID(selectedComposoitionID);
                    if (selectedCompositionCandidate.has_value()) {
                        auto& selectedComposition = selectedCompositionCandidate.value();
                        float boundsDragDistance;
                        if (s_backwardBoundsDrag.GetDragDistance(boundsDragDistance)) {
                            selectedComposition->beginFrame += boundsDragDistance / s_pixelsPerFrame;
                        } else s_backwardBoundsDrag.Deactivate();

                        float scrollAmount = ProcessLayerScroll();
                        selectedComposition->beginFrame += scrollAmount / s_pixelsPerFrame;
                    }
                }
            }
 
            if ((compositionHovered || s_layerDrag.isActive) && !s_timelineRulerDragged && !s_backwardBoundsDrag.isActive && !s_forwardBoundsDrag.isActive && !UIShared::s_timelineAnykeyframeDragged) {
                s_layerDrag.Activate();

                float layerDragDistance;
                if (s_layerDrag.GetDragDistance(layerDragDistance) && !s_timelineRulerDragged) {
                    for (auto& selectedComposoitionID : selectedComposoitions) {
                        bool breakDrag = false;
                        for (auto& testingCompositionID : selectedComposoitions) {
                            auto selectedCompositionCandidate = Workspace::GetCompositionByID(testingCompositionID);
                            if (selectedCompositionCandidate.has_value()) {
                                auto& testingComposition = selectedCompositionCandidate.value();
                                if (testingComposition->beginFrame <= 0) {
                                    breakDrag = true;
                                    break;
                                }
                            }
                        }
                        if (breakDrag && selectedComposoitions.size() > 1 && layerDragDistance < 0) break;
                        auto selectedCompositionCandidate = Workspace::GetCompositionByID(selectedComposoitionID);
                        if (selectedCompositionCandidate.has_value()) {
                            auto& selectedComposition = selectedCompositionCandidate.value();
                            ImVec2 reservedBounds = ImVec2(selectedComposition->beginFrame, selectedComposition->endFrame);

                            selectedComposition->beginFrame += layerDragDistance / s_pixelsPerFrame;
                            selectedComposition->endFrame += layerDragDistance / s_pixelsPerFrame;

                            float scrollAmount = ProcessLayerScroll();
                            selectedComposition->beginFrame += scrollAmount / s_pixelsPerFrame;
                            selectedComposition->endFrame += scrollAmount / s_pixelsPerFrame;

                            if (selectedComposition->beginFrame < 0) {
                                selectedComposition->beginFrame = reservedBounds.x;
                                selectedComposition->endFrame = reservedBounds.y;
                            }

                            selectedComposition->beginFrame = std::max(selectedComposition->beginFrame, 0.0f);
                            selectedComposition->endFrame = std::max(selectedComposition->endFrame, 0.0f);
                        }
                    }
                    
                } else s_layerDrag.Deactivate();
            }

            if (compositionHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                ImGui::OpenPopup(FormatString("##compositionPopup%i", composition->id).c_str());
            }

            PopStyleVars();
            if (ImGui::BeginPopup(FormatString("##compositionPopup%i", composition->id).c_str())) {
                RenderCompositionPopup(composition);
                ImGui::EndPopup();
                s_layerPopupActive = true;
            } else if (!s_layerPopupActive) {
                s_layerPopupActive = false;
            }
            PushStyleVars();
            ImGui::PopID();

            SetDrawListChannel(TimelineChannels::Separators);
            ImGui::SetCursorPosX(0);
            RectBounds separatorBounds(
                ImVec2(ImGui::GetScrollX(), buttonCursor.y + buttonSize.y - LAYER_SEPARATOR / 2.0f),
                ImVec2(ImGui::GetWindowSize().x, LAYER_SEPARATOR)
            );
            DrawRect(separatorBounds, ImGui::GetStyleColorVec4(ImGuiCol_Separator));
            SetDrawListChannel(TimelineChannels::Compositions);

            for (auto& attribute : composition->attributes) {
                ImGui::SetCursorPosY(s_attributeYCursors[attribute->id]);
                PopStyleVars();
                attribute->RenderKeyframes();
                PushStyleVars();
            }

            if (mustDeleteComposition) {
                int compositionIndex = 0;
                for (auto& selectedComposoition : selectedComposoitions) {
                    if (selectedComposoition == t_id) break;
                    compositionIndex++;
                }
                selectedComposoitions.erase(selectedComposoitions.begin() + compositionIndex);
            }
        }
    }

    void TimelineUI::DeleteComposition(Composition* composition) {
        auto& project = Workspace::s_project.value();
        Workspace::s_selectedCompositions = {};
        int targetCompositionIndex = 0;
        for (auto& iterationComposition : project.compositions) {
            if (composition->id == iterationComposition.id) break;
            targetCompositionIndex++;
        }
        project.compositions.erase(project.compositions.begin() + targetCompositionIndex);
    }

    void TimelineUI::AppendSelectedCompositions(Composition* composition) {
        auto& selectedCompositions = Workspace::s_selectedCompositions;
        if (std::find(selectedCompositions.begin(), selectedCompositions.end(), composition->id) == selectedCompositions.end()) {
            selectedCompositions.push_back(composition->id);
        }
    }

    void TimelineUI::RenderCompositionPopup(Composition* composition) {
        auto& selectedCompositions = Workspace::s_selectedCompositions;
        ImGui::SeparatorText(FormatString("%s %s", ICON_FA_LAYER_GROUP, composition->name.c_str()).c_str());
        if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_PLUS, Localization::GetString("ADD_ATTRIBUTE").c_str()).c_str())) {
            RenderNewAttributePopup(composition);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("EDIT_METADATA").c_str()).c_str())) {
            ImGui::InputText("##compositionName", &composition->name);
            ImGui::SetItemTooltip("%s %s", ICON_FA_PENCIL, Localization::GetString("COMPOSITION_NAME").c_str());
            ImGui::InputTextMultiline("##compositionDescription", &composition->description);
            ImGui::SetItemTooltip("%s %s", ICON_FA_PENCIL, Localization::GetString("COMPOSITION_DESCRIPTION").c_str());
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_ARROW_POINTER, Localization::GetString("SELECT_COMPOSITION").c_str()).c_str())) {
            AppendSelectedCompositions(composition);
        }
        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_TRASH_CAN, Localization::GetString("DELETE_SELECTED_COMPOSITIONS").c_str()).c_str(), "Delete")) {
            auto selectedCompositionsCopy = selectedCompositions;
            for (auto& compositionID : selectedCompositions) {
                auto compositionCandidate = Workspace::GetCompositionByID(compositionID);
                if (compositionCandidate.has_value()) {
                    DeleteComposition(compositionCandidate.value());
                }
            }
        }

        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_COPY, Localization::GetString("COPY_SELECTED_COMPOSITIONS").c_str()).c_str(), "Ctrl+C")) {
            ProcessCopyAction();
        }
        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_PASTE, Localization::GetString("PASTE_SELECTED_COMPOSITIONS").c_str()).c_str(), "Ctrl+V")) {
            ProcessPasteAction();
        }
    }

    void TimelineUI::ProcessCopyAction() {
        std::vector<Composition> copiedCompositions;
        for (auto& selectedComposition : Workspace::s_selectedCompositions) {
            auto compositionCandidate = Workspace::GetCompositionByID(selectedComposition);
            if (compositionCandidate.has_value()) {
                auto& composition = compositionCandidate.value();
                Composition copiedComposition = Composition(composition->Serialize());
                copiedComposition.name += " (" + Localization::GetString("COPY") + ")";
                copiedComposition.id = Randomizer::GetRandomInteger();
                for (auto& attribute : copiedComposition.attributes) {
                    attribute->id = Randomizer::GetRandomInteger();
                    for (auto& keyframe : attribute->keyframes) {
                        keyframe.id = Randomizer::GetRandomInteger();
                    }
                }
                std::unordered_map<int, int> idReplacements;
                for (auto& node : copiedComposition.nodes) {
                    int originalID = node->nodeID;
                    node->nodeID = Randomizer::GetRandomInteger();
                    idReplacements[originalID] = node->nodeID;

                    if (node->flowInputPin.has_value()) {
                        UpdateCopyPin(node->flowInputPin.value(), idReplacements);
                    }
                    if (node->flowOutputPin.has_value()) {
                        UpdateCopyPin(node->flowOutputPin.value(), idReplacements);
                    }
                    for (auto& inputPin : node->inputPins) {
                        UpdateCopyPin(inputPin, idReplacements);
                    }
                    for (auto& outputPin : node->outputPins) {
                        UpdateCopyPin(outputPin, idReplacements);
                    }
                }
                
                for (auto& node : copiedComposition.nodes) {
                    if (node->flowInputPin.has_value()) {
                        ReplaceCopyPin(node->flowInputPin.value(), idReplacements);
                    }
                    if (node->flowOutputPin.has_value()) {
                        ReplaceCopyPin(node->flowOutputPin.value(), idReplacements);
                    }
                    for (auto& inputPin : node->inputPins) {
                        ReplaceCopyPin(inputPin, idReplacements);
                    }
                    for (auto& outputPin : node->outputPins) {
                        ReplaceCopyPin(outputPin, idReplacements);
                    }
                }
                copiedCompositions.push_back(copiedComposition);
            }
        }
        s_copyCompositions = copiedCompositions;
    }

    void TimelineUI::ProcessPasteAction() {
        auto& project = Workspace::s_project.value();
        for (auto& composition : s_copyCompositions) {
            project.compositions.push_back(composition);
        }
    }

    void TimelineUI::UpdateCopyPin(GenericPin& pin, std::unordered_map<int, int>& idReplacements) {
        int originalID = pin.pinID;
        pin.pinID = Randomizer::GetRandomInteger();
        pin.linkID = Randomizer::GetRandomInteger();
        idReplacements[originalID] = pin.pinID;
    }

    void TimelineUI::ReplaceCopyPin(GenericPin& pin, std::unordered_map<int, int>& idReplacements) {
        if (idReplacements.find(pin.connectedPinID) != idReplacements.end()) {
            pin.connectedPinID = idReplacements[pin.connectedPinID];
        }
    }

    void TimelineUI::RenderNewAttributePopup(Composition* t_composition) {
        ImGui::SeparatorText(FormatString("%s %s", ICON_FA_PLUS, Localization::GetString("ADD_ATTRIBUTE").c_str()).c_str());
        for (auto& entry : Attributes::s_attributes) {
            if (ImGui::MenuItem(FormatString("%s %s %s", ICON_FA_PLUS, entry.prettyName.c_str(), Localization::GetString("ATTRIBUTE").c_str()).c_str())) {
                auto attributeCandidate = Attributes::InstantiateAttribute(entry.packageName);
                if (attributeCandidate.has_value()) {
                    t_composition->attributes.push_back(attributeCandidate.value());
                }
            }
        }
    }

    void TimelineUI::RenderTimelinePopup() {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !s_layerPopupActive && ImGui::IsWindowFocused()) {
            ImGui::OpenPopup("##layerPopup");
        }
        PopStyleVars();
        if (ImGui::BeginPopup("##layerPopup")) {
            ImGui::SeparatorText(FormatString("%s %s", ICON_FA_TIMELINE, Localization::GetString("TIMELINE").c_str()).c_str());
            if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_PLUS, Localization::GetString("NEW_COMPOSITION").c_str()).c_str())) {
                Workspace::s_project.value().compositions.push_back(Composition());
            }
            ImGui::EndPopup();
        }
        PushStyleVars();
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
        if (s_timelineDrag.GetDragDistance(timelineDragDistance) && !s_anyLayerDragged && !UIShared::s_timelineAnykeyframeDragged) {
            project.currentFrame = GetRelativeMousePos().x / s_pixelsPerFrame;
        } else s_timelineDrag.Deactivate();
    }

    void TimelineUI::RenderLegend() {
        ImGui::BeginChild("##timelineLegend", ImVec2(ImGui::GetWindowSize().x * s_splitterState, ImGui::GetContentRegionAvail().y));
            ImGui::SetScrollY(s_timelineScrollY);
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

            ImGui::SetCursorPos({0, backgroundBounds.size.y});

            Composition* targetCompositionDelete = nullptr;
            float layerAccumulator = 0;
            for (int i = project.compositions.size(); i --> 0;) {
                auto& composition = project.compositions[i];
                std::string compositionName = FormatString("%s %s", ICON_FA_LAYER_GROUP, composition.name.c_str());
                ImVec2 compositionNameSize = ImGui::CalcTextSize(compositionName.c_str());
                ImVec2 baseCursor = ImVec2{
                    0, backgroundBounds.size.y + layerAccumulator
                };
                PopStyleVars();
                ImGui::SetCursorPos({5, backgroundBounds.size.y + layerAccumulator + LAYER_HEIGHT * 0.5f - compositionNameSize.y / 2.0f});
                ImGui::PushID(composition.id);
                if (ImGui::Button(ICON_FA_TRASH_CAN)) {
                    targetCompositionDelete = &composition;
                }
                ImGui::SameLine();
                if (ImGui::Button(ICON_FA_PLUS)) {
                    ImGui::OpenPopup(FormatString("##createAttribute%i", composition.id).c_str());
                }
                if (ImGui::BeginPopup(FormatString("##createAttribute%i", composition.id).c_str())) {
                    RenderNewAttributePopup(&composition);
                    ImGui::EndPopup();
                }
                ImGui::SameLine();
                auto selectedIterator = std::find(Workspace::s_selectedCompositions.begin(), Workspace::s_selectedCompositions.end(), composition.id);
                bool compositionTreeExpanded = ImGui::TreeNode(FormatString("%s%s", selectedIterator != Workspace::s_selectedCompositions.end() ? ICON_FA_ARROW_POINTER " " : "", compositionName.c_str()).c_str());
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                    ImGui::OpenPopup(FormatString("##accessibilityPopup%i", composition.id).c_str());
                }
                if (ImGui::BeginPopup(FormatString("##accessibilityPopup%i", composition.id).c_str())) {
                    RenderCompositionPopup(&composition);
                    ImGui::EndPopup();
                }
                auto treeNodeID = ImGui::GetItemID();
                ImGui::SetCursorPos(baseCursor);
                ImGui::SetNextItemAllowOverlap();
                if (ImGui::InvisibleButton("##accessibilityButton", ImVec2(ImGui::GetWindowSize().x, LAYER_HEIGHT))) {
                    ImGui::TreeNodeSetOpen(treeNodeID, !compositionTreeExpanded);
                }
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                    ImGui::OpenPopup(FormatString("##compositionLegendPopup%i", composition.id).c_str());
                }

                ImVec2 reservedCursor = ImGui::GetCursorPos();
                ImGui::SetCursorPos({0, backgroundBounds.size.y + layerAccumulator + LAYER_HEIGHT - LAYER_SEPARATOR / 2.0f});
                RectBounds separatorBounds(
                    ImVec2(0, 0), 
                    ImVec2(ImGui::GetWindowSize().x, LAYER_SEPARATOR)
                );

                DrawRect(separatorBounds, ImGui::GetStyleColorVec4(ImGuiCol_Separator));
                ImGui::SetCursorPos(reservedCursor);

                if (ImGui::BeginPopup(FormatString("##compositionLegendPopup%i", composition.id).c_str())) {
                    RenderCompositionPopup(&composition);
                    ImGui::EndPopup();
                }

                if (compositionTreeExpanded) {
                    float firstCursor = ImGui::GetCursorPosY();
                    if (composition.attributes.empty()) {
                        ImGui::Text("%s", Localization::GetString("NO_ATTRIBUTES").c_str());
                    }
                    for (auto& attribute : composition.attributes) {
                        s_attributeYCursors[attribute->id] = ImGui::GetCursorPosY();
                        float firstAttribueCursor = ImGui::GetCursorPosY();
                        attribute->RenderLegend(&composition);
                        UIShared::s_timelineAttributeHeights[composition.id] = ImGui::GetCursorPosY() - firstAttribueCursor;
                    }
                    ImGui::TreePop();
                    s_legendOffsets[composition.id] = ImGui::GetCursorPosY() - firstCursor;
                }
                ImGui::PopID();
                PushStyleVars();

                layerAccumulator += LAYER_HEIGHT + s_legendOffsets[composition.id];
            }

        ImGui::EndChild();

        if (targetCompositionDelete) {
            DeleteComposition(targetCompositionDelete);
        }
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
        if (splitterDragging && ImGui::IsMouseDown(ImGuiMouseButton_Left) &&ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) && !s_anyLayerDragged && !s_timelineRulerDragged && !UIShared::s_timelineAnykeyframeDragged) {
            s_splitterState = GetRelativeMousePos().x / ImGui::GetWindowSize().x;
        } else splitterDragging = false;

        s_splitterState = std::clamp(s_splitterState, 0.2f, 0.6f);
    }

    float TimelineUI::ProcessLayerScroll() {
        ImGui::SetCursorPos({0, 0});
        float mouseX = GetRelativeMousePos().x;
        float eventZone = ImGui::GetWindowSize().x / 10.0f;
        if (mouseX > ImGui::GetWindowSize().x - eventZone && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            ImGui::SetScrollX(ImGui::GetScrollX() + 5);
            return 5;
        }

        if (mouseX < eventZone && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
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