#pragma once
#include "raster.h"
#include "common/common.h"

namespace Raster {
    struct DebugPrintNode : public NodeBase {
        AbstractPinMap Execute();

        std::string Header();
        std::optional<std::string> Footer();
    };
};