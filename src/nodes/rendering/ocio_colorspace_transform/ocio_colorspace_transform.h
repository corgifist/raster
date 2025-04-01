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
#include <OpenColorIO/OpenColorIO.h>
#include <OpenColorIO/OpenColorTypes.h>
#include <memory>

namespace Raster {

    struct OCIOColorSpaceTransformContext {
        std::string src, dst;
        bool bypass;
        OCIO::TransformDirection direction;
        Pipeline pipeline;
        std::vector<Texture> lut1ds;
        std::vector<std::string> uniformNames;
        bool valid;

        OCIOColorSpaceTransformContext(OCIO::TransformDirection t_direction, std::string t_src, std::string t_dst, bool t_bypass);
        
        void Destroy();
    };

    struct OCIOColorSpaceTransform : public NodeBase {
    public:
        OCIOColorSpaceTransform();
        
        AbstractPinMap AbstractExecute(ContextData& t_contextData);
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

    private:
        std::optional<OCIOColorSpaceTransformContext> m_context;
        ManagedFramebuffer m_managedFramebuffer;
    };
};