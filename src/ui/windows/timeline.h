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

        void RenderTicksBar();
        void RenderTicks();

        void RenderComposition(int t_id);

        void RenderCompositionsEditor();
        void RenderLegend();
        void RenderSplitter();

        void RenderTimelineRuler();

        ImVec2 GetRelativeMousePos();
    };
};