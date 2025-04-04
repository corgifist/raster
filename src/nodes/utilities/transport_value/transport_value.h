#pragma once
#include "raster.h"
#include "common/common.h"
#include "common/transform2d.h"

namespace Raster {
    struct TransportValue : public NodeBase {
        TransportValue();
        
        AbstractPinMap AbstractExecute(ContextData& t_contextData);
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        Json AbstractSerialize();
        void AbstractLoadSerialized(Json t_data);

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    };
};