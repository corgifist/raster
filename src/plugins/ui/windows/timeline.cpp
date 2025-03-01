#include "timeline.h"
#include "common/composition_mask.h"
#include "common/localization.h"
#include "common/ui_helpers.h"
#include "font/IconsFontAwesome5.h"
#include "font/font.h"
#include "common/attributes.h"
#include "common/ui_shared.h"
#include "compositor/compositor.h"
#include "compositor/blending.h"
#include "common/transform2d.h"
#include "common/dispatchers.h"
#include "common/rendering.h"
#include "common/waveform_manager.h"
#include "gpu/gpu.h"
#include "raster.h"
#include "common/line2d.h"
#include "common/layouts.h"

#define SPLITTER_RULLER_WIDTH 8
#define TIMELINE_RULER_WIDTH 4
#define TICKS_BAR_HEIGHT 30
#define TICK_SMALL_WIDTH 3
#define LAYER_HEIGHT 44
#define LAYER_SEPARATOR 1

#define ROUND_EVEN(x) std::round( (x) * 0.5f ) * 2.0f

#define LAYER_REARRANGE_DRAG_DROP "LAYER_REARRANGE_DRAG_DROP"
#define LAYER_LOCK_DRAG_DROP "LAYER_LOCK_DRAG_DROP"

namespace Raster {

    enum class TimelineChannels {
        Compositions,
        Separators,
        Timestamps,
        TimelineRuler,
        Count
    };

    enum class TimestampFormat {
        Regular, Frame, Seconds
    };

    static void SplitDrawList() {
        ImGui::GetWindowDrawList()->ChannelsSplit(static_cast<int>(TimelineChannels::Count));
    }

    static void SetDrawListChannel(TimelineChannels channel) {
        ImGui::GetWindowDrawList()->ChannelsSetCurrent(static_cast<int>(channel));
    }

    static glm::vec2 FitRectInRect(glm::vec2 dst, glm::vec2 src) {
        float scale = std::min(dst.x / src.x, dst.y / src.y);
        return glm::vec2{src.x * scale, src.y * scale};
    }

    static float s_splitterState = 0.3f;
    static DragStructure s_dragStructure;

    static std::vector<float> s_layerSeparators;

    static std::unordered_map<int, float> s_attributeYCursors;
    static std::unordered_map<int, bool> s_attributesExpanded;
    static std::unordered_map<int, ImGuiID> s_compositionTrees;
    static std::unordered_map<int, float> s_compositionTreeScrolls;

    static float s_targetLegendScroll = -1;

    static float s_pixelsPerFrame = 4;

    static bool s_timelineRulerDragged = false;
    static bool s_anyLayerDragged = false;
    static bool s_scrollbarActive = true;
    static bool s_layerPopupActive = false;
    static bool s_anyCompositionWasPressed = false;
    static bool s_timelineFocused = false;

    static float s_timelineScrollY = 0;

    static float s_compositionsEditorCursorX = 0;

    static DragStructure s_timelineDrag;

    static std::unordered_map<int, float> s_legendOffsets;
    static ImGuiID s_legendTargetOpenTree = 0;

    static ImVec2 s_rootWindowSize;

    static std::vector<Composition> s_copyCompositions;

    static std::string s_compositionFilter = "";
    static uint32_t s_colorMarkFilter = IM_COL32(0, 0, 0, 0);

    static ImVec2 s_timelineMousePos(0, 0);

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

    static bool ClampedButton(const char* label, const ImVec2& size_arg, ImGuiButtonFlags flags, bool& hovered, Composition* t_composition = nullptr)
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

        if (t_composition) {
            TimelineUI::RenderLayerDragDrop(t_composition);
        }

        if (g.LogEnabled)
            ImGui::LogSetNextTextDecoration("[", "]");

        ImVec2 textMin = bb.Min + style.FramePadding;
        ImVec2 textMax = bb.Max - style.FramePadding;

        // Automatically close popups
        //if (pressed && !(flags & ImGuiButtonFlags_DontClosePopups) && (window->Flags & ImGuiWindowFlags_Popup))
        //    CloseCurrentPopup();

        IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);
        return pressed;
    }

    static bool TextColorButton(const char* id, ImVec4 color) {
        if (ImGui::BeginChild(FormatString("##%scolorMark", id).c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 0), ImGuiChildFlags_AutoResizeY)) {
            ImGui::SetCursorPos({0, 0});
            ImGui::PushStyleColor(ImGuiCol_Button, color);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color * 1.1f);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, color * 1.2f);
            ImGui::ColorButton(FormatString("%s %s", ICON_FA_DROPLET, id).c_str(), color, ImGuiColorEditFlags_AlphaPreview);
            ImGui::PopStyleColor(3);
            ImGui::SameLine();
            if (ImGui::IsWindowHovered()) ImGui::BeginDisabled();
            std::string defaultColorMarkText = "";
            if (Workspace::s_defaultColorMark == id) {
                defaultColorMarkText = FormatString(" (%s)", Localization::GetString("DEFAULT").c_str());
            }
            ImGui::Text("%s %s%s", ICON_FA_DROPLET, id, defaultColorMarkText.c_str());
            if (ImGui::IsWindowHovered()) ImGui::EndDisabled();
        }
        ImGui::EndChild();
        return ImGui::IsItemClicked();
    }

    Json TimelineUI::AbstractSerialize() {
        return {};
    }

    void TimelineUI::AbstractLoad(Json t_data) {
        
    }

    void TimelineUI::AbstractRender() {
        PushStyleVars();
        if (!open) {
            Layouts::DestroyWindow(id);
        }
        s_layerPopupActive = false;
        s_anyCompositionWasPressed = false;

        UIShared::s_timelinePixelsPerFrame = s_pixelsPerFrame;
        ImGui::SetNextWindowSize(ImVec2(700, 500), ImGuiCond_FirstUseEver);
        if (ImGui::Begin(FormatString("%s %s###%i", ICON_FA_TIMELINE, Localization::GetString("TIMELINE").c_str(), id).c_str(), &open)) {
            if (!Workspace::IsProjectLoaded()) {
                ImGui::PushFont(Font::s_denseFont);
                ImGui::SetWindowFontScale(2.0f);
                    ImVec2 exclamationSize = ImGui::CalcTextSize(ICON_FA_TRIANGLE_EXCLAMATION);
                    ImGui::SetCursorPos(ImGui::GetWindowSize() / 2.0f - exclamationSize / 2.0f);
                    ImGui::Text(ICON_FA_TRIANGLE_EXCLAMATION);
                ImGui::SetWindowFontScale(1.0f);
                ImGui::PopFont();
                ImGui::End();
                PopStyleVars();
                return;
            }
            auto& project = Workspace::GetProject();
            if (project.customData.contains("TimelineColorFilter")) {
                s_colorMarkFilter = project.customData["TimelineColorFilter"];
            }
            if (project.customData.contains("TimelineSplitterState")) {
                s_splitterState = project.customData["TimelineSplitterState"];
            }
            s_rootWindowSize = ImGui::GetWindowSize();
            s_timelineFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
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
            project.customData["TimelineColorFilter"] = s_colorMarkFilter;
            project.customData["TimelineSplitterState"] = s_splitterState;
        }
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

        DrawRect(backgroundBounds, ImGui::GetStyleColorVec4(ImGuiCol_WindowBg));
        if (MouseHoveringBounds(backgroundBounds) && !s_anyLayerDragged) {
            s_timelineDrag.Activate();
        }

        SetDrawListChannel(TimelineChannels::Separators);
        ImGui::GetWindowDrawList()->AddLine(ImVec2(
            backgroundBounds.UL.x, backgroundBounds.BR.y
        ), backgroundBounds.BR, IM_COL32(0, 0, 0, 255), 1.0f);
        SetDrawListChannel(TimelineChannels::Compositions);
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

            SetDrawListChannel(TimelineChannels::Compositions);
            ImDrawList* drawList = ImGui::GetWindowDrawList();
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

    void TimelineUI::ProcessShortcuts() {
        // DUMP_VAR((int) UIShared::s_lastClickedObjectType);
        if (!UIHelpers::AnyItemFocused() && ImGui::IsWindowFocused() && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_C) && UIShared::s_lastClickedObjectType == LastClickedObjectType::Composition) {
            ProcessCopyAction();
        }
        if (!UIHelpers::AnyItemFocused() && ImGui::IsWindowFocused() && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V)&& UIShared::s_lastClickedObjectType == LastClickedObjectType::Composition) {
            ProcessPasteAction();
        }
        if (!UIHelpers::AnyItemFocused() && ImGui::IsWindowFocused() && ImGui::IsKeyPressed(ImGuiKey_Delete) && UIShared::s_lastClickedObjectType == LastClickedObjectType::Composition) {
            ProcessDeleteAction();
        }
        if (!UIHelpers::AnyItemFocused() && ImGui::IsWindowFocused() && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D) && UIShared::s_lastClickedObjectType == LastClickedObjectType::Composition) {
            ProcessCopyAction();
            ProcessPasteAction();
        }
        if (!UIHelpers::AnyItemFocused() && ImGui::IsWindowFocused() && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_R) && UIShared::s_lastClickedObjectType == LastClickedObjectType::Composition) {
            ProcessResizeToMatchContentDurationAction();
        }
        if (!UIHelpers::AnyItemFocused() && ImGui::IsWindowFocused() && ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_X) && UIShared::s_lastClickedObjectType == LastClickedObjectType::Composition) {
            ProcessCutAction();
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
        ImVec4 editorBackgroundColor = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg) * 1.2f;
        editorBackgroundColor.w = 1.0f;
        ImGui::PushStyleColor(ImGuiCol_ChildBg, editorBackgroundColor);
        bool timelineCompositionsChildStatus = ImGui::BeginChild("##timelineCompositions", ImVec2(ImGui::GetWindowSize().x * (1 - s_splitterState), ImGui::GetContentRegionAvail().y), 0, timelineFlags);
        ImGui::PopStyleColor();
        if (timelineCompositionsChildStatus) {
            ImVec2 baseCursorScreen = ImGui::GetCursorScreenPos();
            SplitDrawList();
            AttributeBase::ProcessKeyframeShortcuts();

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
            RenderTicks();

            float layerAccumulator = 0;
            s_anyLayerDragged = false;
            ImVec2 clipRectUL = baseCursorScreen;
            clipRectUL.x += ImGui::GetScrollX();
            clipRectUL.y += TICKS_BAR_HEIGHT + ImGui::GetScrollY();
            ImVec2 clipRectBR = baseCursorScreen + ImGui::GetWindowSize();
            clipRectBR.x += ImGui::GetScrollX();
            clipRectBR.y += ImGui::GetScrollY();
            ImGui::GetWindowDrawList()->PushClipRect(clipRectUL, clipRectBR, true);
            for (int i = project.compositions.size(); i --> 0;) {
                auto& composition = project.compositions[i];
                if (!s_compositionFilter.empty() && LowerCase(composition.name).find(LowerCase(s_compositionFilter)) == std::string::npos) continue;
                if (s_colorMarkFilter != IM_COL32(0, 0, 0, 0) && composition.colorMark != s_colorMarkFilter) continue;
                ImGui::SetCursorPosY(backgroundBounds.size.y + layerAccumulator);
                RenderComposition(composition.id);
                float legendOffset = 0;
                if (s_legendOffsets.find(composition.id) != s_legendOffsets.end()) {
                    legendOffset = s_legendOffsets[composition.id];
                }
                layerAccumulator += LAYER_HEIGHT + legendOffset;
            }
            ImGui::GetWindowDrawList()->PopClipRect();
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && project.selectedCompositions.size() > 1 && !s_anyCompositionWasPressed && ImGui::IsWindowFocused()) {
                project.selectedCompositions = {project.selectedCompositions[0]};
                UIShared::s_lastClickedObjectType = LastClickedObjectType::Composition;
            }
            
            RenderTimelinePopup();

            ImGui::SetCursorPos({0, 0});
            RectBounds shadowGradientBounds(
                ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY()), 
                ImVec2(30, ImGui::GetWindowSize().x)
            );
            int shadowAlpha = 128;
            ImGui::GetWindowDrawList()->AddRectFilledMultiColor(
                shadowGradientBounds.UL, shadowGradientBounds.BR,
                IM_COL32(0, 0, 0, shadowAlpha), IM_COL32(0, 0, 0, 0),
                IM_COL32(0, 0, 0, 0), IM_COL32(0, 0, 0, shadowAlpha) 
            );
            RenderTimelineRuler();
        }
        
        ProcessShortcuts();
        s_timelineMousePos = GetRelativeMousePos();
        ImGui::EndChild();

        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(ASSET_MANAGER_DRAG_DROP_PAYLOAD)) {
                auto assetID = *((int*) payload->Data);
                auto assetCandidate = Workspace::GetAssetByAssetID(assetID);
                if (assetCandidate) {
                    auto& asset = *assetCandidate;
                    asset->OnTimelineDrop((ImGui::GetScrollX() + s_timelineMousePos.x) / s_pixelsPerFrame);
                }
            }
            ImGui::EndDragDropTarget();
        }
    }

    void TimelineUI::RenderComposition(int t_id) {
        auto compositionCandidate = Workspace::GetCompositionByID(t_id);
        auto& project = Workspace::s_project.value();
        if (compositionCandidate.has_value()) {
            auto& composition = compositionCandidate.value();
            auto& selectedCompositions = project.selectedCompositions;
            ImGui::PushID(composition->id);
            ImGui::SetCursorPosX(std::ceil(composition->beginFrame * s_pixelsPerFrame));
            ImVec4 buttonColor = ImGui::ColorConvertU32ToFloat4(composition->colorMark);
            bool isCompositionSelected = false;
            if (std::find(selectedCompositions.begin(), selectedCompositions.end(), t_id) != selectedCompositions.end()) {
                buttonColor = 1.1f * buttonColor;
                isCompositionSelected = true;
            }
            if (!composition->enabled) buttonColor = buttonColor * 0.8f;
            buttonColor.w = 1.0f;
            PopStyleVars();
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);
            ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, buttonColor * 1.1f);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, buttonColor * 1.2f);
            ImVec2 buttonCursor = ImGui::GetCursorPos();
            ImVec2 buttonSize = ImVec2(std::ceil((composition->endFrame - composition->beginFrame) * s_pixelsPerFrame), LAYER_HEIGHT);
            bool compositionHovered;
            std::string compositionIcons = "";
            if (composition->lockedCompositionID > 0) compositionIcons += ICON_FA_LOCK " ";
            if (composition->audioEnabled) compositionIcons += ICON_FA_VOLUME_HIGH;
            else compositionIcons += ICON_FA_VOLUME_OFF;
            compositionIcons += " ";
            compositionIcons += ICON_FA_LAYER_GROUP;
            std::string compositionLabel = FormatString("%s %s", compositionIcons.c_str(), composition->name.c_str());
            bool compositionPressed = ClampedButton(compositionLabel.c_str(), buttonSize, 0, compositionHovered, composition);
            if (compositionHovered) {
                s_anyCompositionWasPressed = true;
            }
            bool mustDeleteComposition = false;

            if (UIShared::s_timelineAttributeHeights.find(t_id) != UIShared::s_timelineAttributeHeights.end()) {
                auto dummyY = UIShared::s_timelineAttributeHeights[t_id];
                ImGui::SetCursorPos({buttonCursor.x, buttonCursor.y + LAYER_HEIGHT});
                ImGui::GetWindowDrawList()->AddRectFilled(ImGui::GetCursorScreenPos(), ImGui::GetCursorScreenPos() + ImVec2{buttonSize.x, dummyY}, 0xFF);
                ImGui::Dummy({buttonSize.x, dummyY});
                ImGui::SetCursorPos({buttonCursor.x, buttonCursor.y + LAYER_HEIGHT});
            }

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
            PushStyleVars();
            auto& io = ImGui::GetIO();
            // auto& project = Workspace::GetProject();

            if (compositionPressed && !io.KeyCtrl && std::abs(io.MouseDelta.x) < 0.1f) {
                project.selectedCompositions = {composition->id};
                UIShared::s_lastClickedObjectType = LastClickedObjectType::Composition;
                std::vector<int> newSelectedAttributes = {};
                if (!composition->attributes.empty()) {
                    for (auto& attribute : composition->attributes) {
                        auto& valueType = attribute->Get(project.currentFrame - composition->beginFrame, composition).type();
                        if (valueType == typeid(Transform2D) || valueType == typeid(Line2D)) {
                            newSelectedAttributes.push_back(attribute->id);
                        }
                    }
                    project.selectedAttributes = newSelectedAttributes;
                }
            }

            if (ImGui::GetIO().KeyCtrl && compositionHovered && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                if (!isCompositionSelected) {
                    AppendSelectedCompositions(composition);
                } else if (!s_layerDrag.isActive && !s_forwardBoundsDrag.isActive && !s_backwardBoundsDrag.isActive) {
                    mustDeleteComposition = true;
                }
                UIShared::s_lastClickedObjectType = LastClickedObjectType::Composition;
            }

            ImVec2 dragSize = ImVec2((composition->endFrame - composition->beginFrame) * s_pixelsPerFrame / 10, LAYER_HEIGHT - 1);
            dragSize.x = std::clamp(dragSize.x, 1.0f, 30.0f);

            ImGui::SetCursorPos({0, 0});
            RectBounds forwardBoundsDrag(
                ImVec2(buttonCursor.x + buttonSize.x - dragSize.x, buttonCursor.y),
                dragSize
            );

            RectBounds backwardBoundsDrag(
                ImVec2(buttonCursor.x, buttonCursor.y), dragSize
            );

            ImVec4 dragColor = buttonColor * 0.7f;
            dragColor.w = 1.0f;

            ImVec4 forwardDragColor = dragColor;
            if (MouseHoveringBounds(forwardBoundsDrag) && s_timelineFocused) {
                forwardDragColor = forwardDragColor * 1.1f;
                if (ImGui::GetIO().MouseDown[ImGuiMouseButton_Left]) {
                    forwardDragColor = forwardDragColor * 1.1f;
                }
            }

            ImVec4 backwardDragColor = dragColor;
            if (MouseHoveringBounds(backwardBoundsDrag) && s_timelineFocused) {
                backwardDragColor = backwardDragColor * 1.1f;
                if (ImGui::GetIO().MouseDown[ImGuiMouseButton_Left]) {
                    backwardDragColor = backwardDragColor * 1.1f;
                }
            }

            forwardDragColor.w = 1.0f;


            DrawRect(forwardBoundsDrag, forwardDragColor);
            DrawRect(backwardBoundsDrag, backwardDragColor);

            auto& waveformRecordsSync = WaveformManager::GetRecords();
            waveformRecordsSync.Lock();
            auto& waveformRecords = waveformRecordsSync.GetReference();
            if (waveformRecords.find(t_id) != waveformRecords.end()) {
                auto originalButtonCursor = ImGui::GetCursorPos();
                ImGui::SetCursorPos(buttonCursor);
                auto& waveformRecord = waveformRecords[t_id];
                ImVec2 originalCursor = ImGui::GetCursorScreenPos();
                ImVec2 layerEndCursor = {originalCursor.x + composition->endFrame * s_pixelsPerFrame, originalCursor.y};
                float pixelAdvance = (float) waveformRecord.precision / (float) AudioInfo::s_sampleRate * project.framerate * s_pixelsPerFrame;
                float advanceAccumulator = 0;
                ImVec4 waveformColor = buttonColor * 0.7f;
                waveformColor.w = 1.0f;
                for (size_t i = 0; i < waveformRecord.data.size(); i++) {
                    if (advanceAccumulator > (composition->endFrame - composition->beginFrame) * s_pixelsPerFrame) break;
                    if (originalCursor.x < s_splitterState * s_rootWindowSize.x) {
                        originalCursor.x += pixelAdvance;
                        advanceAccumulator += pixelAdvance;
                        continue;
                    }
                    if (originalCursor.x > ImGui::GetWindowSize().x + s_splitterState * s_rootWindowSize.x) break;
                    float average = glm::abs(waveformRecord.data[i]);
                    float averageInPixels = average * LAYER_HEIGHT;
                    float invertedAverageInPixels = LAYER_HEIGHT - averageInPixels;
                    ImVec2 bottomRight = originalCursor;
                    bottomRight.y += LAYER_HEIGHT - 1;
                    bottomRight.x += pixelAdvance;
                    ImVec2 upperLeft = originalCursor;
                    upperLeft.y += invertedAverageInPixels + 1;
                    ImGui::GetWindowDrawList()->AddRectFilled(upperLeft, bottomRight, ImGui::GetColorU32(waveformColor));
                    // RASTER_LOG("drawing waveform");
                    originalCursor.x += pixelAdvance;
                    advanceAccumulator += pixelAdvance;
                }
                ImGui::SetCursorPos(originalButtonCursor);
            }
            waveformRecordsSync.Unlock();

            auto& bundles = Compositor::s_bundles.GetFrontValue();
            ImVec2 bundlePreviewSize(0, 0);
            if (bundles.find(t_id) != bundles.end()) {
                auto& bundle = bundles[t_id];
                if (bundle.primaryFramebuffer.handle) {
                    auto& framebuffer = bundle.primaryFramebuffer;
                    glm::vec2 previewSize = {LAYER_HEIGHT * ((float) framebuffer.width / (float) framebuffer.height), LAYER_HEIGHT};
                    glm::vec2 rectSize = FitRectInRect(previewSize, glm::vec2{(float) framebuffer.width, (float) framebuffer.height});
                    float legendWidth = s_splitterState * s_rootWindowSize.x;
                    ImVec2 upperLeft = ImGui::GetCursorScreenPos() + buttonCursor;
                    ImVec2 bottomRight = upperLeft;
                    bottomRight += ImVec2{rectSize.x, rectSize.y};
                    upperLeft.x += dragSize.x;
                    bottomRight.x += dragSize.x;
                    upperLeft.x = glm::max(upperLeft.x, legendWidth);
                    bottomRight.x = glm::max(bottomRight.x, legendWidth + rectSize.x);
                    bottomRight.x = glm::min(bottomRight.x, ImGui::GetCursorScreenPos().x + buttonCursor.x + buttonSize.x - dragSize.x);
                    upperLeft.y += 1;
                    bottomRight.y -= 1;
                    bundlePreviewSize = bottomRight - upperLeft;
                    auto reservedCursor = ImGui::GetCursorPos();
                    ImGui::SetCursorScreenPos(upperLeft);
                    ImGui::Stripes(ImVec4(0.05f, 0.05f, 0.05f, 0.8f), ImVec4(0.1f, 0.1f, 0.1f, 0.8f), 14, 214, bottomRight - upperLeft);
                    ImGui::GetWindowDrawList()->AddImage((ImTextureID) framebuffer.attachments[0].handle, upperLeft, bottomRight);
                    if (ImGui::IsMouseHoveringRect(upperLeft, bottomRight) && s_timelineFocused) {
                        PopStyleVars();
                        if (ImGui::BeginTooltip()) {
                            std::any dynamicFramebuffer = framebuffer;
                            Dispatchers::DispatchString(dynamicFramebuffer);
                            ImGui::EndTooltip();
                        }
                        PushStyleVars();
                    }
                    ImGui::SetCursorPos(reservedCursor);
                }
            }

            {
                ImVec2 reservedCursor = ImGui::GetCursorPos();
                ImGui::SetCursorPos(buttonCursor);
                ImRect bb(ImGui::GetCursorScreenPos(), ImGui::GetCursorScreenPos() + buttonSize);
                ImVec2 baseCursor = buttonCursor;
                ImVec2 size = buttonSize;
                auto& style = ImGui::GetStyle();
                ImVec2 label_size = ImGui::CalcTextSize(compositionLabel.c_str());

                ImVec2 labelCursor = {
                    baseCursor.x + size.x / 2.0f - label_size.x / 2.0f,
                    baseCursor.y + size.y / 2.0f - label_size.y / 2.0f
                };
                if (labelCursor.x - ImGui::GetScrollX() <= style.FramePadding.x + bundlePreviewSize.x) {
                    labelCursor.x = ImGui::GetScrollX() + style.FramePadding.x + bundlePreviewSize.x;
                }
                if (labelCursor.x - ImGui::GetScrollX() >= ImGui::GetWindowSize().x - style.FramePadding.x - label_size.x) {
                    labelCursor.x = ImGui::GetScrollX() + ImGui::GetWindowSize().x - style.FramePadding.x - label_size.x;
                }

                ImGui::PushClipRect(bb.Min, bb.Max, true);
                ImGui::SetCursorPos(labelCursor);
                ImGui::Text("%s", compositionLabel.c_str());
                ImGui::SetCursorPos(reservedCursor);
                ImGui::PopClipRect();
            }

            if ((MouseHoveringBounds(forwardBoundsDrag) || s_forwardBoundsDrag.isActive) && !s_timelineRulerDragged && !s_layerDrag.isActive && !s_backwardBoundsDrag.isActive && !UIShared::s_timelineAnykeyframeDragged) {
                s_forwardBoundsDrag.Activate();
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                auto modifiedSelectedCompositions = selectedCompositions;
                for (auto& id : selectedCompositions) {
                    auto compositionCandidate = Workspace::GetCompositionByID(id);
                    if (!compositionCandidate) continue;
                    auto& composition = *compositionCandidate;
                    if (composition->lockedCompositionID > 0) {
                        modifiedSelectedCompositions.push_back(composition->lockedCompositionID);
                    }
                }

                for (auto& selectedComposoitionID : modifiedSelectedCompositions) {
                    auto selectedCompositionCandidate = Workspace::GetCompositionByID(selectedComposoitionID);
                    if (selectedCompositionCandidate.has_value()) {
                        auto& selectedComposition = selectedCompositionCandidate.value();
                        selectedComposition->OnTimelineSeek();

                        float boundsDragDistance;
                        if (s_forwardBoundsDrag.GetDragDistance(boundsDragDistance)) {
                            selectedComposition->endFrame += boundsDragDistance / s_pixelsPerFrame;
                        } else s_forwardBoundsDrag.Deactivate();

                        float scrollAmount = ProcessLayerScroll();
                        selectedComposition->endFrame += scrollAmount / s_pixelsPerFrame;
                    }
                }
            } else s_forwardBoundsDrag.Deactivate();

            if ((MouseHoveringBounds(backwardBoundsDrag) || s_backwardBoundsDrag.isActive) && !s_timelineRulerDragged && !s_layerDrag.isActive && !s_forwardBoundsDrag.isActive && !UIShared::s_timelineAnykeyframeDragged && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                s_backwardBoundsDrag.Activate();
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                auto modifiedSelectedCompositions = selectedCompositions;
                for (auto& id : selectedCompositions) {
                    auto compositionCandidate = Workspace::GetCompositionByID(id);
                    if (!compositionCandidate) continue;
                    auto& composition = *compositionCandidate;
                    if (composition->lockedCompositionID > 0) {
                        modifiedSelectedCompositions.push_back(composition->lockedCompositionID);
                    }
                }

                for (auto& selectedComposoitionID : modifiedSelectedCompositions) {
                    auto selectedCompositionCandidate = Workspace::GetCompositionByID(selectedComposoitionID);
                    if (selectedCompositionCandidate.has_value()) {
                        auto& selectedComposition = selectedCompositionCandidate.value();
                        selectedComposition->OnTimelineSeek();
                        float boundsDragDistance;
                        if (s_backwardBoundsDrag.GetDragDistance(boundsDragDistance)) {
                            selectedComposition->beginFrame += boundsDragDistance / s_pixelsPerFrame;
                        } else s_backwardBoundsDrag.Deactivate();

                        float scrollAmount = ProcessLayerScroll();
                        selectedComposition->beginFrame += scrollAmount / s_pixelsPerFrame;
                    }
                }
            } else s_backwardBoundsDrag.Deactivate();
 
            if ((compositionHovered || s_layerDrag.isActive) && !s_timelineRulerDragged && !s_backwardBoundsDrag.isActive && !s_forwardBoundsDrag.isActive && !UIShared::s_timelineAnykeyframeDragged && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                s_layerDrag.Activate();

                float layerDragDistance;
                if (s_layerDrag.GetDragDistance(layerDragDistance) && !s_timelineRulerDragged) {
                    auto modifiedSelectedCompositions = selectedCompositions;
                    for (auto& id : selectedCompositions) {
                        auto compositionCandidate = Workspace::GetCompositionByID(id);
                        if (!compositionCandidate) continue;
                        auto& composition = *compositionCandidate;
                        if (composition->lockedCompositionID > 0) {
                            modifiedSelectedCompositions.push_back(composition->lockedCompositionID);
                        }
                    }
                    for (auto& selectedComposoitionID : modifiedSelectedCompositions) {
                        bool breakDrag = false;
                        for (auto& testingCompositionID : modifiedSelectedCompositions) {
                            auto selectedCompositionCandidate = Workspace::GetCompositionByID(testingCompositionID);
                            if (selectedCompositionCandidate.has_value()) {
                                auto& testingComposition = selectedCompositionCandidate.value();
                                if (testingComposition->beginFrame <= 0) {
                                    breakDrag = true;
                                    break;
                                }
                            }
                        }
                        if (breakDrag && modifiedSelectedCompositions.size() > 1 && layerDragDistance < 0) break;
                        auto selectedCompositionCandidate = Workspace::GetCompositionByID(selectedComposoitionID);
                        if (selectedCompositionCandidate.has_value() && ImGui::IsWindowFocused()) {
                            auto& selectedComposition = selectedCompositionCandidate.value();
                            selectedComposition->OnTimelineSeek();
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
            } else s_layerDrag.Deactivate();

            s_anyLayerDragged = (s_anyLayerDragged || s_layerDrag.isActive || s_backwardBoundsDrag.isActive || s_forwardBoundsDrag.isActive) && ImGui::IsMouseDragging(ImGuiMouseButton_Left);

            if (compositionHovered && ImGui::GetIO().MouseDoubleClicked[ImGuiMouseButton_Left]) {
                ImGui::OpenPopup(FormatString("##renameComposition%i", t_id).c_str());
            }
            static bool renameFieldFocued = false;
            PopStyleVars();
            if (ImGui::BeginPopup(FormatString("##renameComposition%i", t_id).c_str())) {
                if (!renameFieldFocued) {
                    ImGui::SetKeyboardFocusHere(0);
                    renameFieldFocued = true;
                }
                ImGui::InputTextWithHint("##renameField", FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("COMPOSITION_NAME").c_str()).c_str(), &composition->name);
                if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            } else renameFieldFocued = false;
            PushStyleVars();

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
            DrawRect(separatorBounds, ImVec4(0, 0, 0, 1));
            SetDrawListChannel(TimelineChannels::Compositions);

            for (auto& attribute : composition->attributes) {
                ImGui::SetCursorPosY(s_attributeYCursors[attribute->id]);
                PopStyleVars();
                if (s_attributesExpanded.find(t_id) != s_attributesExpanded.end() && s_attributesExpanded[t_id]) {
                    attribute->RenderKeyframes();
                }
                PushStyleVars();
            }

            if (mustDeleteComposition) {
                int compositionIndex = 0;
                for (auto& selectedComposoition : selectedCompositions) {
                    if (selectedComposoition == t_id) break;
                    compositionIndex++;
                }
                selectedCompositions.erase(selectedCompositions.begin() + compositionIndex);
            }
        }
    }

    void TimelineUI::DeleteComposition(Composition* composition) {
        auto& project = Workspace::s_project.value();
        
        auto selectedCompositionIterator = std::find(project.selectedCompositions.begin(), project.selectedCompositions.end(), composition->id);
        if (selectedCompositionIterator != project.selectedCompositions.end()) {
            project.selectedCompositions.erase(selectedCompositionIterator);
        }

        Workspace::DeleteComposition(composition->id);
    }

    void TimelineUI::AppendSelectedCompositions(Composition* composition) {
        auto& project = Workspace::GetProject();
        auto& selectedCompositions = project.selectedCompositions;
        if (std::find(selectedCompositions.begin(), selectedCompositions.end(), composition->id) == selectedCompositions.end()) {
            selectedCompositions.push_back(composition->id);
        }
        UIShared::s_lastClickedObjectType = LastClickedObjectType::Composition;
    }

    void TimelineUI::RenderCompositionPopup(Composition* t_composition, ImGuiID t_parentTreeID) {
        auto& project = Workspace::GetProject();
        auto& selectedCompositions = project.selectedCompositions;
        ImGui::SeparatorText(FormatString("%s %s", ICON_FA_LAYER_GROUP, t_composition->name.c_str()).c_str());
        if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_PLUS, Localization::GetString("ADD_ATTRIBUTE").c_str()).c_str())) {
            RenderNewAttributePopup(t_composition, t_parentTreeID);
            ImGui::EndMenu();
        }
        std::string blendingPreviewText = "";
        auto modeCandidate = Compositor::s_blending.GetModeByCodeName(t_composition->blendMode);
        if (modeCandidate.has_value()) {
            blendingPreviewText = "(" + modeCandidate.value().name + ")";
        }
        if (ImGui::BeginMenu(FormatString("%s %s %s", ICON_FA_SLIDERS, Localization::GetString("BLENDING").c_str(), blendingPreviewText.c_str()).c_str())) {
            ImGui::SeparatorText(FormatString("%s %s", ICON_FA_SLIDERS, Localization::GetString("BLENDING").c_str()).c_str());
            static std::string blendFilter = "";
            bool attributeOpacityUsed = false;
            bool correctOpacityTypeUsed = false;
            float opacity = t_composition->GetOpacity(&attributeOpacityUsed, &correctOpacityTypeUsed);
            std::string attributeSelectorText = ICON_FA_LINK;
            if (attributeOpacityUsed) {
                if (correctOpacityTypeUsed) attributeSelectorText = ICON_FA_CHECK " " ICON_FA_LINK;
                else attributeSelectorText = ICON_FA_TRIANGLE_EXCLAMATION " " ICON_FA_LINK;
            }
            static float searchBarWidth = 30;
            static ImVec2 fullWindowSize = ImGui::GetWindowSize();
            ImGui::BeginChild("##opacityContainer", ImVec2(fullWindowSize.x, 0), ImGuiChildFlags_AutoResizeY);
            if (ImGui::Button(attributeSelectorText.c_str())) {
                ImGui::OpenPopup("##opacityAttributeChooser");
            } 
            ImGui::SetItemTooltip("%s %s", ICON_FA_DROPLET, Localization::GetString("OPACITY_ATTRIBUTE").c_str());
            if (attributeOpacityUsed && !correctOpacityTypeUsed) {
                ImGui::SetItemTooltip("%s %s", ICON_FA_TRIANGLE_EXCLAMATION, Localization::GetString("BAD_OPACITY_ATTRIBUTE").c_str());
            } else {
                ImGui::SetItemTooltip("%s %s", ICON_FA_LINK, Localization::GetString("OPACITY_ATTRIBUTE").c_str());
            }
            if (ImGui::BeginPopup("##opacityAttributeChooser")) {
                ImGui::SeparatorText(FormatString("%s %s", ICON_FA_LINK, Localization::GetString("OPACITY_ATTRIBUTE").c_str()).c_str());
                static std::string attributeFilter = "";
                ImGui::InputTextWithHint("##attributeSearchFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &attributeFilter);
                if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_XMARK, Localization::GetString("NO_ATTRIBUTE").c_str()).c_str())) {
                    t_composition->opacityAttributeID = -1;
                    Rendering::ForceRenderFrame();
                }
                for (auto& attribute : t_composition->attributes) {
                    if (!attributeFilter.empty() && attribute->name.find(attributeFilter) == std::string::npos) continue;
                    ImGui::PushID(attribute->id);
                        if (ImGui::MenuItem(FormatString("%s%s %s", t_composition->opacityAttributeID == attribute->id ? ICON_FA_CHECK " " : "", ICON_FA_LINK, attribute->name.c_str()).c_str())) {
                            t_composition->opacityAttributeID = attribute->id;
                            Rendering::ForceRenderFrame();
                        } 
                    ImGui::PopID();
                }
                ImGui::EndPopup();
            }
            ImGui::SameLine(0, 2.0f);
            ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
            ImGui::SliderFloat("##compositionOpacity", &opacity, 0, 1);
            if (ImGui::IsItemEdited()) Rendering::ForceRenderFrame();
            ImGui::SetItemTooltip("%s %s", ICON_FA_DROPLET, Localization::GetString("COMPOSITION_OPACITY").c_str());
            ImGui::PopItemWidth();
            if (!attributeOpacityUsed) t_composition->opacity = opacity;
            ImGui::EndChild();
            ImGui::InputTextWithHint("##blendFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &blendFilter);
            ImGui::SameLine(0, 0);
            searchBarWidth = ImGui::GetCursorPosX();
            ImGui::NewLine();
            ImGui::BeginChild("##blendCandidates", ImVec2(0, RASTER_PREFERRED_POPUP_HEIGHT));
                auto& blending = Compositor::s_blending;
                if (ImGui::MenuItem(FormatString("%s %s Normal", t_composition->blendMode.empty() ? ICON_FA_CHECK : "", ICON_FA_DROPLET).c_str())) {
                    t_composition->blendMode = "";
                    Rendering::ForceRenderFrame();
                }
                for (auto& mode : blending.modes) {
                    auto& project = Workspace::s_project.value();
                    if (!blendFilter.empty() && mode.name.find(blendFilter) == std::string::npos) continue;
                    if (ImGui::MenuItem(FormatString("%s %s %s", t_composition->blendMode == mode.codename ? ICON_FA_CHECK : "", Font::GetIcon(mode.icon).c_str(), mode.name.c_str()).c_str())) {
                        t_composition->blendMode = mode.codename;
                        Rendering::ForceRenderFrame();
                    }
                    if (ImGui::BeginItemTooltip()) {
                        auto& bundles = Compositor::s_bundles;
                        if (!IsInBounds(project.currentFrame, t_composition->beginFrame, t_composition->endFrame) || bundles.GetFrontValue().find(t_composition->id) == bundles.GetFrontValue().end()) {
                            ImGui::Text("%s %s", ICON_FA_TRIANGLE_EXCLAMATION, Localization::GetString("BLENDING_PREVIEW_IS_UNAVAILABLE").c_str());
                        } else {
                            // FIXME: restore blending preview
                            auto& bundle = bundles.GetFrontValue()[t_composition->id];
                            auto requiredResolution = Compositor::GetRequiredResolution();
                            static Framebuffer previewFramebuffer;
                            static Pipeline s_compositorPipeline =
                                GPU::GeneratePipeline(
                                    GPU::GenerateShader(ShaderType::Vertex,
                                                        "compositor/shader"),
                                    GPU::GenerateShader(ShaderType::Fragment,
                                                        "compositor/shader"));
                            Compositor::EnsureResolutionConstraintsForFramebuffer(previewFramebuffer);
                            GPU::BindFramebuffer(previewFramebuffer);
                            GPU::ClearFramebuffer(project.backgroundColor.r, project.backgroundColor.g, project.backgroundColor.b, project.backgroundColor.a);
                            int compositionIndex = 0;
                            for (auto& candidate : project.compositions) {
                                if (candidate.id == t_composition->id) break;
                                compositionIndex++;
                            }
                            std::vector<int> allowedCompositions;
                            for (int i = 0; i < compositionIndex; i++) {
                                allowedCompositions.push_back(project.compositions[i].id);
                            }
                            GPU::BindFramebuffer(previewFramebuffer);
                            GPU::ClearFramebuffer(project.backgroundColor.r, project.backgroundColor.g, project.backgroundColor.b, project.backgroundColor.a);
                            static Blending s_previewBlending = Blending(Compositor::s_blending.Serialize());
                            static bool s_blendingPipelineGenerated = false;
                            if (!s_blendingPipelineGenerated) {
                                s_previewBlending.GenerateBlendingPipeline();
                                s_blendingPipelineGenerated = true;
                            }

                            auto targets = Compositor::s_targets.Get();
                            std::vector<CompositorTarget> filteredTargets;
                            for (auto& target : targets) {
                                if (std::find(allowedCompositions.begin(), allowedCompositions.end(), target.compositionID) != allowedCompositions.end()) {
                                    filteredTargets.push_back(target);
                                }
                            }

                            for (auto& target : targets) {
                                if (target.compositionID == t_composition->id) {
                                    auto modifiedTarget = target;
                                    modifiedTarget.blendMode = mode.codename;
                                    filteredTargets.push_back(modifiedTarget);
                                    break;
                                }
                            }

                            Compositor::PerformManualComposition(filteredTargets, previewFramebuffer, project.backgroundColor, s_previewBlending, s_compositorPipeline);

                            auto previewSize = FitRectInRect({128, 128}, {previewFramebuffer.width, previewFramebuffer.height});
                            auto blendingInfoString = FormatString("%s %s (%s)", Font::GetIcon(mode.icon).c_str(), mode.name.c_str(), mode.codename.c_str());
                            ImGui::Image((ImTextureID) previewFramebuffer.attachments[0].handle, {previewSize.x, previewSize.y}); 
                            ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - ImGui::CalcTextSize(blendingInfoString.c_str()).x / 2.0f);
                            ImGui::Text("%s", blendingInfoString.c_str());
                        }
                        ImGui::EndTooltip();
                    }
                }
                fullWindowSize = ImGui::GetWindowSize();
            ImGui::EndChild();
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_IMAGE, Localization::GetString("MASK_COMPOSITION").c_str()).c_str())) {
            RenderMaskCompositionPopup(t_composition);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("EDIT_METADATA").c_str()).c_str())) {
            ImGui::SeparatorText(FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("EDIT_METADATA").c_str()).c_str());
            ImGui::InputTextWithHint("##compositionName", FormatString("%s %s", ICON_FA_PENCIL, Localization::GetString("COMPOSITION_NAME").c_str()).c_str(), &t_composition->name);
            ImGui::SetItemTooltip("%s %s", ICON_FA_PENCIL, Localization::GetString("COMPOSITION_NAME").c_str());
            ImGui::InputTextMultiline("##compositionDescription", &t_composition->description);
            ImGui::SetItemTooltip("%s %s", ICON_FA_PENCIL, Localization::GetString("COMPOSITION_DESCRIPTION").c_str());
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu(FormatString("%s %s", t_composition->lockedCompositionID > 0 ? ICON_FA_LOCK : ICON_FA_LOCK_OPEN, Localization::GetString("LOCK_COMPOSITION").c_str()).c_str())) {
            RenderLockCompositionPopup(t_composition);
            ImGui::EndMenu();
        }
        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_ARROW_POINTER, Localization::GetString("SELECT_COMPOSITION").c_str()).c_str())) {
            AppendSelectedCompositions(t_composition);
        }
        if (ImGui::MenuItem(FormatString("%s %s", t_composition->enabled ? ICON_FA_TOGGLE_ON : ICON_FA_TOGGLE_OFF, Localization::GetString("ENABLE_DISABLE_COMPOSITIONS").c_str()).c_str())) {
            for (auto& compositionID : project.selectedCompositions) {
                auto compositionCandidate = Workspace::GetCompositionByID(compositionID);
                if (compositionCandidate.has_value()) {
                    auto& composition = compositionCandidate.value();
                    composition->enabled = !composition->enabled;
                    Rendering::ForceRenderFrame();
                    WaveformManager::RequestWaveformRefresh(composition->id);
                }
            }
        }
        if (ImGui::MenuItem(FormatString("%s %s", t_composition->audioEnabled ? ICON_FA_VOLUME_HIGH : ICON_FA_VOLUME_OFF, Localization::GetString("ENABLE_DISABLE_COMPOSITIONS_AUDIO_MIXING").c_str()).c_str(), "Ctrl+A")) {
            ProcessAudioMixingAction();
        }
        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_COPY, Localization::GetString("COPY_SELECTED_COMPOSITIONS").c_str()).c_str(), "Ctrl+C")) {
            ProcessCopyAction();
        }
        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_PASTE, Localization::GetString("PASTE_SELECTED_COMPOSITIONS").c_str()).c_str(), "Ctrl+V")) {
            ProcessPasteAction();
        }
        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_CLONE, Localization::GetString("DUPLICATE_SELECTED_COMPOSITIONS").c_str()).c_str(), "Ctrl+D")) {
            ProcessCopyAction();
            ProcessPasteAction();
        }
        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_SCISSORS, Localization::GetString("CUT_COMPOSITIONS").c_str()).c_str())) {
            ProcessCutAction();
        }
        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_TRASH_CAN, Localization::GetString("DELETE_SELECTED_COMPOSITIONS").c_str()).c_str(), "Delete")) {
            ProcessDeleteAction();
        }
        ImGui::Separator();   
        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_LOCK_OPEN, Localization::GetString("UNLOCK_COMPOSITION").c_str()).c_str(), nullptr, false, t_composition->lockedCompositionID > 0)) {
            t_composition->lockedCompositionID = -1;
        }     
        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_FORWARD, Localization::GetString("RESIZE_TO_MATCH_CONTENT_DURATION").c_str()).c_str(), "Ctrl+R")) {
            ProcessResizeToMatchContentDurationAction();
        }
        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_WAVE_SQUARE, Localization::GetString("RECOMPUTE_AUDIO_WAVEFORM").c_str()).c_str())) {
            ProcessRecomputeAudioWaveformAction();
        }
    }

    void TimelineUI::ProcessCopyAction() {
        std::vector<Composition> copiedCompositions;
        auto& project = Workspace::GetProject();
        for (auto& selectedComposition : project.selectedCompositions) {
            auto compositionCandidate = Workspace::CopyComposition(selectedComposition);
            if (compositionCandidate.has_value()) {
                copiedCompositions.push_back(*compositionCandidate);
            }
        }
        s_copyCompositions = copiedCompositions;
    }

    void TimelineUI::ProcessCutAction() {
        if (!Workspace::IsProjectLoaded()) return;
        auto& project = Workspace::GetProject();
        for (auto& compositionID : project.selectedCompositions) {
            auto baseCompositionCandidate = Workspace::GetCompositionByID(compositionID);
            if (!baseCompositionCandidate) continue;
            auto& baseComposition = *baseCompositionCandidate;
            if (!IsInBounds(project.currentFrame, baseComposition->beginFrame - 1, baseComposition->endFrame + 1)) continue;
            auto copiedCompositionCandidate = Workspace::CopyComposition(compositionID);
            if (!copiedCompositionCandidate) continue;
            auto& copiedComposition = *copiedCompositionCandidate;
            baseComposition->lockedCompositionID = 0;
            baseComposition->endFrame = project.currentFrame;
            copiedComposition.beginFrame = project.currentFrame;
            copiedComposition.cutTimeOffset += baseComposition->endFrame - baseComposition->beginFrame;
            copiedComposition.lockedCompositionID = 0;
            int baseCompositionIndex = 0;
            bool compositionIndexFound = false;
            for (auto& composition : project.compositions) {
                if (composition.id == baseComposition->id) {
                    compositionIndexFound = true;
                    break;
                }
                baseCompositionIndex++;
            }
            if (compositionIndexFound) {
                project.compositions.insert(project.compositions.begin() + baseCompositionIndex + 1, copiedComposition);
                WaveformManager::RequestWaveformRefresh(copiedComposition.id);
            }
        }
    }

    void TimelineUI::ProcessPasteAction() {
        auto& project = Workspace::s_project.value();
        for (auto& composition : s_copyCompositions) {
            project.compositions.push_back(composition);
            WaveformManager::RequestWaveformRefresh(composition.id);
        }
        Rendering::ForceRenderFrame();
    }

    void TimelineUI::ProcessDeleteAction() {
        auto& project = Workspace::GetProject();
        auto selectedCompositionsCopy = project.selectedCompositions;
        for (auto& compositionID : selectedCompositionsCopy) {
            auto compositionCandidate = Workspace::GetCompositionByID(compositionID);
            if (compositionCandidate.has_value()) {
                DeleteComposition(compositionCandidate.value());
            }
            WaveformManager::EraseRecord(compositionID);
        }
        Rendering::ForceRenderFrame();
    }

    void TimelineUI::ProcessResizeToMatchContentDurationAction() {
        auto& project = Workspace::GetProject();
        Rendering::ForceRenderFrame();
        for (auto& compositionID : project.selectedCompositions) {
            auto compositionCandidate = Workspace::GetCompositionByID(compositionID);
            if (compositionCandidate.value()) {
                auto& composition = compositionCandidate.value();
                WaveformManager::RequestWaveformRefresh(composition->id);
                std::optional<float> contentDuration = std::nullopt;
                for (auto& node : composition->nodes) {
                    auto durationCandidate = node.second->GetContentDuration();
                    if (!contentDuration.has_value() && durationCandidate.has_value()) {
                        contentDuration = durationCandidate;
                    }
                    if (contentDuration.has_value() && durationCandidate.has_value()) {
                        auto& duration = durationCandidate.value();
                        auto& currentDuration = contentDuration.value();
                        if (duration > currentDuration) {
                            duration = currentDuration;
                        }
                    }
                }
                if (contentDuration.has_value()) {
                    auto duration = contentDuration.value();
                    composition->endFrame = composition->beginFrame + duration;
                }
            }
        }
    }

    void TimelineUI::ProcessRecomputeAudioWaveformAction() {
        auto& project = Workspace::GetProject();
        for (auto& compositionID : project.selectedCompositions) {
            WaveformManager::RequestWaveformRefresh(compositionID);
        }
        Rendering::ForceRenderFrame();
    }

    void TimelineUI::ProcessAudioMixingAction() {
        auto& project = Workspace::GetProject();
        for (auto& compositionID : project.selectedCompositions) {
            auto compositionCandidate = Workspace::GetCompositionByID(compositionID);
            if (compositionCandidate.value()) {
                auto& composition = compositionCandidate.value();
                composition->audioEnabled = !composition->audioEnabled;
                WaveformManager::RequestWaveformRefresh(compositionID);
            }
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

    void TimelineUI::RenderNewAttributePopup(Composition* t_composition, ImGuiID t_parentTreeID) {
        ImGui::SeparatorText(FormatString("%s %s", ICON_FA_PLUS, Localization::GetString("ADD_ATTRIBUTE").c_str()).c_str());
        auto& project = Workspace::GetProject();
        for (auto& entry : Attributes::s_implementations) {
            if (ImGui::MenuItem(FormatString("%s %s %s", ICON_FA_PLUS, entry.description.prettyName.c_str(), Localization::GetString("ATTRIBUTE").c_str()).c_str())) {
                auto attributeCandidate = Attributes::InstantiateAttribute(entry.description.packageName);
                if (attributeCandidate.has_value()) {
                    t_composition->attributes.push_back(attributeCandidate.value());
                    Rendering::ForceRenderFrame();
                    if (!t_parentTreeID && s_compositionTrees.find(t_composition->id) != s_compositionTrees.end()) {
                        t_parentTreeID = s_compositionTrees[t_composition->id];
                    }
                    if (t_parentTreeID) {
                        s_legendTargetOpenTree = t_parentTreeID;
                        project.selectedAttributes = {attributeCandidate.value()->id};
                        if (s_compositionTreeScrolls.find(t_composition->id) != s_compositionTreeScrolls.end()) {
                            s_targetLegendScroll = s_compositionTreeScrolls[t_composition->id];
                        }
                    }
                }
            }
        }
    }

    void TimelineUI::RenderMaskCompositionPopup(Composition *t_composition) {
        auto& project = Workspace::GetProject();
        ImGui::SeparatorText(FormatString("%s %s", ICON_FA_IMAGE, Localization::GetString("MASK_COMPOSITION").c_str()).c_str());
        static std::string s_searchFilter = "";
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputTextWithHint("##searchFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &s_searchFilter);
        ImGui::PopItemWidth();
        if (ImGui::BeginChild("##maskCandidates", ImVec2(ImGui::GetContentRegionAvail().x, RASTER_PREFERRED_POPUP_HEIGHT))) {
            bool hasCandidates = false;
            int targetRemoveMask = -1;
            int maskIndex = 0;
            for (auto& mask : t_composition->masks) {
                maskIndex++;
                if (mask.compositionID == t_composition->id) continue;
                auto compositionCandidate = Workspace::GetCompositionByID(mask.compositionID);
                if (!compositionCandidate) continue;
                auto& composition = *compositionCandidate;
                if (!s_searchFilter.empty() && LowerCase(composition->name).find(LowerCase(s_searchFilter)) == std::string::npos) continue;
                ImGui::PushID(mask.compositionID);
                if (ImGui::SmallButton(ICON_FA_TRASH_CAN)) {
                    targetRemoveMask = maskIndex - 1;
                }
                ImGui::SetItemTooltip("%s %s", ICON_FA_TRASH_CAN, Localization::GetString("DELETE_MASK").c_str());
                ImGui::SameLine();
                if (ImGui::SmallButton(mask.precompose ? ICON_FA_IMAGE " " ICON_FA_TOGGLE_ON : ICON_FA_IMAGE " " ICON_FA_TOGGLE_OFF)) {
                    mask.precompose = !mask.precompose;
                    Rendering::ForceRenderFrame();
                }
                ImGui::SetItemTooltip("%s %s", mask.precompose ? ICON_FA_IMAGE " " ICON_FA_TOGGLE_ON : ICON_FA_IMAGE " " ICON_FA_TOGGLE_OFF, Localization::GetString("MASK_PRECOMPOSITION").c_str());
                ImGui::SameLine();
                static const char* s_maskSigns[] = {
                    ICON_FA_DROPLET, ICON_FA_PLUS, ICON_FA_MINUS, ICON_FA_XMARK, ICON_FA_DIVIDE
                };
                static std::string s_maskStrings[] = {
                    Localization::GetString("NORMAL"),
                    Localization::GetString("ADD"), Localization::GetString("SUBTRACT"),
                    Localization::GetString("MULTIPLY"), Localization::GetString("DIVIDE")
                };
                if (ImGui::SmallButton(s_maskSigns[static_cast<int>(mask.op)])) {
                    ImGui::OpenPopup("##maskOperationChooser");
                }
                if (ImGui::BeginPopup("##maskOperationChooser")) {
                    ImGui::SeparatorText(FormatString("%s %s", s_maskSigns[static_cast<int>(mask.op)], Localization::GetString("MASK_OPERATION").c_str()).c_str());
                    if (ImGui::MenuItem(FormatString("%s%s %s", mask.op == MaskOperation::Normal ? ICON_FA_CHECK " " : "", ICON_FA_DROPLET, Localization::GetString("NORMAL").c_str()).c_str())) {
                        mask.op = MaskOperation::Normal;
                        Rendering::ForceRenderFrame();
                    }
                    if (ImGui::MenuItem(FormatString("%s%s %s", mask.op == MaskOperation::Add ? ICON_FA_CHECK " " : "", ICON_FA_PLUS, Localization::GetString("ADD").c_str()).c_str())) {
                        mask.op = MaskOperation::Add;
                        Rendering::ForceRenderFrame();
                    }
                    if (ImGui::MenuItem(FormatString("%s%s %s", mask.op == MaskOperation::Subtract ? ICON_FA_CHECK " " : "", ICON_FA_MINUS, Localization::GetString("SUBTRACT").c_str()).c_str())) {
                        mask.op = MaskOperation::Subtract;
                        Rendering::ForceRenderFrame();
                    }
                    if (ImGui::MenuItem(FormatString("%s%s %s", mask.op == MaskOperation::Multiply ? ICON_FA_CHECK " " : "", ICON_FA_XMARK, Localization::GetString("MULTIPLY").c_str()).c_str())) {
                        mask.op = MaskOperation::Multiply;
                        Rendering::ForceRenderFrame();
                    }
                    if (ImGui::MenuItem(FormatString("%s%s %s", mask.op == MaskOperation::Divide ? ICON_FA_CHECK " " : "", ICON_FA_DIVIDE, Localization::GetString("DIVIDE").c_str()).c_str())) {
                        mask.op = MaskOperation::Divide;
                        Rendering::ForceRenderFrame();
                    }
                    ImGui::EndPopup();
                }
                ImGui::SameLine();
                if (ImGui::Selectable(FormatString("%s %s", ICON_FA_LAYER_GROUP, composition->name.c_str()).c_str())) {
                    ImGui::OpenPopup("##maskProperties");
                }
#define COMPOSITION_MASK_DRAG_DROP "COMPOSITION_MASK_DRAG_DROP"
                if (ImGui::BeginDragDropSource()) {
                    ImGui::SetDragDropPayload(COMPOSITION_MASK_DRAG_DROP, &mask, sizeof(&mask));
                    ImGui::EndDragDropSource();
                }
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(COMPOSITION_MASK_DRAG_DROP)) {
                        CompositionMask* anotherMask = *((CompositionMask**) payload->Data);
                        std::swap(*anotherMask, mask);
                    }
                    ImGui::EndDragDropTarget();
                }
                if (ImGui::BeginPopup("##maskProperties")) {
                    ImGui::SeparatorText(FormatString("%s %s", ICON_FA_LAYER_GROUP, composition->name.c_str()).c_str());
                    if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_LAYER_GROUP, Localization::GetString("MASK_SOURCE").c_str()).c_str())) {
                        ImGui::SeparatorText(FormatString("%s %s", ICON_FA_LAYER_GROUP, Localization::GetString("MASK_SOURCE").c_str()).c_str());
                        static std::string s_compositionFilter = "";
                        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                        ImGui::InputTextWithHint("##searchFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &s_compositionFilter);
                        ImGui::PopItemWidth();
                        if (ImGui::BeginChild("##changeMasksCandidates", ImVec2(ImGui::GetContentRegionAvail().x, RASTER_PREFERRED_POPUP_HEIGHT))) {
                            bool hasCompositionCandidates = false;
                            for (auto& newComposition : project.compositions) {
                                if (!s_compositionFilter.empty() && LowerCase(newComposition.name).find(LowerCase(s_compositionFilter)) == std::string::npos) continue;
                                if (newComposition.id == mask.compositionID) continue;
                                if (newComposition.id == t_composition->id) continue;
                                if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_LAYER_GROUP, newComposition.name.c_str()).c_str())) {
                                    mask.compositionID = newComposition.id;
                                    Rendering::ForceRenderFrame();
                                    ImGui::CloseCurrentPopup();
                                }
                                hasCompositionCandidates = true;
                            }
                            if (!hasCompositionCandidates) UIHelpers::RenderNothingToShowText();
                        }
                        ImGui::EndChild();
                        ImGui::EndMenu();
                    }
                    if (ImGui::BeginMenu(FormatString("%s %s: %s", s_maskSigns[static_cast<int>(mask.op)], Localization::GetString("MASK_OPERATION").c_str(), s_maskStrings[static_cast<int>(mask.op)].c_str()).c_str())) {
                        ImGui::SeparatorText(FormatString("%s %s", s_maskSigns[static_cast<int>(mask.op)], Localization::GetString("MASK_OPERATION").c_str()).c_str());
                        if (ImGui::MenuItem(FormatString("%s%s %s", mask.op == MaskOperation::Normal ? ICON_FA_CHECK " " : "", ICON_FA_DROPLET, Localization::GetString("NORMAL").c_str()).c_str())) {
                            mask.op = MaskOperation::Normal;
                            Rendering::ForceRenderFrame();
                        }
                        if (ImGui::MenuItem(FormatString("%s%s %s", mask.op == MaskOperation::Add ? ICON_FA_CHECK " " : "", ICON_FA_PLUS, Localization::GetString("ADD").c_str()).c_str())) {
                            mask.op = MaskOperation::Add;
                            Rendering::ForceRenderFrame();
                        }
                        if (ImGui::MenuItem(FormatString("%s%s %s", mask.op == MaskOperation::Subtract ? ICON_FA_CHECK " " : "", ICON_FA_MINUS, Localization::GetString("SUBTRACT").c_str()).c_str())) {
                            mask.op = MaskOperation::Subtract;
                            Rendering::ForceRenderFrame();
                        }
                        if (ImGui::MenuItem(FormatString("%s%s %s", mask.op == MaskOperation::Multiply ? ICON_FA_CHECK " " : "", ICON_FA_XMARK, Localization::GetString("MULTIPLY").c_str()).c_str())) {
                            mask.op = MaskOperation::Multiply;
                            Rendering::ForceRenderFrame();
                        }
                        if (ImGui::MenuItem(FormatString("%s%s %s", mask.op == MaskOperation::Divide ? ICON_FA_CHECK " " : "", ICON_FA_DIVIDE, Localization::GetString("DIVIDE").c_str()).c_str())) {
                            mask.op = MaskOperation::Divide;
                            Rendering::ForceRenderFrame();
                        }
                        ImGui::EndMenu();
                    }
                    if (ImGui::MenuItem(FormatString("%s %s %s", ICON_FA_IMAGE, mask.precompose ? ICON_FA_TOGGLE_ON : ICON_FA_TOGGLE_OFF, Localization::GetString("ENABLE_DISABLE_MASK_PRECOMPOSITION").c_str()).c_str())) {
                        mask.precompose = !mask.precompose;
                        Rendering::ForceRenderFrame();
                    }
                    if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_TRASH_CAN, Localization::GetString("DELETE_MASK").c_str()).c_str())) {
                        targetRemoveMask = maskIndex - 1;
                    }
                    ImGui::EndPopup();
                }
                hasCandidates = true;
                ImGui::PopID();
            }
            if (targetRemoveMask >= 0) {
                t_composition->masks.erase(t_composition->masks.begin() + targetRemoveMask);
                Rendering::ForceRenderFrame();
            }
            if (!hasCandidates) UIHelpers::RenderNothingToShowText();
            ImGui::Separator();
            if (UIHelpers::CenteredButton(FormatString("%s %s", ICON_FA_PLUS, Localization::GetString("ADD_MASK").c_str()).c_str())) {
                ImGui::OpenPopup("##newMask");
            }
            if (ImGui::BeginPopup("##newMask")) {
                ImGui::SeparatorText(FormatString("%s %s", ICON_FA_PLUS, Localization::GetString("ADD_MASK").c_str()).c_str());
                static std::string s_compositionFilter = "";
                ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
                ImGui::InputTextWithHint("##searchFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &s_compositionFilter);
                ImGui::PopItemWidth();
                if (ImGui::BeginChild("##changeMasksCandidates", ImVec2(ImGui::GetContentRegionAvail().x, RASTER_PREFERRED_POPUP_HEIGHT))) {
                    bool hasCompositionCandidates = false;
                    for (auto& newComposition : project.compositions) {
                        if (!s_compositionFilter.empty() && LowerCase(newComposition.name).find(LowerCase(s_compositionFilter)) == std::string::npos) continue;
                        if (newComposition.id == t_composition->id) continue;
                        bool skip = false;
                        for (auto& currentMask : t_composition->masks) {
                            if (currentMask.compositionID == newComposition.id) {
                                skip = true;
                                break;
                            }
                        }
                        if (skip) continue;
                        if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_LAYER_GROUP, newComposition.name.c_str()).c_str())) {
                            t_composition->masks.push_back(CompositionMask(newComposition.id, MaskOperation::Add));
                            ImGui::CloseCurrentPopup();
                        }
                        hasCompositionCandidates = true;
                    }
                    if (!hasCompositionCandidates) UIHelpers::RenderNothingToShowText();
                }
                ImGui::EndChild();
                ImGui::EndPopup();
            }
        }
        ImGui::EndChild();
    }

    void TimelineUI::RenderLockCompositionPopup(Composition *t_composition) {
        auto& project = Workspace::GetProject();
        ImGui::SeparatorText(FormatString("%s %s", t_composition->lockedCompositionID > 0 ? ICON_FA_LOCK : ICON_FA_LOCK_OPEN, Localization::GetString("LOCK_COMPOSITION").c_str()).c_str());
        static std::string s_searchFilter = "";
        ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
        ImGui::InputTextWithHint("##searchFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &s_searchFilter);
        ImGui::PopItemWidth();
        if (ImGui::BeginChild("##compositionCandidates", ImVec2(ImGui::GetContentRegionAvail().x, RASTER_PREFERRED_POPUP_HEIGHT))) {
            if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_LOCK_OPEN, Localization::GetString("NO_LOCK").c_str()).c_str())) {
                t_composition->lockedCompositionID = -1;
                ImGui::CloseCurrentPopup();
            }
            bool hasCandidates = false;
            for (auto& composition : project.compositions) {
                if (composition.id == t_composition->id) continue;
                if (composition.lockedCompositionID == t_composition->id) continue;
                if (!s_searchFilter.empty() && LowerCase(composition.name).find(LowerCase(s_searchFilter)) == std::string::npos) continue;
                if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_LAYER_GROUP, composition.name.c_str()).c_str())) {
                    t_composition->lockedCompositionID = composition.id;
                    ImGui::CloseCurrentPopup();
                }
                hasCandidates = true;
            }
            if (!hasCandidates) {
                UIHelpers::RenderNothingToShowText();
            }
        }
        ImGui::EndChild();
    }

    void TimelineUI::LockCompositionDragSource(Composition *t_composition) {
        if (ImGui::BeginDragDropSource()) {
            ImGui::SetDragDropPayload(LAYER_LOCK_DRAG_DROP, &t_composition->id, sizeof(t_composition->id));
            ImGui::EndDragDropSource();
        }
    }

    void TimelineUI::LockCompositionDragTarget(Composition *t_composition) {
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(LAYER_LOCK_DRAG_DROP)) {
                int compositionID = *((int*) payload->Data);
                t_composition->lockedCompositionID = compositionID;
            }
            ImGui::EndDragDropTarget();
        }
    }

    void TimelineUI::RenderTimelinePopup() {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && !s_layerPopupActive && ImGui::IsWindowFocused() && !UIShared::s_timelineBlockPopup) {
            ImGui::OpenPopup("##layerPopup");
        }
        UIShared::s_timelineBlockPopup = false;
        auto& project = Workspace::GetProject();
        auto& selectedAssets = project.selectedAssets;
        PopStyleVars();
        if (ImGui::BeginPopup("##layerPopup")) {
            ImGui::SeparatorText(FormatString("%s %s", ICON_FA_TIMELINE, Localization::GetString("TIMELINE").c_str()).c_str());
            if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_PLUS, Localization::GetString("NEW_COMPOSITION").c_str()).c_str())) {
                Workspace::s_project.value().compositions.push_back(Composition());
            }
            std::string insertSelectedAssetFormat = FormatString("%s %s", ICON_FA_PLUS, Localization::GetString("INSERT_SELECTED_ASSETS").c_str());
            if (selectedAssets.size() == 1) {
                auto selectedAssetCandidate = Workspace::GetAssetByAssetID(selectedAssets[0]);
                if (selectedAssetCandidate) {
                    auto& selectedAsset = *selectedAssetCandidate;
                    insertSelectedAssetFormat = FormatString("%s %s", ICON_FA_PLUS, FormatString(Localization::GetString("INSERT_SELECTED_ASSET_FORMAT"), selectedAsset->name.c_str()).c_str());
                }
            } else if (selectedAssets.size() > 1) {
                insertSelectedAssetFormat = FormatString("%s %s", ICON_FA_PLUS, FormatString(Localization::GetString("INSERT_SELECTED_ASSETS_FORMAT").c_str(), (int) selectedAssets.size()).c_str());
            }
            if (ImGui::MenuItem(insertSelectedAssetFormat.c_str(), nullptr, false, selectedAssets.size() > 0)) {
                for (auto& assetID : selectedAssets) {
                    auto assetCandidate = Workspace::GetAssetByAssetID(assetID);
                    if (assetCandidate) {
                        auto& asset = *assetCandidate;
                        asset->OnTimelineDrop((ImGui::GetScrollX() + s_timelineMousePos.x) / s_pixelsPerFrame);
                    }
                }
            }
            if (ImGui::BeginMenu(FormatString("%s %s", ICON_FA_PLUS, Localization::GetString("INSERT_ASSET").c_str()).c_str())) {
                ImGui::SeparatorText(FormatString("%s %s", ICON_FA_PLUS, Localization::GetString("INSERT_ASSET").c_str()).c_str());
                static std::string s_searchFilter = "";
                ImGui::InputTextWithHint("##searchFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &s_searchFilter);
                if (ImGui::BeginChild("##assets", ImVec2(0, RASTER_PREFERRED_POPUP_HEIGHT))) {
                    std::function<void(std::vector<AbstractAsset>&)> renderAssets = [&](std::vector<AbstractAsset>& t_assets) {
                        for (auto& asset : t_assets) {
                            if (!s_searchFilter.empty() && LowerCase(asset->name).find(LowerCase(s_searchFilter)) == std::string::npos) continue;
                            auto implementationCandidate = Assets::GetAssetImplementation(asset->packageName);
                            if (!implementationCandidate) continue;
                            auto& implementation = *implementationCandidate;
                            auto childAssetsCandidate = asset->GetChildAssets();
                            ImGui::PushID(asset->id);
                            if (!childAssetsCandidate) {
                                if (ImGui::MenuItem(FormatString("%s %s", implementation.description.icon.c_str(), asset->name.c_str()).c_str())) {
                                    asset->OnTimelineDrop((ImGui::GetScrollX() + s_timelineMousePos.x) / s_pixelsPerFrame);
                                    ImGui::CloseCurrentPopup();
                                }
                            } else {
                                if (ImGui::TreeNode(FormatString("%s %s", implementation.description.icon.c_str(), asset->name.c_str()).c_str())) {
                                    renderAssets(**childAssetsCandidate);
                                    ImGui::TreePop();
                                }
                            }
                            ImGui::PopID();
                        }
                    };
                    renderAssets(project.assets);
                }
                ImGui::EndChild();
                ImGui::EndMenu();
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
        SetDrawListChannel(TimelineChannels::TimelineRuler);
        DrawRect(timelineBounds, ImVec4(1, 0, 0, 1));
        SetDrawListChannel(TimelineChannels::Compositions);

        if (MouseHoveringBounds(timelineBounds)) {
            s_timelineDrag.Activate();
        }

        float timelineDragDistance;
        if (s_timelineDrag.GetDragDistance(timelineDragDistance) && ImGui::IsWindowFocused() && !s_anyLayerDragged && !UIShared::s_timelineAnykeyframeDragged && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            project.currentFrame = GetRelativeMousePos().x / s_pixelsPerFrame;
            project.OnTimelineSeek();
        } else s_timelineDrag.Deactivate();
    }



    void TimelineUI::RenderLegend() {
        Composition* targetCompositionDelete = nullptr;
        if (ImGui::BeginChild("##timelineLegend", ImVec2(ImGui::GetWindowSize().x * s_splitterState, ImGui::GetContentRegionAvail().y), 0, ImGuiWindowFlags_NoScrollbar)) {
            SplitDrawList();
            if (s_targetLegendScroll > 0) {
                s_timelineScrollY = s_targetLegendScroll;
            }
            ImGui::SetScrollY(s_timelineScrollY);
            RenderTicksBar();
            RectBounds backgroundBounds = RectBounds(
                ImVec2(ImGui::GetScrollX(), ImGui::GetScrollY()),
                ImVec2(ImGui::GetWindowSize().x, TICKS_BAR_HEIGHT)
            );
            auto& project = Workspace::s_project.value();
            static ImVec2 infoCentererSize = ImVec2(200, 20);
            ImGui::SetCursorPos(backgroundBounds.size / 2.0f - infoCentererSize / 2.0f);
            if (ImGui::BeginChild("##infoCenterer", ImVec2(0, 0), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
                ImGui::PushFont(Font::s_denseFont);
                ImGui::SetWindowFontScale(1.6f);
                static TimestampFormat timestampFormat = TimestampFormat::Regular;
                if (!project.customData.contains("TimelineTimestampFormat")) {
                    project.customData["TimelineTimestampFormat"] = static_cast<int>(timestampFormat);
                } else {
                    timestampFormat = static_cast<TimestampFormat>(project.customData["TimelineTimestampFormat"]);
                }
                std::string formattedTimestamp = project.FormatFrameToTime(project.currentFrame);
                if (timestampFormat == TimestampFormat::Frame) formattedTimestamp = std::to_string((int) project.currentFrame);
                if (timestampFormat == TimestampFormat::Seconds) formattedTimestamp = FormatString("%.2f", Precision(project.currentFrame / project.framerate, 2));
                ImVec2 timestampSize = ImGui::CalcTextSize(formattedTimestamp.c_str());
                ImGui::SetCursorPos(ImVec2(
                    5,
                    backgroundBounds.size.y / 2.0f - timestampSize.y / 2.0f
                ));
                static bool timestampHovered = false;
                ImVec4 textColor = ImGui::GetStyleColorVec4(ImGuiCol_Text);
                if (timestampHovered) textColor = textColor * 0.8f;
                ImGui::PushStyleColor(ImGuiCol_Text, textColor);
                ImGui::Text("%s", formattedTimestamp.c_str());
                ImGui::PopStyleColor();
                timestampHovered = ImGui::IsItemHovered();
                if (timestampHovered && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                    ImGui::OpenPopup("##timestampFormatPopup");
                }
                PopStyleVars();
                ImGui::PopFont();
                ImGui::SetWindowFontScale(1.0f);
                if (ImGui::BeginPopup("##timestampFormatPopup")) {
                    ImGui::SeparatorText(FormatString("%s %s", ICON_FA_STOPWATCH, Localization::GetString("TIMESTAMP_FORMAT").c_str()).c_str());
                    if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_STOPWATCH, Localization::GetString("REGULAR").c_str()).c_str())) {
                        timestampFormat = TimestampFormat::Regular;
                    }
                    if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_STOPWATCH, Localization::GetString("FRAMES").c_str()).c_str())) {
                        timestampFormat = TimestampFormat::Frame;
                    }
                    if (ImGui::MenuItem(FormatString("%s %s", ICON_FA_STOPWATCH, Localization::GetString("SECONDS").c_str()).c_str())) {
                        timestampFormat = TimestampFormat::Seconds;
                    }
                    ImGui::EndPopup();
                }
                PushStyleVars();

                static ImVec2 searchFilterChildSize = ImVec2(200, 20);
                ImGui::SameLine(0, 6);
                ImGui::SetCursorPosY(
                    backgroundBounds.size.y / 2.0f - searchFilterChildSize.y / 2.0f
                );
                if (ImGui::BeginChild("##compositionSearchChild", ImVec2(0, 0), ImGuiChildFlags_AutoResizeX | ImGuiChildFlags_AutoResizeY)) {
                    static std::string s_newCompositionName = "";
                    if (ImGui::Button(ICON_FA_PLUS)) {
                        s_newCompositionName = "New Composition";
                        ImGui::OpenPopup("##createNewCompositionPopup");
                    }
                    ImGui::SameLine();

                    static bool createNewCompositionPopupFieldFocused = false;
                    PopStyleVars();
                    if (ImGui::BeginPopup("##createNewCompositionPopup")) {
                        if (!createNewCompositionPopupFieldFocused) ImGui::SetKeyboardFocusHere(0);
                        ImGui::InputTextWithHint("##newCompositionName", FormatString("%s %s", ICON_FA_LAYER_GROUP, Localization::GetString("NEW_COMPOSITION_NAME").c_str()).c_str(), &s_newCompositionName);
                        ImGui::SameLine();
                        if (ImGui::Button(FormatString("%s %s", ICON_FA_CHECK, Localization::GetString("OK").c_str()).c_str()) || ImGui::IsKeyPressed(ImGuiKey_Enter)) {
                            Composition newComposition;
                            newComposition.name = s_newCompositionName;
                            if (s_colorMarkFilter != IM_COL32(0, 0, 0, 0)) newComposition.colorMark = s_colorMarkFilter;
                            project.compositions.push_back(newComposition);
                            ImGui::CloseCurrentPopup();
                        }
                        createNewCompositionPopupFieldFocused = true;

                        ImGui::EndPopup();
                    } else createNewCompositionPopupFieldFocused = false;
                    PushStyleVars();

                    PopStyleVars();
                    if (ImGui::ColorButton(FormatString("%s %s", ICON_FA_TAG, Localization::GetString("FILTER_BY_COLOR_MARK").c_str()).c_str(), ImGui::ColorConvertU32ToFloat4(s_colorMarkFilter), ImGuiColorEditFlags_AlphaPreview)) {
                        ImGui::OpenPopup("##filterByColorMark");
                    }

                    if (ImGui::BeginPopup("##filterByColorMark")) {
                        ImGui::SeparatorText(FormatString("%s %s", ICON_FA_FILTER, Localization::GetString("FILTER_BY_COLOR_MARK").c_str()).c_str());
                        static std::string s_colorMarkNameFilter = "";
                        ImGui::InputTextWithHint("##colorMarkFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &s_colorMarkNameFilter);
                        if (ImGui::BeginChild("##filterColorMarkCandidates", ImVec2(ImGui::GetContentRegionAvail().x, 220))) {
                            if (TextColorButton(Localization::GetString("NO_FILTER").c_str(), IM_COL32(0, 0, 0, 0))) {
                                s_colorMarkFilter = IM_COL32(0, 0, 0, 0);
                                ImGui::CloseCurrentPopup();
                            }
                            for (auto& pair : Workspace::s_colorMarks) {    
                                if (!s_colorMarkNameFilter.empty() && LowerCase(pair.first).find(LowerCase(s_colorMarkNameFilter)) == std::string::npos) continue;
                                if (TextColorButton(pair.first.c_str(), ImGui::ColorConvertU32ToFloat4(pair.second))) {
                                    s_colorMarkFilter = pair.second;
                                    ImGui::CloseCurrentPopup();
                                }
                            }
                        }
                        ImGui::EndChild();
                        ImGui::EndPopup();
                    }
                    PushStyleVars();

                    ImGui::SameLine();

                    ImGui::PushItemWidth(220);
                        ImGui::InputTextWithHint("##compositionSearch", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &s_compositionFilter);
                    ImGui::PopItemWidth();
                    searchFilterChildSize = ImGui::GetWindowSize();
                }
                ImGui::EndChild();

                project.customData["TimelineTimestampFormat"] = static_cast<int>(timestampFormat);
                static bool timestampDragged = false;
                if ((timestampHovered || timestampDragged) && ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsWindowFocused() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
                    project.currentFrame += ImGui::GetIO().MouseDelta.x / s_pixelsPerFrame;
                    project.currentFrame = std::clamp(project.currentFrame, 0.0f, project.GetProjectLength());
                    project.OnTimelineSeek();
                    ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                    timestampDragged = true;
                } else timestampDragged = false;
                ImGui::SetWindowFontScale(1.0f);
                infoCentererSize = ImGui::GetWindowSize();
            }
            ImGui::EndChild();

            ImGui::SetCursorPos({0, backgroundBounds.size.y});

            float layerAccumulator = 0;
            bool hasCompositionCandidates = false;
            for (int i = project.compositions.size(); i --> 0;) {
                auto& composition = project.compositions[i];
                if (!s_compositionFilter.empty() && LowerCase(composition.name).find(LowerCase(s_compositionFilter)) == std::string::npos) continue;
                if (s_colorMarkFilter != IM_COL32(0, 0, 0, 0) && composition.colorMark != s_colorMarkFilter) continue;
                hasCompositionCandidates = true;
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
                ImGui::SetItemTooltip(FormatString("%s %s", ICON_FA_TRASH_CAN, Localization::GetString("DELETE_COMPOSITION").c_str()).c_str());
                ImGui::SameLine();
                if (ImGui::Button(composition.lockedCompositionID > 0 ? ICON_FA_LOCK : ICON_FA_LOCK_OPEN)) {
                    ImGui::OpenPopup("##lockCompositionMenu");
                }
                LockCompositionDragSource(&composition);
                LockCompositionDragTarget(&composition);
                ImGui::SetItemTooltip("%s %s", composition.lockedCompositionID > 0 ? ICON_FA_LOCK : ICON_FA_LOCK_OPEN, Localization::GetString("LOCK_COMPOSITION_TO_ANOTHER_COMPOSITION").c_str());
                if (ImGui::BeginPopup("##lockCompositionMenu")) {
                    RenderLockCompositionPopup(&composition);
                    ImGui::EndPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button(ICON_FA_IMAGE)) {
                    ImGui::OpenPopup("##maskCompositionPopup");
                }
#define TIMELINE_MASK_DRAG_DROP "TIMELINE_MASK_DRAG_DROP"
                if (ImGui::BeginDragDropSource()) {
                    ImGui::SetDragDropPayload(TIMELINE_MASK_DRAG_DROP, &i, sizeof(i));
                    ImGui::Text("%s %s", ICON_FA_IMAGE, Localization::GetString("ADD_COMPOSITION_AS_MASK_FOR_ANOTHER_COMPOSITION").c_str());
                    ImGui::EndDragDropSource();
                }
                if (ImGui::BeginDragDropTarget()) {
                    if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(TIMELINE_MASK_DRAG_DROP)) {
                        Composition* fromComposition = &(project.compositions[*((int*) payload->Data)]);
                        CompositionMask newMask;
                        newMask.compositionID = fromComposition->id;
                        composition.masks.push_back(newMask);
                        Rendering::ForceRenderFrame(); 
                    }
                    ImGui::EndDragDropTarget();
                }
                ImGui::SetItemTooltip("%s %s", ICON_FA_IMAGE, Localization::GetString("MASK_COMPOSITION").c_str());
                if (ImGui::BeginPopup("##maskCompositionPopup")) {
                    RenderMaskCompositionPopup(&composition);
                    ImGui::EndPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button(composition.enabled ? ICON_FA_TOGGLE_ON : ICON_FA_TOGGLE_OFF)) {
                    composition.enabled = !composition.enabled;
                    WaveformManager::RequestWaveformRefresh(composition.id);
                }
                ImGui::SetItemTooltip(composition.enabled ? ICON_FA_TOGGLE_ON : ICON_FA_TOGGLE_OFF, Localization::GetString("ENABLE_DISABLE_COMPOSITION").c_str());
                ImGui::SameLine();
                if (ImGui::Button(composition.audioEnabled ? ICON_FA_VOLUME_HIGH : ICON_FA_VOLUME_OFF)) {
                    composition.audioEnabled = !composition.audioEnabled;
                    WaveformManager::RequestWaveformRefresh(composition.id);
                }
                ImGui::SetItemTooltip("%s %s", composition.audioEnabled ? ICON_FA_VOLUME_HIGH : ICON_FA_VOLUME_OFF, Localization::GetString("ENABLE_DISABLE_AUDIO_MIXING").c_str());
                ImGui::SameLine();
                if (ImGui::Button(ICON_FA_PLUS)) {
                    ImGui::OpenPopup(FormatString("##createAttribute%i", composition.id).c_str());
                }
                ImGui::SetItemTooltip("%s %s", ICON_FA_PLUS, Localization::GetString("CREATE_NEW_ATTRIBUTE").c_str());
                if (ImGui::BeginPopup(FormatString("##createAttribute%i", composition.id).c_str())) {
                    RenderNewAttributePopup(&composition);
                    ImGui::EndPopup();
                }
                ImGui::SameLine();
                std::string colorMarkEditorPopupID = FormatString("##colorMarkEditorPopup%i", composition.id);
                bool colorMarkEditorPressed = ImGui::ColorButton(FormatString("%s %s", ICON_FA_TAG, Localization::GetString("COLOR_MARK").c_str()).c_str(), ImGui::ColorConvertU32ToFloat4(composition.colorMark), ImGuiColorEditFlags_AlphaPreview);
                if (colorMarkEditorPressed) {
                    ImGui::OpenPopup(colorMarkEditorPopupID.c_str());
                }
                if (ImGui::BeginPopup(colorMarkEditorPopupID.c_str())) {
                    ImGui::SeparatorText(FormatString("%s %s", ICON_FA_TAG, Localization::GetString("COLOR_MARK").c_str()).c_str());
                    static std::string s_colorMarkFilter = "";
                    ImGui::InputTextWithHint("##colorMarkFilter", FormatString("%s %s", ICON_FA_MAGNIFYING_GLASS, Localization::GetString("SEARCH_FILTER").c_str()).c_str(), &s_colorMarkFilter);
                    if (ImGui::BeginChild("##colorMarkCandidates", ImVec2(ImGui::GetContentRegionAvail().x, 210))) {
                        for (auto& colorPair : Workspace::s_colorMarks) {
                            ImVec4 v4ColorMark = ImGui::ColorConvertU32ToFloat4(colorPair.second);
                            if (TextColorButton(colorPair.first.c_str(), v4ColorMark)) {
                                composition.colorMark = colorPair.second;
                                ImGui::CloseCurrentPopup();
                            }
                        }
                    }
                    ImGui::EndChild();
                    ImGui::EndPopup();
                }
                ImGui::SameLine();
                auto selectedIterator = std::find(project.selectedCompositions.begin(), project.selectedCompositions.end(), composition.id);
                s_compositionTreeScrolls[composition.id] = ImGui::GetScrollY();
                bool compositionTreeExpanded = ImGui::TreeNode(FormatString("%s%s###%i%s", selectedIterator != project.selectedCompositions.end() ? ICON_FA_ARROW_POINTER " " : "", compositionName.c_str(), composition.id, composition.name.c_str()).c_str());
                RenderLayerDragDrop(&composition);
                auto treeNodeID = ImGui::GetItemID();
                bool wasExpanded = false;
                s_compositionTrees[composition.id] = treeNodeID;
                s_attributesExpanded[composition.id] = compositionTreeExpanded;
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                    ImGui::OpenPopup(FormatString("##accessibilityPopup%i", composition.id).c_str());
                }
                if (ImGui::BeginPopup(FormatString("##accessibilityPopup%i", composition.id).c_str())) {
                    RenderCompositionPopup(&composition, treeNodeID);
                    ImGui::EndPopup();
                }
                ImGui::SetCursorPos(baseCursor);
                ImGui::SetNextItemAllowOverlap();
                if (ImGui::InvisibleButton("##accessibilityButton", ImVec2(ImGui::GetWindowSize().x, LAYER_HEIGHT)) && !wasExpanded) {
                    ImGui::TreeNodeSetOpen(treeNodeID, !compositionTreeExpanded);
                }
                RenderLayerDragDrop(&composition);
                if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right)) {
                    ImGui::OpenPopup(FormatString("##compositionLegendPopup%i", composition.id).c_str());
                }

                ImVec2 reservedCursor = ImGui::GetCursorPos();
                ImGui::SetCursorPos({0, backgroundBounds.size.y + layerAccumulator + LAYER_HEIGHT - LAYER_SEPARATOR / 2.0f});
                RectBounds separatorBounds(
                    ImVec2(0, 0), 
                    ImVec2(ImGui::GetWindowSize().x, LAYER_SEPARATOR)
                );

                DrawRect(separatorBounds, ImVec4(0, 0, 0, 1));
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
                    ImGui::Spacing();
                    s_legendOffsets[composition.id] = ImGui::GetCursorPosY() - firstCursor;
                }
                ImGui::PopID();
                PushStyleVars();

                layerAccumulator += LAYER_HEIGHT + s_legendOffsets[composition.id];
            }

            if (!hasCompositionCandidates) {
                UIHelpers::RenderNothingToShowText();
            }

            if (s_legendTargetOpenTree) {
                ImGui::TreeNodeSetOpen(s_legendTargetOpenTree, true);
                s_legendTargetOpenTree = 0;
            }
        }
        ProcessShortcuts();
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
            if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
                splitterDragging = true;   
            }
        }
        if (splitterDragging && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopup) && s_timelineFocused && !s_anyLayerDragged && !s_timelineRulerDragged && !UIShared::s_timelineAnykeyframeDragged) {
            s_splitterState = GetRelativeMousePos().x / ImGui::GetWindowSize().x;
        } else splitterDragging = false;

        s_splitterState = std::clamp(s_splitterState, 0.2f, 0.6f);
    }

    void TimelineUI::RenderLayerDragDrop(Composition* t_composition) {
        auto& project = Workspace::GetProject();
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
            ImGui::SetDragDropPayload(LAYER_REARRANGE_DRAG_DROP, &t_composition->id, sizeof(t_composition->id));
            TextColorButton(FormatString("%s %s", ICON_FA_LAYER_GROUP, t_composition->name.c_str()).c_str(), ImGui::ColorConvertU32ToFloat4(t_composition->colorMark));
            ImGui::Text("%s %s: %s (%0.1f)", ICON_FA_TIMELINE, Localization::GetString("IN_POINT").c_str(), project.FormatFrameToTime(t_composition->beginFrame).c_str(), t_composition->beginFrame);
            ImGui::Text("%s %s: %s (%0.1f)", ICON_FA_TIMELINE, Localization::GetString("OUT_POINT").c_str(), project.FormatFrameToTime(t_composition->endFrame).c_str(), t_composition->endFrame);
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(LAYER_REARRANGE_DRAG_DROP)) {
                int fromID = *((int*) payload->Data);
                auto fromCompositionCandidate = Workspace::GetCompositionByID(fromID);
                if (fromCompositionCandidate.has_value()) {
                    auto& fromComposition = fromCompositionCandidate.value();
                    std::swap(*t_composition, *fromComposition);
                }
            }
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(LAYER_LOCK_DRAG_DROP)) {
                int compositionID = *((int*) payload->Data);
                auto compositionCandidate = Workspace::GetCompositionByID(compositionID);
                if (compositionCandidate) {
                    auto& composition = *compositionCandidate;
                    composition->lockedCompositionID = t_composition->id;
                }
            }
            ImGui::EndDragDropTarget();
        }
    }

    float TimelineUI::ProcessLayerScroll() {
        ImGui::SetCursorPos({0, 0});
        float mouseX = GetRelativeMousePos().x - ImGui::GetScrollX();
        // DUMP_VAR(mouseX);
        float eventZone = ImGui::GetWindowSize().x / 10.0f;
        float mouseDeltaX = 5;
        if (mouseX > ImGui::GetWindowSize().x - eventZone && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            ImGui::SetScrollX(ImGui::GetScrollX() + mouseDeltaX);
            return mouseDeltaX;
        }

        if (mouseX < eventZone && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (ImGui::GetScrollX() + mouseDeltaX > 0) {
                ImGui::SetScrollX(ImGui::GetScrollX() + mouseDeltaX);
            }
            return -mouseDeltaX;
        }

        return 0;
    }

    ImVec2 TimelineUI::GetRelativeMousePos() {
        return ImGui::GetIO().MousePos - ImGui::GetCursorScreenPos();
    }
}