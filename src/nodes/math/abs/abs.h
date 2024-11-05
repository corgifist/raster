#pragma once
#include "raster.h"
#include "common/common.h"

namespace Raster {
    struct Abs : public NodeBase {
        Abs();
        
        AbstractPinMap AbstractExecute(ContextData& t_contextData);
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();
        void AbstractRenderDetails();

        std::optional<std::any> ComputeAbs(ContextData& t_contextData);

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    private:
        std::optional<std::any> m_lastAbs;
    };
};