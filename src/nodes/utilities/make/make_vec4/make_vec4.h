#pragma once
#include "raster.h"
#include "common/common.h"

namespace Raster {
    struct MakeVec4 : public NodeBase {
        MakeVec4();
        
        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    };
};