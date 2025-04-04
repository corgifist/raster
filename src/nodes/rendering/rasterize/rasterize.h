#pragma once
#include "raster.h"
#include "common/common.h"
#include "gpu/gpu.h"
#include "compositor/compositor.h"
#include "compositor/managed_framebuffer.h"
#include "compositor/texture_interoperability.h"
#include "common/transform2d.h"
#include "raster.h"

namespace Raster {
    struct Rasterize : public NodeBase {
    public:
        Rasterize();
        
        AbstractPinMap AbstractExecute(ContextData& t_contextData);
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

    private:
        int m_lastPassID;
        Framebuffer m_lastFramebuffer;
        ManagedFramebuffer m_managedFramebuffer;
    };
};