#pragma once
#include "raster.h"
#include "common/common.h"
#include "gpu/gpu.h"
#include "compositor/compositor.h"
#include "compositor/texture_interoperability.h"
#include "compositor/managed_framebuffer.h"

namespace Raster {
    struct RadialGradient: public NodeBase {
        RadialGradient();
        
        AbstractPinMap AbstractExecute(ContextData& t_contextData);
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();

        ManagedFramebuffer m_framebuffer;
        static std::optional<Pipeline> s_pipeline;
    };
};