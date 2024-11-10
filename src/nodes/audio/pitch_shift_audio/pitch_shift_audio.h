#pragma once
#include "raster.h"
#include "common/common.h"
#include "common/generic_audio_decoder.h"
#include "common/audio_cache.h"
#include "common/audio_info.h"
#include "common/shared_mutex.h"

#include <rubberband/RubberBandStretcher.h>

#define MAX_PITCH_SHIFT_CONTEXT_HEALTH 20

namespace Raster {

    struct PitchShiftContext {
        std::shared_ptr<RubberBand::RubberBandStretcher> stretcher;
        AudioCache cache;
        bool seeked;
        int health;
        bool highQualityPitch;

        PitchShiftContext();
    };

    using SharedPitchShiftContext = std::shared_ptr<PitchShiftContext>;

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
        std::unordered_map<float, SharedPitchShiftContext> m_contexts;
    };
};