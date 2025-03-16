#pragma once
#include "compositor/managed_framebuffer.h"
#include "raster.h"
#include "common/common.h"
#include "gpu/gpu.h"
#include "compositor/compositor.h"
#include "compositor/texture_interoperability.h"

namespace Raster {
    struct SplitChannels : public NodeBase {
    public:
        SplitChannels();
        ~SplitChannels();
        
        AbstractPinMap AbstractExecute(ContextData& t_contextData);
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    private:
        ManagedFramebuffer m_rFramebuffer, m_gFramebuffer, m_bFramebuffer, m_aFramebuffer;

        static std::optional<Pipeline> s_pipeline;
    };
};