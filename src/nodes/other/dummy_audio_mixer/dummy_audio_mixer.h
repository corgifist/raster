#pragma once
#include "raster.h"
#include "common/common.h"

namespace Raster {
    struct DummyAudioMixer : public NodeBase {
        DummyAudioMixer();
        
        AbstractPinMap AbstractExecute(ContextData& t_contextData);
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        bool AbstractDoesAudioMixing();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    };
};