#include "ocio_grading_primary_transform.h"
#include "common/choice.h"
#include "common/color_management.h"
#include "font/IconsFontAwesome5.h"
#include "common/attribute_metadata.h"
#include "common/line2d.h"

#include "../../../ImGui/imgui.h"
#include "common/dispatchers.h"
#include "ocio_pipeline.h"
#include "raster.h"
#include <OpenColorIO/OpenColorTransforms.h>
#include <OpenColorIO/OpenColorTypes.h>
#include <filesystem>
#include <memory>
#include "common/choice.h"
#include "ocio_compiler.h"


namespace Raster {

    std::shared_ptr<OCIOGradingPrimaryTransformContext> OCIOGradingPrimaryTransform::m_context;
    std::shared_ptr<OCIOGradingPrimaryTransformContext> OCIOGradingPrimaryTransform::m_inverseContext;

    OCIOGradingPrimaryTransform::OCIOGradingPrimaryTransform() {
        NodeBase::Initialize();

        AddInputPin("Base");
        AddOutputPin("Output");

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("Brightness", glm::vec4(0, 0, 0, 0));
        SetupAttribute("Contrast", glm::vec4(1, 1, 1, 1));
        SetupAttribute("Gamma", glm::vec4(1));
        SetupAttribute("Offset", glm::vec4(0, 0, 0, 0));
        SetupAttribute("Exposure", glm::vec4(0, 0, 0, 0));
        SetupAttribute("Lift", glm::vec4(0));
        SetupAttribute("Gain", glm::vec4(1));
        SetupAttribute("Saturation", 1.0f);
        SetupAttribute("Pivot", 0.18f);
        SetupAttribute("PivotBlack", 0.0f);
        SetupAttribute("PivotWhite", 1.0f);
        SetupAttribute("ClampBlack", (float) OCIO::GradingPrimary::NoClampBlack());
        SetupAttribute("ClampWhite", (float) OCIO::GradingPrimary::NoClampWhite());
        SetupAttribute("Direction", Choice(std::vector<std::string>{"Forward", "Inverse"}));
    }

