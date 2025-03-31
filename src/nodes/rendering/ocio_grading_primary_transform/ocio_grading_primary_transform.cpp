#include "ocio_grading_primary_transform.h"
#include "common/choice.h"
#include "common/color_management.h"
#include "font/IconsFontAwesome5.h"
#include "common/attribute_metadata.h"
#include "common/line2d.h"

#include "../../../ImGui/imgui.h"
#include "common/dispatchers.h"
#include "raster.h"
#include <OpenColorIO/OpenColorTransforms.h>
#include <OpenColorIO/OpenColorTypes.h>
#include <filesystem>
#include <memory>
#include "common/choice.h"


namespace Raster {

    std::shared_ptr<OCIOGradingPrimaryTransformContext> OCIOGradingPrimaryTransform::m_context;
    std::shared_ptr<OCIOGradingPrimaryTransformContext> OCIOGradingPrimaryTransform::m_inverseContext;

    OCIOGradingPrimaryTransform::OCIOGradingPrimaryTransform() {
        NodeBase::Initialize();

        AddInputPin("Base");
        AddOutputPin("Output");

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("Offset", glm::vec4(0, 0, 0, 0));
        SetupAttribute("Exposure", glm::vec4(0, 0, 0, 0));
        SetupAttribute("Contrast", glm::vec4(1, 1, 1, 1));
        SetupAttribute("Pivot", 1.0f);
        SetupAttribute("ClampBlack", 0.0f);
        SetupAttribute("ClampWhite", 1.0f);
        SetupAttribute("Saturation", 1.0f);
        SetupAttribute("Direction", Choice(std::vector<std::string>{"Forward", "Inverse"}));
        SetupAttribute("LocalBypass", false);
    }

