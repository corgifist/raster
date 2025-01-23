#pragma once
#include "common/audio_context_storage.h"
#include "raster.h"
#include "common/common.h"
#include "common/audio_samples.h"
#include "audio/audio.h"
#include "Reverb_libSoX.h"
#include "common/audio_cache.h"
#include "common/shared_mutex.h"

#define MAX_BUFFER_LIFESPAN 100

namespace Raster {

    struct ReverbPrivate {
        reverb_t reverb;
        float* dry;
        std::vector<float*> wet;
        bool initialized;

        float wetGain;
        float roomSize;
        float reverberance;
        float hfDamping;
        float stereoWidth;
        float preDelay;
        float toneLow;
        float toneHigh;

        ReverbPrivate();
    };

    struct ManagedReverbPrivate : ReverbPrivate {
        ManagedReverbPrivate() : ReverbPrivate() {}
        ~ManagedReverbPrivate() {
            reverb_delete(&reverb);
        }
    };

    struct ReverbContext {
        std::vector<std::shared_ptr<ManagedReverbPrivate>> m_reverbs;
        AudioCache cache;
        int health;

        ReverbContext();
    };

    struct ReverbEffect : public NodeBase {
        ReverbEffect();
        
        AbstractPinMap AbstractExecute(ContextData& t_contextData);
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();

    private:
        AudioContextStorage<ReverbContext> m_contexts;
        SharedMutex m_mutex;
    };
};