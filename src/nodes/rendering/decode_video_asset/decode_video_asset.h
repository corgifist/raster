#pragma once
#include "gpu/gpu.h"
#include "image/image.h"
#include "raster.h"
#include "common/common.h"

#include "common/generic_video_decoder.h"

#include "common/shared_mutex.h"


namespace Raster {
    struct DecodeVideoAsset : public NodeBase {
        DecodeVideoAsset();
        ~DecodeVideoAsset();
        
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
        Texture m_videoTexture;

        ImageAllocation m_imageAllocation;
        GenericVideoDecoder m_decoder;
        SharedMutex m_decodingMutex;
    };
};