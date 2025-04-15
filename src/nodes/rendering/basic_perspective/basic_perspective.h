#pragma once
#include "compositor/managed_framebuffer.h"
#include "raster.h"
#include "common/common.h"
#include "gpu/gpu.h"
#include "compositor/compositor.h"
#include "compositor/texture_interoperability.h"
#include "compositor/geometry_framebuffer.h"

namespace Raster {
    struct BasicPerspective : public NodeBase {
    public:
        BasicPerspective();
        ~BasicPerspective();
        
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
        GeometryFramebuffer m_geometryBuffer;
        GeometryFramebuffer m_temporaryBuffer;

        bool m_hasCamera;

        static std::optional<Pipeline> s_pipeline, s_geometryPipeline, s_combinerPipeline;
    };
};