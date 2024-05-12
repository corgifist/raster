#pragma once

#include "raster.h"
#include "ui/ui.h"

#include "../../ImGui/imgui.h"
#include "../../ImGui/imgui_node_editor.h"

namespace Raster {

    namespace Nodes = ax::NodeEditor;

    struct NodeGraphUI : public UI {
        void Render();
    };
};