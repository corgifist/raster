#pragma once
#include "raster.h"
#include "common/common.h"

namespace Raster {
    struct Abs : public NodeBase {
        Abs();
        
        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();
        void AbstractRenderDetails();

        std::optional<std::any> ComputeAbs();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    };
};