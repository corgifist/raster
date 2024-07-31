#pragma once
#include "raster.h"
#include "common/common.h"

namespace Raster {
    struct GetTime : public NodeBase {
        GetTime();
        
        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    };
};