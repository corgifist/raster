#pragma once
#include "raster.h"
#include "common/common.h"
#include "../../rendering/layer2d/layer2d.h"

namespace Raster {
    struct SDFCircle : public NodeBase {
        SDFCircle();
        
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