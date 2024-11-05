#pragma once
#include "raster.h"
#include "common/common.h"
#include "gpu/gpu.h"
#include "compositor/compositor.h"

namespace Raster {
    struct Echo : public NodeBase {
    public:
        Echo();
        ~Echo();
        
        AbstractPinMap AbstractExecute(ContextData& t_contextData);
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    private:
        DoubleBufferedFramebuffer m_framebuffer;

        static std::optional<Pipeline> s_echoPipeline;
    };
};