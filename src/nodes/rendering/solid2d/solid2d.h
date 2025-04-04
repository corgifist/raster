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
    struct Solid2D : public NodeBase {
    public:
        Solid2D();
        
        AbstractPinMap AbstractExecute(ContextData& t_contextData);
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

    private:
        ManagedFramebuffer m_managedFramebuffer;

        static std::optional<Pipeline> s_pipeline;
    };
};