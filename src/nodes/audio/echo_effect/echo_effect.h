#pragma once

#include "raster.h"
#include "common/common.h"

#include "common/audio_samples.h"
#include "audio/audio.h"
#include "common/audio_cache.h"
#include "common/shared_mutex.h"

#define MAX_BUFFER_LIFESPAN 20

namespace Raster {

    struct EchoBuffer {
        SharedRawAudioSamples samples;
        AudioCache cache;
        int health;
        int pos;
        int len;

        EchoBuffer();
    };

    struct EchoEffect : public NodeBase {
        EchoEffect();
        
        AbstractPinMap AbstractExecute(ContextData& t_contextData);
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();

    private:
        EchoBuffer& GetEchoBuffer(float t_delay);

        std::unordered_map<float, EchoBuffer> m_reverbBuffers;

        SharedMutex m_mutex;
    };
};