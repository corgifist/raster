#pragma once

#include "raster.h"
#include "ui/ui.h"
#include "common/common.h"
#include "utils/widgets.h"

#include "../../ImGui/imgui.h"
#include "../../ImGui/imgui_stdlib.h"
#include "../../ImGui/imgui_node_editor.h"

namespace Raster {

    namespace Nodes = ax::NodeEditor;
    namespace Widgets = ax::Widgets;

    struct NodeGraphUI : public UI {
        void Render();

        std::optional<ImVec4> GetColorByDynamicValue(std::any& value);

        void ProcessCopyAction();
        void ProcessPasteAction();
        
        void UpdateCopyPin(GenericPin& pin, std::unordered_map<int, int>& idReplacements);
        void ReplaceCopyPin(GenericPin& pin, std::unordered_map<int, int>& idReplacements);

        void ShowLabel(std::string t_label, ImU32 t_color);
        void RenderInputPin(GenericPin& pin, bool flow = false);
        void RenderOutputPin(GenericPin& pin, bool flow = false);
    };
};