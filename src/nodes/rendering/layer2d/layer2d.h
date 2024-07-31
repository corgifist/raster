#pragma once
#include "raster.h"
#include "common/common.h"
#include "gpu/gpu.h"
#include "compositor/compositor.h"
#include "common/managed_framebuffer.h"
#include "common/transform2d.h"
#include "raster.h"

namespace Raster {
    struct Layer2D : public NodeBase {
    public:
        Layer2D();
        ~Layer2D();
        
        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();

    private:
        Sampler m_sampler;
        ManagedFramebuffer m_managedFramebuffer;

        static std::optional<Pipeline> s_pipeline;
    };
};