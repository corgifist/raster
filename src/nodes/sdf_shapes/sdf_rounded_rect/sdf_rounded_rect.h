#pragma once
#include "raster.h"
#include "common/common.h"
#include "../../rendering/layer2d/layer2d.h"

namespace Raster {
    struct SDFRoundedRect : public NodeBase {
        SDFRoundedRect();
        
        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    };
};