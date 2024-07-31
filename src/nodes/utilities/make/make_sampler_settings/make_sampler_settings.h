#pragma once
#include "raster.h"
#include "common/common.h"
#include "common/sampler_settings.h"

namespace Raster {
    struct MakeSamplerSettings : public NodeBase {
        MakeSamplerSettings();
        
        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    };
};