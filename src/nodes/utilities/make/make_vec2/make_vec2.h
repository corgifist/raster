#pragma once
#include "raster.h"
#include "common/common.h"

namespace Raster {
    struct MakeVec2 : public NodeBase {
        MakeVec2();
        
        AbstractPinMap AbstractExecute(ContextData& t_contextData);
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    };
};