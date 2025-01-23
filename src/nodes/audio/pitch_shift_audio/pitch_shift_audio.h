#pragma once
#include "audio/time_stretcher.h"
#include "raster.h"
#include "common/common.h"
#include "common/generic_audio_decoder.h"
#include "common/audio_cache.h"
#include "common/audio_info.h"
#include "common/shared_mutex.h"
#include "common/audio_context_storage.h"

namespace Raster {

    struct PitchShiftContext {
        std::shared_ptr<TimeStretcher> stretcher;
        AudioCache cache;
        bool seeked;
        int health;
        bool highQualityPitch;

        PitchShiftContext();
    };

    using SharedPitchShiftContext = PitchShiftContext;

    struct PitchShiftAudio : public NodeBase {
        PitchShiftAudio();
        
        AbstractPinMap AbstractExecute(ContextData& t_contextData);
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        void AbstractOnTimelineSeek();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();

    private:
        SharedMutex m_mutex;
        SharedPitchShiftContext GetContext();
        AudioContextStorage<SharedPitchShiftContext> m_contexts;
    };
};