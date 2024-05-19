#pragma once
#include "raster.h"
#include "common/common.h"

namespace Raster {
    struct DebugPrintNode : public NodeBase {
        DebugPrintNode();
        
        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});

        std::string Header();
        std::optional<std::string> Footer();
    };
};