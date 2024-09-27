#pragma once

#include "raster.h"
#include "common/common.h"

#include "common/audio_samples.h"
#include "audio/audio.h"

#define MAX_BUFFER_LIFESPAN 300

namespace Raster {

    struct ReverbBuffer {
        SharedRawAudioSamples samples;
        SharedRawAudioSamples outputBuffer;
        int health;
        int pos;
        int len;

        ReverbBuffer();
    };

    struct ReverbEffect : public NodeBase {
        ReverbEffect();
        
        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();

    private:
        ReverbBuffer& GetReverbBuffer(float t_delay);

        std::unordered_map<float, ReverbBuffer> m_reverbBuffers;
    };
};