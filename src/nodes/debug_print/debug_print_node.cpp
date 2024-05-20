#include "common/common.h"
#include "font/font.h"
#include "raster.h"

#include "debug_print_node.h"

namespace Raster {

    DebugPrintNode::DebugPrintNode() {
        NodeBase::GenerateFlowPins();
        this->inputPins.push_back(GenericPin("ExposedInput", PinType::Input));
        this->outputPins.push_back(GenericPin("ExposedOutput", PinType::Output));
    }

    AbstractPinMap DebugPrintNode::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};
        std::optional<std::string> inputAttribute = GetAttribute<std::string>("ExposedInput");
        if (inputAttribute.has_value()) {
            std::cout << inputAttribute.value() << std::endl;
        }
        result[this->outputPins[0].pinID] = std::string("NodeID: ") + std::to_string(nodeID);
        return result;
    }

    std::string DebugPrintNode::Header() {
        return ICON_FA_CIRCLE_NODES " Expressions Test";
    }

    std::optional<std::string> DebugPrintNode::Footer() {
        return std::optional{"ExposedInput: " + GetAttribute<std::string>("ExposedInput").value_or("")};
    }
}

extern "C" {
    Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::DebugPrintNode>();
    }
}