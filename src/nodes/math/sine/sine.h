#pragma once
#include "raster.h"
#include "common/common.h"

namespace Raster {
    struct Sine : public NodeBase {
        Sine();
        
        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();
        void AbstractRenderDetails();

        std::optional<std::any> ComputeSine();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    };
};