#pragma once
#include "raster.h"
#include "common/common.h"

namespace Raster {
    struct Sine : public NodeBase {
        Sine();
        
        AbstractPinMap AbstractExecute(ContextData& t_contextData);
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();
        void AbstractRenderDetails();

        std::optional<std::any> ComputeSine(ContextData& t_contextData);

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    private:
        std::optional<std::any> m_lastSine;
    };
};