#include "ocio_colorspace_transform.h"
#include "common/choice.h"
#include "common/color_management.h"
#include "font/IconsFontAwesome5.h"
#include "common/attribute_metadata.h"
#include "common/line2d.h"

#include "../../../ImGui/imgui.h"
#include "common/dispatchers.h"
#include "gpu/gpu.h"
#include "raster.h"
#include <OpenColorIO/OpenColorTransforms.h>
#include <OpenColorIO/OpenColorTypes.h>

#include <filesystem>
#include <memory>
#include "common/choice.h"
#include "ocio_compiler.h"


namespace Raster {
    OCIOColorSpaceTransform::OCIOColorSpaceTransform() {
        NodeBase::Initialize();

        AddInputPin("Base");
        AddOutputPin("Output");

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("SourceColorspace", std::string("Linear"));
        SetupAttribute("DestinationColorspace", std::string("Linear"));
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
                if (m_context) m_context = std::nullopt;
                RASTER_LOG("recreating context");
                m_context = OCIOColorSpaceTransformContext(direction == 0 ? OCIO::TransformDirection::TRANSFORM_DIR_FORWARD : OCIO::TransformDirection::TRANSFORM_DIR_INVERSE, src, dst, bypass);
            }
            if (!m_context || !m_context->valid || !m_context->pipeline.handle) return {};
            auto& pipeline = m_context->pipeline;
            GPU::BindFramebuffer(framebuffer);
            GPU::BindPipeline(pipeline);
            GPU::SetShaderUniform(pipeline.fragment, "uResolution", glm::vec2(framebuffer.width, framebuffer.height));
            if (!framebuffer.attachments.empty())
                GPU::BindTextureToShader(pipeline.fragment, "uTexture", framebuffer.attachments[0], 0);
            for (int i = 0; i < m_context->lut1ds.size(); i++) {
                GPU::BindTextureToShader(pipeline.fragment, m_context->uniformNames[i], m_context->lut1ds[i], i + 1);
            }
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
            auto gp = OCIO::ColorSpaceTransform::Create();
            gp->setDirection(direction);
            gp->setSrc(src.c_str());
            gp->setDst(dst.c_str());
            gp->setDataBypass(bypass);
            auto processor = ColorManagement::s_config->getProcessor(gp);
            auto gpuProcessor = ColorManagement::s_useLegacyGPU ?
                                    processor->getOptimizedLegacyGPUProcessor(OCIO::OptimizationFlags::OPTIMIZATION_DEFAULT, 32) :
                                    processor->getOptimizedGPUProcessor(OCIO::OptimizationFlags::OPTIMIZATION_DEFAULT);
            auto shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
            shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_GLSL_ES_3_0);
            shaderDesc->setFunctionName("OCIODisplay");
            shaderDesc->setResourcePrefix("ocio_");
            gpuProcessor->extractGpuShaderInfo(shaderDesc);
            DUMP_VAR(shaderDesc->getShaderText());
            pipeline = CompileOCIOShader(shaderDesc->getShaderText());
            for (int i = 0; i < shaderDesc->getNumTextures(); i++) {
                const char* textureName, *samplerName;
                unsigned int width, height;
                OCIO::GpuShaderCreator::TextureType channel;
                OCIO::GpuShaderCreator::TextureDimensions dimensions;
                OCIO::Interpolation interpolation;
                shaderDesc->getTexture(i, textureName, samplerName, width, height, channel, dimensions, interpolation);
                DUMP_VAR(width);
                DUMP_VAR(height);
                Texture generatedTexture = GPU::GenerateTexture(width, height, channel == OCIO::GpuShaderDesc::TEXTURE_RGB_CHANNEL ? 3 : 1, TexturePrecision::Full);
                std::unique_ptr<float> data = (std::unique_ptr<float>) new float[width * height * (channel == OCIO::GpuShaderDesc::TEXTURE_RGB_CHANNEL ? 3 : 1)];
                const float* cData = data.get();
                shaderDesc->getTextureValues(i, cData);
                GPU::UpdateTexture(generatedTexture, 0, 0, width, height, (channel == OCIO::GpuShaderDesc::TEXTURE_RGB_CHANNEL ? 3 : 1), (uint8_t*) cData);
                lut1ds.push_back(generatedTexture);
                uniformNames.push_back(samplerName);
            }
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

    OCIOColorSpaceTransformContext::~OCIOColorSpaceTransformContext() {

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