#pragma once
#include "raster.h"
#include "common/common.h"
#include "common/audio_cache.h"

#include "common/generic_audio_decoder.h"

#include "audio/audio.h"

#include "common/audio_samples.h"
#include "common/shared_mutex.h"


namespace Raster {
    struct DecodeAudioAsset : public NodeBase {
        DecodeAudioAsset();
        ~DecodeAudioAsset();
        
        AbstractPinMap AbstractExecute(ContextData& t_contextData);
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();

        void AbstractOnTimelineSeek();
        std::optional<float> AbstractGetContentDuration();

    private:
        GenericAudioDecoder m_decoder;
        SharedMutex m_decodingMutex;
    };
};