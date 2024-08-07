#pragma once
#include "raster.h"
#include "common/common.h"
#include "gpu/gpu.h"
#include "compositor/compositor.h"

namespace Raster {
    struct Echo : public NodeBase {
    public:
        Echo();
        
        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    private:
        Framebuffer m_framebuffer;

        static std::optional<Pipeline> s_echoPipeline;
    };
};