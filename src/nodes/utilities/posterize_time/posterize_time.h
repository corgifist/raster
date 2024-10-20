#pragma once
#include "raster.h"
#include "common/common.h"

namespace Raster {
    struct PosterizeTime : public NodeBase {
        PosterizeTime();
        
        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        void PerformPosterization(float t_levels);

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    };
};