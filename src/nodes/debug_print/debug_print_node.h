#pragma once
#include "raster.h"
#include "common/common.h"

namespace Raster {
    struct DebugPrintNode : public NodeBase {
        DebugPrintNode();
        
        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});
        void AbstractRenderProperties();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    };
};