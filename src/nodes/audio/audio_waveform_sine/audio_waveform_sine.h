#pragma once
#include "raster.h"
#include "common/common.h"

#include "common/audio_samples.h"
#include "audio/audio.h"

#include "common/audio_cache.h"
#include "common/shared_mutex.h"

namespace Raster {
    struct AudioWaveformSine : public NodeBase {
        AudioWaveformSine();
        
        AbstractPinMap AbstractExecute(ContextData& t_contextData);
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();

    private:
        AudioCache m_cache;
        SharedMutex m_mutex;
    };
};