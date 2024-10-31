#pragma once
#include "raster.h"
#include "common/common.h"

namespace Raster {
    struct ExportToAudioBus : public NodeBase {
        ExportToAudioBus();
        
        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        bool AbstractDoesAudioMixing();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        std::vector<int> AbstractGetUsedAudioBuses();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    private:
        int m_lastUsedAudioBusID;
    };
};