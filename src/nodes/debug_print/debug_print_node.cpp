#include "common/common.h"
#include "font/font.h"
#include "raster.h"

#include "debug_print_node.h"

namespace Raster {

    DebugPrintNode::DebugPrintNode() {
        NodeBase::GenerateFlowPins();
        this->inputPins.push_back(GenericPin("Exposed Input"));
        this->outputPins.push_back(GenericPin("Exposed Output"));
    }

    AbstractPinMap DebugPrintNode::Execute() {
        AbstractPinMap result = {};
        std::cout << "OMG!! Debug Print!" << std::endl;
        result[this->outputPins[0].pinID] = "Hello?";
        return result;
    }

    std::string DebugPrintNode::Header() {
        return ICON_FA_CIRCLE_NODES " Expressions Test";
    }

    std::optional<std::string> DebugPrintNode::Footer() {
        return std::optional{"A mighty footer just landed!"};
    }
}

extern "C" {
    Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_unique<Raster::DebugPrintNode>();
    }
}