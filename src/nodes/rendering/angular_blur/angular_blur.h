#pragma once
#include "raster.h"
#include "common/common.h"

#include "compositor/compositor.h"
#include "compositor/texture_interoperability.h"
#include "gpu/gpu.h"

namespace Raster {
    struct AngularBlur : public NodeBase {
        AngularBlur();
        ~AngularBlur();
        
        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();
    
    private:
        Framebuffer m_framebuffer;

        static std::optional<Pipeline> s_pipeline;
    };
};