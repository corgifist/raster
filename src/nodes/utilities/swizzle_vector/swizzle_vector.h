#pragma once
#include "raster.h"
#include "common/common.h"

namespace Raster {
    struct SwizzleVector : public NodeBase {
        SwizzleVector();
        
        AbstractPinMap AbstractExecute(ContextData& t_contextData);
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
    private:
        std::optional<std::string> m_lastSwizzleMask;
    };
};