#include "common/common.h"
#include "font/font.h"
#include "raster.h"

#include "debug_print_node.h"
#include "../../../ImGui/imgui.h"

namespace Raster {

    DebugPrintNode::DebugPrintNode() {
        NodeBase::Initialize();
        NodeBase::GenerateFlowPins();
        SetupAttribute("ArbitraryValue", std::string("EEE"));

        AddOutputPin("ExposedOutput");
        AddOutputPin("EEEEE");
        AddOutputPin("Um, Pins?");
    }

    AbstractPinMap DebugPrintNode::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};
        std::optional<std::string> inputAttribute = GetAttribute<std::string>("ArbitraryValue", t_contextData);
        TryAppendAbstractPinMap(result, "ExposedOutput", std::string("Exposed Output Works!"));
        PushImmediateFooter("Immediate Footer 1");
        PushImmediateFooter("Immediate Footer 2");
        return result;
    }

    void DebugPrintNode::AbstractRenderProperties() {
        RenderAttributeProperty("ArbitraryValue");
    }

    bool DebugPrintNode::AbstractDetailsAvailable() {
        return true;
    }

    void DebugPrintNode::AbstractRenderDetails() {
        ImGui::Text("Testing Details Rendering");
    }

    std::string DebugPrintNode::AbstractHeader() {
        return "Expressions Test";
    }

    std::string DebugPrintNode::Icon() {
        return ICON_FA_CIRCLE_NODES;
    }

    std::optional<std::string> DebugPrintNode::Footer() {
        return "ExposedInput: " + m_lastArbitraryValue.value_or("");
    }
}


extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::DebugPrintNode>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Debug Print",
            .packageName = "packaged.raster.debug_node",
            .category = Raster::DefaultNodeCategories::s_other
        };
    }
}