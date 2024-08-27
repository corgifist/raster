#pragma once
#include "raster.h"
#include "common/common.h"
#include "gpu/gpu.h"
#include "compositor/compositor.h"
#include "common/transform2d.h"

namespace Raster {
    struct TrackingMotionBlur : public NodeBase {
    public:
        TrackingMotionBlur();
        ~TrackingMotionBlur();
        
        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    private:
        Framebuffer m_framebuffer, m_temporalFramebuffer;

        static std::optional<Pipeline> s_pipeline;
        static std::optional<Sampler> s_sampler;
    };
};