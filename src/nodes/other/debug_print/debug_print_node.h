#pragma once
#include "raster.h"
#include "common/common.h"

namespace Raster {
    struct DebugPrintNode : public NodeBase {
        DebugPrintNode();
        
        AbstractPinMap AbstractExecute(ContextData& t_contextData);
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();
        void AbstractRenderDetails();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();

    private:
        std::optional<std::string> m_lastArbitraryValue;
    };
};