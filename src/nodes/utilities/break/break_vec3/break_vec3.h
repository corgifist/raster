#pragma once
#include "raster.h"
#include "common/common.h"
#include "common/transform2d.h"

namespace Raster {
    struct BreakVec3 : public NodeBase {
        BreakVec3();
        
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