#pragma once
#include "raster.h"
#include "common/common.h"

#include "common/audio_samples.h"
#include "audio/audio.h"

namespace Raster {
    struct MixAudioSamples : public NodeBase {
        MixAudioSamples();
        
        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    };
};