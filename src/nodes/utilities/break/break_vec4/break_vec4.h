#pragma once
#include "raster.h"
#include "common/common.h"
#include "common/transform2d.h"

namespace Raster {
    struct BreakVec4 : public NodeBase {
        BreakVec4();
        
        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    };
};