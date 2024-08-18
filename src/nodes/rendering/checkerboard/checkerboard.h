#pragma once
#include "raster.h"
#include "common/common.h"
#include "gpu/gpu.h"
#include "common/managed_framebuffer.h"

namespace Raster {
    struct Checkerboard : public NodeBase {
    public:
        Checkerboard();
        ~Checkerboard();
        
        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});
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