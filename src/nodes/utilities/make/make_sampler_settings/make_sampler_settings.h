#pragma once
#include "raster.h"
#include "common/common.h"
#include "common/sampler_settings.h"

namespace Raster {
    struct MakeSamplerSettings : public NodeBase {
        MakeSamplerSettings();
        
        AbstractPinMap AbstractExecute(ContextData& t_contextData);
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();

    private:
        std::optional<int> m_lastTextureFiltering, m_lastTextureWrapping;
    };
};