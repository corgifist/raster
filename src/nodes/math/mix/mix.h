#pragma once
#include "raster.h"
#include "common/common.h"

namespace Raster {
    struct Mix : public NodeBase {
        Mix();
        
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