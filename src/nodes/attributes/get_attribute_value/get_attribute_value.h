#pragma once
#include "raster.h"
#include "common/common.h"

namespace Raster {
    struct GetAttributeValue : public NodeBase {
        GetAttributeValue();
        
        AbstractPinMap AbstractExecute(ContextData& t_contextData);
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();
        void AbstractRenderDetails();

        std::optional<AbstractAttribute> GetCompositionAttribute(ContextData& t_contextData);

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();

        private:
        std::optional<int> m_lastAttributeID;
    };
};