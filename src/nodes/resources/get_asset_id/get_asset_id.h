#pragma once
#include "raster.h"
#include "common/common.h"

namespace Raster {
    struct GetAssetID : public NodeBase {
        GetAssetID();
        
        AbstractPinMap AbstractExecute(ContextData& t_contextData);
        void AbstractRenderProperties();
        void AbstractRenderDetails();
        bool AbstractDetailsAvailable();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();

    private:
        std::optional<int> m_lastAssetID;
    };
};