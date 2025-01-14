#pragma once

#include "raster.h"
#include "ui/ui.h"
#include "common/common.h"
#include "font/IconsFontAwesome5.h"

#include "../../ImGui/imgui.h"
#include "../../ImGui/imgui_stdlib.h"

#include "../../ImGui/imgui_drag.h"
#include "../../ImGui/imgui_stripes.h"

#define ASSET_MANAGER_DRAG_DROP_PAYLOAD "ASSET_MANAGER_DRAG_DROP_PAYLOAD"

namespace Raster {
    struct TimelineUI : public UI {
        void Render();

        static void PushStyleVars();
        static void PopStyleVars();

        static void RenderTicksBar();
        static void RenderTicks();

        static void RenderComposition(int t_id);
        static void RenderCompositionPopup(Composition* composition, ImGuiID t_parentTreeID = 0);
        static void RenderNewAttributePopup(Composition* t_composition, ImGuiID t_parentTreeID = 0);
        static void RenderLockCompositionPopup(Composition* t_composition);

        static void RenderCompositionsEditor();
        static void RenderLegend();
        static void RenderSplitter();

        static void RenderTimelineRuler();

        static void RenderTimelinePopup();

        static void DeleteComposition(Composition* composition);
        static void AppendSelectedCompositions(Composition* composition);

        static void ProcessCopyAction();
        static void ProcessPasteAction();
        static void ProcessDeleteAction();

        static void ProcessResizeToMatchContentDurationAction();
        static void ProcessAudioMixingAction();
        static void ProcessRecomputeAudioWaveformAction();

        static void ProcessShortcuts();

        static void UpdateCopyPin(GenericPin& pin, std::unordered_map<int, int>& idReplacements);

        static void ReplaceCopyPin(GenericPin& pin, std::unordered_map<int, int>& idReplacements);

        static float ProcessLayerScroll();

        static void RenderLayerDragDrop(Composition* t_composition);

        static ImVec2 GetRelativeMousePos();
    };
};