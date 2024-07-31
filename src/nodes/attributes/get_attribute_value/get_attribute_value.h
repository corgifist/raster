#pragma once
#include "raster.h"
#include "common/common.h"

namespace Raster {
    struct GetAttributeValue : public NodeBase {
        GetAttributeValue();
        
        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();
        void AbstractRenderDetails();

        std::optional<AbstractAttribute> GetCompositionAttribute();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    };
};