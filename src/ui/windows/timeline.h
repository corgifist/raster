#pragma once

#include "raster.h"
#include "ui/ui.h"
#include "common/common.h"
#include "font/IconsFontAwesome5.h"

#include "../../ImGui/imgui.h"
#include "../../ImGui/imgui_stdlib.h"

#include "../../ImGui/imgui_drag.h"

namespace Raster {
    struct TimelineUI : public UI {
        void Render();

        void PushStyleVars();
        void PopStyleVars();

        void RenderTicksBar();
        void RenderTicks();

        void RenderComposition(int t_id);
        void RenderCompositionPopup(Composition* composition);
        void RenderNewAttributePopup(Composition* t_composition);

        void RenderCompositionsEditor();
        void RenderLegend();
        void RenderSplitter();

        void RenderTimelineRuler();

        void RenderTimelinePopup();

        void DeleteComposition(Composition* composition);
        void AppendSelectedCompositions(Composition* composition);

        void ProcessCopyAction();
        void ProcessPasteAction();

        void UpdateCopyPin(GenericPin& pin, std::unordered_map<int, int>& idReplacements);

        void ReplaceCopyPin(GenericPin& pin, std::unordered_map<int, int>& idReplacements);

        float ProcessLayerScroll();

        ImVec2 GetRelativeMousePos();
    };
};