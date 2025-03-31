#pragma once
#include "raster.h"
#include "common/common.h"
#include "gpu/gpu.h"
#include "compositor/compositor.h"
#include "compositor/managed_framebuffer.h"
#include "compositor/texture_interoperability.h"
#include "common/transform2d.h"
#include "raster.h"
#include "common/color_management.h"
#include <OpenColorIO/OpenColorTypes.h>
#include <memory>

namespace Raster {

    struct OCIOGradingPrimaryTransformContext {
        OCIO::GradingPrimaryTransformRcPtr gp;
        OCIO::ConstProcessorRcPtr processor;
        OCIO::ConstGPUProcessorRcPtr gpuProcessor;
        OCIO::GpuShaderDescRcPtr shaderDesc;
        Pipeline pipeline;

        OCIOGradingPrimaryTransformContext(OCIO::TransformDirection t_direction);
        ~OCIOGradingPrimaryTransformContext();
    };

    struct OCIOGradingPrimaryTransform : public NodeBase {
    public:
        OCIOGradingPrimaryTransform();
        
        AbstractPinMap AbstractExecute(ContextData& t_contextData);
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

    private:
        static std::shared_ptr<OCIOGradingPrimaryTransformContext> m_context, m_inverseContext;
        ManagedFramebuffer m_managedFramebuffer;
    };
};