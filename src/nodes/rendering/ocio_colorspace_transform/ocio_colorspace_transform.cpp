#include "ocio_colorspace_transform.h"
#include "common/choice.h"
#include "common/color_management.h"
#include "common/colorspace.h"
#include "font/IconsFontAwesome5.h"
#include "common/attribute_metadata.h"
#include "common/line2d.h"

#include "../../../ImGui/imgui.h"
#include "common/dispatchers.h"
#include "gpu/gpu.h"
#include "ocio_pipeline.h"
#include "raster.h"
#include <OpenColorIO/OpenColorTransforms.h>
#include <OpenColorIO/OpenColorTypes.h>

#include <filesystem>
#include <memory>
#include "common/choice.h"
#include "ocio_compiler.h"
#include "common/colorspace.h"


namespace Raster {
    OCIOColorSpaceTransform::OCIOColorSpaceTransform() {
        NodeBase::Initialize();

        AddInputPin("Base");
        AddOutputPin("Output");

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("SourceColorspace", Colorspace());
        SetupAttribute("DestinationColorspace", Colorspace());
        SetupAttribute("Direction", Choice(std::vector<std::string>{"Forward", "Inverse"}));
        SetupAttribute("Bypass", false);
    }

    AbstractPinMap OCIOColorSpaceTransform::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};

        auto& project = Workspace::s_project.value();

        auto& framebuffer = m_managedFramebuffer.Get(GetAttribute<Framebuffer>("Base", t_contextData));
        auto srcCandidate = GetAttribute<std::string>("SourceColorspace", t_contextData);
        auto dstCandidate = GetAttribute<std::string>("DestinationColorspace", t_contextData);
        auto directionCandidate = GetAttribute<int>("Direction", t_contextData);
        auto bypassCandidate = GetAttribute<bool>("Bypass", t_contextData);

        if (!RASTER_GET_CONTEXT_VALUE(t_contextData, "RENDERING_PASS", bool)) {
            return {};
        }
        if (framebuffer.handle && srcCandidate && dstCandidate && directionCandidate && bypassCandidate) {
            auto& src = *srcCandidate;
            auto& dst = *dstCandidate;
            auto& direction = *directionCandidate;
            auto& bypass = *bypassCandidate;
            if (!m_context || (m_context && (m_context->src != src || m_context->dst != dst || m_context->bypass != bypass || m_context->direction != (direction == 0 ? OCIO::TransformDirection::TRANSFORM_DIR_FORWARD : OCIO::TransformDirection::TRANSFORM_DIR_INVERSE)))) {
                if (m_context) m_context->Destroy();
                m_context = OCIOColorSpaceTransformContext(direction == 0 ? OCIO::TransformDirection::TRANSFORM_DIR_FORWARD : OCIO::TransformDirection::TRANSFORM_DIR_INVERSE, src, dst, bypass);
            }
            if (!m_context || !m_context->valid) return {};
            auto& ocio = m_context->pipeline;
            GPU::BindFramebuffer(framebuffer);
            GPU::BindPipeline(ocio.pipeline);
            GPU::SetShaderUniform(ocio.pipeline.fragment, "uResolution", glm::vec2(framebuffer.width, framebuffer.height));
            if (!framebuffer.attachments.empty())
                GPU::BindTextureToShader(ocio.pipeline.fragment, "uTexture", framebuffer.attachments[0], 0);
            ocio.BindAllTextures(1);
            ocio.SetAllUniforms();
            GPU::DrawArrays(3);

            TryAppendAbstractPinMap(result, "Output", framebuffer);
        }

        return result;
    }

    OCIOColorSpaceTransformContext::OCIOColorSpaceTransformContext(OCIO::TransformDirection t_direction, std::string t_src, std::string t_dst, bool t_bypass) {
        try {
            this->src = t_src;
            this->dst = t_dst;
            this->bypass = t_bypass;
            this->direction = t_direction;
            this->gp = OCIO::ColorSpaceTransform::Create();
            gp->setDirection(direction);
            gp->setSrc(src.c_str());
            gp->setDst(dst.c_str());
            gp->setDataBypass(bypass);
            pipeline = OCIOPipeline(gp);
            this->valid = true; 
        } catch (const OCIO::Exception& ex) {
            RASTER_LOG("failed to generate OCIOColorSpaceTransformContext");
            RASTER_LOG(ex.what());
            this->valid = false;
        }  catch (...) {
            RASTER_LOG("failed to generate OCIOColorSpaceTransformContext due to unknown error");
            this->valid = false;
        } 
    }

    void OCIOColorSpaceTransformContext::Destroy() {
        if (!this->valid) return;
        pipeline.Destroy();
    }

    void OCIOColorSpaceTransform::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json OCIOColorSpaceTransform::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    void OCIOColorSpaceTransform::AbstractRenderProperties() {
        RenderAttributeProperty("SourceColorspace", {
            IconMetadata(ICON_FA_DROPLET)
        });
        RenderAttributeProperty("DestinationColorspace", {
            IconMetadata(ICON_FA_DROPLET)
        });
        RenderAttributeProperty("Direction", {
            IconMetadata(ICON_FA_LEFT_RIGHT)
        });
        RenderAttributeProperty("Bypass", {
            IconMetadata(ICON_FA_ROUTE)
        });
    }

    bool OCIOColorSpaceTransform::AbstractDetailsAvailable() {
        return false;
    }

    std::string OCIOColorSpaceTransform::AbstractHeader() {
        return "OCIO Colorspace Transform";
    }

    std::string OCIOColorSpaceTransform::Icon() {
        return ICON_FA_DROPLET " " ICON_FA_ROUTE;
    }

    std::optional<std::string> OCIOColorSpaceTransform::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::OCIOColorSpaceTransform>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "OCIO Colorspace Transform",
            .packageName = RASTER_PACKAGED "ocio_colorspace_transform",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    }
}