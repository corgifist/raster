#include "common/common.h"
#include "font/font.h"
#include "raster.h"

#include "debug_print_node.h"

namespace Raster {

    DebugPrintNode::DebugPrintNode() {
        NodeBase::GenerateFlowPins();
        this->m_attributes["ArbitraryValue"] = std::string("Ummm");

        AddOutputPin("ExposedOutput");
        AddOutputPin("EEEEE");
        AddOutputPin("Um, Pins?");
    }

    AbstractPinMap DebugPrintNode::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};
        std::optional<std::string> inputAttribute = GetAttribute<std::string>("ArbitraryValue");
        TryAppendAbstractPinMap(result, "ExposedOutput", std::string("Exposed Output Works!"));
        return result;
    }

    void DebugPrintNode::AbstractRenderProperties() {
        RenderAttributeProperty("ArbitraryValue");
    }

    std::string DebugPrintNode::Header() {
        return ICON_FA_CIRCLE_NODES " Expressions Test";
    }

    std::optional<std::string> DebugPrintNode::Footer() {
        return "ExposedInput: " + GetAttribute<std::string>("ArbitraryValue").value_or("");
    }
}

extern "C" {
    Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::DebugPrintNode>();
    }

    Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Debug Print",
            .packageName = "packaged.raster.debug_node",
            .category = Raster::NodeCategory::Other
        };
    }
}