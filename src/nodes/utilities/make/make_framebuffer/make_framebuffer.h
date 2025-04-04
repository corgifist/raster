#pragma once
#include "raster.h"
#include "common/common.h"
#include "gpu/gpu.h"
#include "compositor/compositor.h"
#include "compositor/double_buffered_framebuffer.h"

namespace Raster {
    struct MakeFramebuffer : public NodeBase {
        MakeFramebuffer();
        ~MakeFramebuffer();
        
        AbstractPinMap AbstractExecute(ContextData& t_contextData);
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        private:
        std::optional<DoubleBufferedFramebuffer> m_internalFramebuffer;

        static std::optional<Pipeline> s_pipeline;
    };
};