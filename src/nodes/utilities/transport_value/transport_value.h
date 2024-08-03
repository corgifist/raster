#pragma once
#include "raster.h"
#include "common/common.h"
#include "common/transform2d.h"

namespace Raster {
    struct TransportValue : public NodeBase {
        TransportValue();
        
        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    };
};