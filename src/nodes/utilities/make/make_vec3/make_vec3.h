#pragma once
#include "raster.h"
#include "common/common.h"

namespace Raster {
    struct MakeVec3 : public NodeBase {
        MakeVec3();
        
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