    AbstractPinMap OCIOGradingPrimaryTransform::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};

        auto& project = Workspace::s_project.value();

        auto baseCandidate = GetAttribute<Framebuffer>("Base", t_contextData);
        auto& framebuffer = m_managedFramebuffer.Get(baseCandidate);
        auto offsetCandidate = GetAttribute<glm::vec4>("Offset", t_contextData);
        auto exposureCandidate = GetAttribute<glm::vec4>("Exposure", t_contextData);
        auto contrastCandidate = GetAttribute<glm::vec4>("Contrast", t_contextData);
        auto pivotCandidate = GetAttribute<float>("Pivot", t_contextData);
        auto clampBlackCandidate = GetAttribute<float>("ClampBlack", t_contextData);
        auto clampWhiteCandidate = GetAttribute<float>("ClampWhite", t_contextData);
        auto saturationCandidate = GetAttribute<float>("Saturation", t_contextData);
        auto choiceCandidate = GetAttribute<int>("Direction", t_contextData);
        auto brightnessCandidate = GetAttribute<glm::vec4>("Brightness", t_contextData);
        auto gammaCandidate = GetAttribute<glm::vec4>("Gamma", t_contextData);
        auto liftCandidate = GetAttribute<glm::vec4>("Lift", t_contextData);
        auto gainCandidate = GetAttribute<glm::vec4>("Gain", t_contextData);
        auto pBlackCandidate = GetAttribute<float>("PivotBlack", t_contextData);
        auto pWhiteCandidate = GetAttribute<float>("PivotWhite", t_contextData);

        if (framebuffer.handle && baseCandidate && pWhiteCandidate && gainCandidate && pBlackCandidate && liftCandidate && gammaCandidate && offsetCandidate && exposureCandidate && brightnessCandidate && choiceCandidate && contrastCandidate && pivotCandidate && saturationCandidate && clampBlackCandidate && clampWhiteCandidate) {
            if (!m_context || !m_inverseContext) {
                m_context = std::make_shared<OCIOGradingPrimaryTransformContext>(OCIO::TransformDirection::TRANSFORM_DIR_FORWARD);
                m_inverseContext = std::make_shared<OCIOGradingPrimaryTransformContext>(OCIO::TransformDirection::TRANSFORM_DIR_INVERSE);
            }
            auto& base = *baseCandidate;
            auto& offset = *offsetCandidate;
            auto& exposure = *exposureCandidate;
            auto& contrast = *contrastCandidate;
            auto& pivot = *pivotCandidate;
            auto& clampBlack = *clampBlackCandidate;
            auto& clampWhite = *clampWhiteCandidate;
            auto& saturation = *saturationCandidate;
            auto& direction = *choiceCandidate;
            auto& brightness = *brightnessCandidate;
            auto& gamma = *gammaCandidate;
            auto& lift = *liftCandidate;
            auto& gain = *gainCandidate;
            auto& pBlack = *pBlackCandidate;
            auto& pWhite = *pWhiteCandidate;

            auto& context = direction == 0 ? m_context : m_inverseContext;
            if (!context->pipeline.valid) return {};
            auto& ocio = context->pipeline;

            OCIO::GradingPrimary data(OCIO::GradingStyle::GRADING_LIN);
            data.m_brightness = {brightness.r, brightness.g, brightness.b, brightness.a};
            data.m_contrast = {contrast.r, contrast.g, contrast.b, contrast.a};
            data.m_gamma = {gamma.r, gamma.g, gamma.b, gamma.a};
            data.m_offset = {offset.r, offset.g, offset.b, offset.a};
            data.m_exposure = {exposure.r, exposure.g, exposure.b, exposure.a};
            data.m_lift = {lift.r, lift.g, lift.b, lift.a};
            data.m_gain = {gain.r, gain.g, gain.b, gain.a};
            data.m_saturation = saturation;
            data.m_pivot = pivot;
            data.m_pivotBlack = pBlack;
            data.m_pivotWhite = pWhite;
            data.m_clampBlack = clampBlack;
            data.m_clampWhite = clampWhite;
            
            if (ocio.Ready()) {
                OCIO::DynamicPropertyRcPtr dp = ocio.GetDynamicProperty(OCIO::DYNAMIC_PROPERTY_GRADING_PRIMARY);
                OCIO::DynamicPropertyGradingPrimaryRcPtr prop = OCIO::DynamicPropertyValue::AsGradingPrimary(dp);
                prop->setValue(data);
            }

            ocio.Apply(framebuffer, base);

            TryAppendAbstractPinMap(result, "Output", framebuffer);
        }

        return result;
    }

    OCIOGradingPrimaryTransformContext::OCIOGradingPrimaryTransformContext(OCIO::TransformDirection t_direction) {
        this->gp = OCIO::GradingPrimaryTransform::Create(OCIO::GradingStyle::GRADING_LIN);
        gp->makeDynamic();
        gp->setDirection(t_direction);
        this->pipeline = OCIOPipeline(gp);
    }

    OCIOGradingPrimaryTransformContext::~OCIOGradingPrimaryTransformContext() {
        pipeline.Destroy();
    }

    void OCIOGradingPrimaryTransform::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json OCIOGradingPrimaryTransform::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    void OCIOGradingPrimaryTransform::AbstractRenderProperties() {
        RenderAttributeProperty("Brightness", {
            IconMetadata(ICON_FA_SUN), 
            SliderStepMetadata(0.01)
        });
        RenderAttributeProperty("Contrast", {
            IconMetadata(ICON_FA_SUN), 
            SliderStepMetadata(0.01)
        });
        RenderAttributeProperty("Gamma", {
            IconMetadata(ICON_FA_SUN), 
            SliderStepMetadata(0.01)
        });
        RenderAttributeProperty("Offset", {
            IconMetadata(ICON_FA_UP_DOWN_LEFT_RIGHT), 
            SliderStepMetadata(0.01)
        });
        RenderAttributeProperty("Exposure", {
            IconMetadata(ICON_FA_SUN), 
            SliderStepMetadata(0.01)
        });
        RenderAttributeProperty("Lift", {
            IconMetadata(ICON_FA_SUN), 
            SliderStepMetadata(0.01)
        });
        RenderAttributeProperty("Gain", {
            IconMetadata(ICON_FA_SUN), 
            SliderStepMetadata(0.01)
        });
        RenderAttributeProperty("Saturation", {
            IconMetadata(ICON_FA_DROPLET), 
            SliderStepMetadata(0.01)
        });
        RenderAttributeProperty("Pivot", {
            IconMetadata(ICON_FA_UP_DOWN_LEFT_RIGHT), 
            SliderStepMetadata(0.01)
        });
        RenderAttributeProperty("PivotBlack", {
            IconMetadata(ICON_FA_UP_DOWN_LEFT_RIGHT), 
            SliderStepMetadata(0.01)
        });
        RenderAttributeProperty("PivotWhite", {
            IconMetadata(ICON_FA_UP_DOWN_LEFT_RIGHT), 
            SliderStepMetadata(0.01)
        });
        RenderAttributeProperty("ClampBlack", {
            IconMetadata(ICON_FA_DROPLET), 
            SliderStepMetadata(0.01)
        });
        RenderAttributeProperty("ClampWhite", {
            IconMetadata(ICON_FA_DROPLET), 
            SliderStepMetadata(0.01)
        });
        RenderAttributeProperty("Direction", {
            IconMetadata(ICON_FA_LEFT_RIGHT)
        });
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