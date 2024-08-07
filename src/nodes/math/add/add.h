#pragma once
#include "raster.h"
#include "common/common.h"

namespace Raster {
    struct Add : public NodeBase {
        Add();
        
        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    };
};