#pragma once
#include "compositor/managed_framebuffer.h"
#include "raster.h"
#include "common/common.h"
#include "gpu/gpu.h"
#include "compositor/compositor.h"
#include "compositor/texture_interoperability.h"

namespace Raster {
    struct CombineChannels : public NodeBase {
    public:
        CombineChannels();
        ~CombineChannels();
        
        AbstractPinMap AbstractExecute(ContextData& t_contextData);
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    private:
        ManagedFramebuffer m_framebuffer;

        static std::optional<Pipeline> s_pipeline;
    };
};