    AbstractPinMap OCIOGradingPrimaryTransform::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};

        auto& project = Workspace::s_project.value();

        auto& framebuffer = m_managedFramebuffer.Get(GetAttribute<Framebuffer>("Base", t_contextData));
        auto offsetCandidate = GetAttribute<glm::vec4>("Offset", t_contextData);
        auto exposureCandidate = GetAttribute<glm::vec4>("Exposure", t_contextData);
        auto contrastCandidate = GetAttribute<glm::vec4>("Contrast", t_contextData);
        auto pivotCandidate = GetAttribute<float>("Pivot", t_contextData);
        auto clampBlackCandidate = GetAttribute<float>("ClampBlack", t_contextData);
        auto clampWhiteCandidate = GetAttribute<float>("ClampWhite", t_contextData);
        auto saturationCandidate = GetAttribute<float>("Saturation", t_contextData);
        auto choiceCandidate = GetAttribute<int>("Direction", t_contextData);
        auto localBypassCandidate = GetAttribute<bool>("LocalBypass", t_contextData);

        if (framebuffer.handle && offsetCandidate && exposureCandidate && choiceCandidate && contrastCandidate && pivotCandidate && saturationCandidate && clampBlackCandidate && clampWhiteCandidate && localBypassCandidate) {
            if (!m_context || !m_inverseContext) {
                m_context = std::make_shared<OCIOGradingPrimaryTransformContext>(OCIO::TransformDirection::TRANSFORM_DIR_FORWARD);
                m_inverseContext = std::make_shared<OCIOGradingPrimaryTransformContext>(OCIO::TransformDirection::TRANSFORM_DIR_INVERSE);
            }
            const int MASTER_CHANNEL = 3;
            const int RED_CHANNEL = 0;
            const int GREEN_CHANNEL = 1;
            const int BLUE_CHANNEL = 2;
            auto& offset = *offsetCandidate;
            auto& exposure = *exposureCandidate;
            auto& contrast = *contrastCandidate;
            auto& pivot = *pivotCandidate;
            auto& clampBlack = *clampBlackCandidate;
            auto& clampWhite = *clampWhiteCandidate;
            auto& saturation = *saturationCandidate;
            auto& direction = *choiceCandidate;
            auto& localBypass = *localBypassCandidate;

            auto& context = direction == 0 ? m_context : m_inverseContext;
            GPU::BindFramebuffer(framebuffer);
            GPU::BindPipeline(context->pipeline);

            auto offset3 = glm::vec3(offset[RED_CHANNEL], offset[GREEN_CHANNEL], offset[BLUE_CHANNEL]);
            offset3[RED_CHANNEL] += offset[MASTER_CHANNEL];
            offset3[GREEN_CHANNEL] += offset[MASTER_CHANNEL];
            offset3[BLUE_CHANNEL] += offset[MASTER_CHANNEL];

            auto exposure3 = glm::vec3(exposure[RED_CHANNEL], exposure[GREEN_CHANNEL], exposure[BLUE_CHANNEL]);
            exposure3[RED_CHANNEL] = std::pow(2.0f, exposure[MASTER_CHANNEL] + exposure[RED_CHANNEL]);
            exposure3[GREEN_CHANNEL] = std::pow(2.0f, exposure[MASTER_CHANNEL] + exposure[GREEN_CHANNEL]);
            exposure3[BLUE_CHANNEL] = std::pow(2.0f, exposure[MASTER_CHANNEL] + exposure[BLUE_CHANNEL]);
            
            auto contrast3 = glm::vec3(contrast[RED_CHANNEL], contrast[GREEN_CHANNEL], contrast[BLUE_CHANNEL]);
            contrast3[RED_CHANNEL] *= contrast[MASTER_CHANNEL];
            contrast3[GREEN_CHANNEL] *= contrast[MASTER_CHANNEL];
            contrast3[BLUE_CHANNEL] *= contrast[MASTER_CHANNEL];

            GPU::SetShaderUniform(context->pipeline.fragment, "ocio_grading_primary_offset", offset3);
            GPU::SetShaderUniform(context->pipeline.fragment, "ocio_grading_primary_exposure", exposure3);
            GPU::SetShaderUniform(context->pipeline.fragment, "ocio_grading_primary_contrast", contrast3);
            GPU::SetShaderUniform(context->pipeline.fragment, "ocio_grading_primary_pivot", pivot);
            GPU::SetShaderUniform(context->pipeline.fragment, "ocio_grading_primary_clampBlack", clampBlack);
            GPU::SetShaderUniform(context->pipeline.fragment, "ocio_grading_primary_clampWhite", clampWhite);
            GPU::SetShaderUniform(context->pipeline.fragment, "ocio_grading_primary_saturation", saturation);
            GPU::SetShaderUniform(context->pipeline.fragment, "ocio_grading_primary_localBypass", localBypass);
            GPU::SetShaderUniform(context->pipeline.fragment, "uResolution", glm::vec2(framebuffer.width, framebuffer.height));
            GPU::DrawArrays(3);

            TryAppendAbstractPinMap(result, "Output", framebuffer);
        }

        return result;
    }

    OCIOGradingPrimaryTransformContext::OCIOGradingPrimaryTransformContext(OCIO::TransformDirection t_direction) {
        this->gp = OCIO::GradingPrimaryTransform::Create(OCIO::GradingStyle::GRADING_LIN);
        gp->makeDynamic();
        gp->setDirection(t_direction);
        this->processor = ColorManagement::s_config->getProcessor(gp);
        this->gpuProcessor = ColorManagement::s_useLegacyGPU ?
                                processor->getOptimizedLegacyGPUProcessor(OCIO::OptimizationFlags::OPTIMIZATION_DEFAULT, 32) :
                                processor->getOptimizedGPUProcessor(OCIO::OptimizationFlags::OPTIMIZATION_DEFAULT);
        this->shaderDesc = OCIO::GpuShaderDesc::CreateShaderDesc();
        shaderDesc->setLanguage(OCIO::GPU_LANGUAGE_GLSL_ES_3_0);
        shaderDesc->setFunctionName("OCIODisplay");
        shaderDesc->setResourcePrefix("ocio_");
        gpuProcessor->extractGpuShaderInfo(shaderDesc);

        static std::optional<std::string> s_ocioPlaceholder;
        if (!s_ocioPlaceholder) {
            s_ocioPlaceholder = ReadFile(GPU::GetShadersPath() + "/ocio_processor/shader.frag");
        }
        auto& placeholder = *s_ocioPlaceholder;
        auto finalShaderCode = ReplaceString(placeholder, "OCIO_SHADER_PLACEHOLDER", shaderDesc->getShaderText());
        int id = Randomizer::GetRandomInteger();
        auto cleanShaderPath = "ocio/shader" + std::to_string(id);
        auto writePath = GPU::GetShadersPath() + cleanShaderPath + ".frag";
        WriteFile(writePath, finalShaderCode);
        auto generatedShader = GPU::GenerateShader(ShaderType::Fragment, cleanShaderPath, false);
        std::filesystem::remove(writePath);
        pipeline = GPU::GeneratePipeline(GPU::s_basicShader, generatedShader);
    }

    OCIOGradingPrimaryTransformContext::~OCIOGradingPrimaryTransformContext() {
        GPU::DestroyShader(pipeline.fragment);
        GPU::DestroyPipeline(pipeline);
    }

    void OCIOGradingPrimaryTransform::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json OCIOGradingPrimaryTransform::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    void OCIOGradingPrimaryTransform::AbstractRenderProperties() {
        RenderAttributeProperty("Offset", {
            SliderStepMetadata(0.01),
            IconMetadata(ICON_FA_UP_DOWN_LEFT_RIGHT)
        });
        RenderAttributeProperty("Exposure", {
            IconMetadata(ICON_FA_SUN),
            SliderStepMetadata(0.01)
        });
        RenderAttributeProperty("Contrast", {
            IconMetadata(ICON_FA_SUN),
            SliderStepMetadata(0.01)
        });
        RenderAttributeProperty("Pivot", {
            IconMetadata(ICON_FA_UP_DOWN_LEFT_RIGHT),
            SliderStepMetadata(0.01)
        });
        RenderAttributeProperty("ClampBlack", {
            IconMetadata(ICON_FA_GEARS),
            SliderStepMetadata(0.01)
        });
        RenderAttributeProperty("ClampWhite", {
            IconMetadata(ICON_FA_GEARS),
            SliderStepMetadata(0.01)
        });
        RenderAttributeProperty("Saturation", {
            IconMetadata(ICON_FA_DROPLET),
            SliderStepMetadata(0.01)
        });
        RenderAttributeProperty("Direction", {
            IconMetadata(ICON_FA_LEFT_RIGHT)
        });
        RenderAttributeProperty("LocalBypass", {IconMetadata(ICON_FA_ROUTE)});
    }

    bool OCIOGradingPrimaryTransform::AbstractDetailsAvailable() {
        return false;
    }

    std::string OCIOGradingPrimaryTransform::AbstractHeader() {
        return "OCIO Grading Primary Transform";
    }

    std::string OCIOGradingPrimaryTransform::Icon() {
        return ICON_FA_DROPLET " " ICON_FA_GEARS;
    }

    std::optional<std::string> OCIOGradingPrimaryTransform::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::OCIOGradingPrimaryTransform>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "OCIO Grading Primary Transform",
            .packageName = RASTER_PACKAGED "ocio_grading_primary_transform",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    }
}