#pragma once
#include "raster.h"
#include "common/common.h"
#include "gpu/gpu.h"
#include "compositor/compositor.h"

namespace Raster {
    struct MakeFramebuffer : public NodeBase {
        MakeFramebuffer();
        ~MakeFramebuffer();
        
        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        private:
        std::optional<Framebuffer> m_internalFramebuffer;

        static std::optional<Pipeline> s_pipeline;
    };
};