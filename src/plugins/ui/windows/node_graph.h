#pragma once

#include "common/user_interface.h"
#include "raster.h"
#include "common/user_interfaces.h"
#include "common/common.h"
#include "utils/widgets.h"

#include "../../../ImGui/imgui.h"
#include "../../../ImGui/imgui_internal.h"
#include "../../../ImGui/imgui_stdlib.h"
#include "../../../ImGui/imgui_node_editor.h"

namespace Raster {

    namespace Nodes = ax::NodeEditor;
    namespace Widgets = ax::Widgets;

    struct NodeGraphUI : public UserInterfaceBase {
        bool open;
        void AbstractRender();
        Json AbstractSerialize();
        void AbstractLoad(Json t_data);

        std::optional<ImVec4> GetColorByDynamicValue(std::any& value);

        NodeGraphUI() : UserInterfaceBase(), open(true) {}

        void ProcessCopyAction();
        void ProcessPasteAction();
        
        void UpdateCopyPin(GenericPin& pin, std::unordered_map<int, int>& idReplacements);
        void ReplaceCopyPin(GenericPin& pin, std::unordered_map<int, int>& idReplacements);

        void ShowLabel(std::string t_label, ImU32 t_color);
        void RenderInputPin(AbstractNode node, GenericPin& pin, bool flow = false);
        void RenderOutputPin(AbstractNode node, GenericPin& pin, bool flow = false);
    };
};