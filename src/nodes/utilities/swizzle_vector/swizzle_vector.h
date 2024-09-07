#pragma once
#include "raster.h"
#include "common/common.h"

namespace Raster {
    struct SwizzleVector : public NodeBase {
        SwizzleVector();
        
        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        bool CanBeSwizzled(std::any t_value);
        int GetVectorSize(std::any t_value);
        float GetVectorElement(std::any t_value, int t_index);

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    };
};