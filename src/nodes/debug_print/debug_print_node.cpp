#include "common/common.h"
#include "raster.h"

#include "debug_print_node.h"

namespace Raster {
    AbstractPinMap DebugPrintNode::Execute() {
        AbstractPinMap result = {};
        std::cout << "OMG!! Debug Print!" << std::endl;
        return result;
    }

    std::string DebugPrintNode::Header() {
        return "Debug Node";
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