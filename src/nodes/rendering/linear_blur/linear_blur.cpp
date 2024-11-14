#include "linear_blur.h"

namespace Raster {

    std::optional<Pipeline> LinearBlur::s_pipeline;

    LinearBlur::LinearBlur() {
        NodeBase::Initialize();

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("Angle", 0.0f);
        SetupAttribute("Intensity", 1.0f);
        SetupAttribute("Opacity", 1.0f);
        SetupAttribute("Samples", 50);

        AddInputPin("Base");
        AddOutputPin("Output");
    }

    AbstractPinMap LinearBlur::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};

        auto baseCandidate = TextureInteroperability::GetFramebuffer(GetDynamicAttribute("Base", t_contextData));
        auto angleCandidate = GetAttribute<float>("Angle", t_contextData);
        auto intensityCandidate = GetAttribute<float>("Intensity", t_contextData);
        auto opacityCandidate = GetAttribute<float>("Opacity", t_contextData);
        auto samplesCandidate = GetAttribute<int>("Samples", t_contextData);
        
        if (!s_pipeline.has_value()) {
            s_pipeline = GPU::GeneratePipeline(
                GPU::s_basicShader,
                GPU::GenerateShader(ShaderType::Fragment, "linear_blur/shader")
            );
        } 

        if (s_pipeline.has_value() && baseCandidate.has_value() && angleCandidate.has_value() && intensityCandidate.has_value() && opacityCandidate.has_value() && samplesCandidate.has_value()) {
            auto& pipeline = s_pipeline.value();
            auto& base = baseCandidate.value();
            auto angle = glm::radians(angleCandidate.value());
            auto& intensity = intensityCandidate.value();
            auto& opacity = opacityCandidate.value();
            auto samples = (float) samplesCandidate.value();   

            auto& framebuffer = m_framebuffer.Get(baseCandidate);

            glm::vec2 direction = glm::vec2(glm::cos(angle), glm::sin(angle));
            direction *= 0.1f * intensity;
            direction *= glm::vec2(framebuffer.width, framebuffer.height);

            GPU::BindFramebuffer(framebuffer);
            GPU::BindPipeline(pipeline);

            GPU::BindTextureToShader(pipeline.fragment, "uTexture", base.attachments.at(0), 0);
            GPU::SetShaderUniform(pipeline.fragment, "uLinearBlurIntensity", direction);
            GPU::SetShaderUniform(pipeline.fragment, "uSamples", samples);
            GPU::SetShaderUniform(pipeline.fragment, "uOpacity", opacity);
            GPU::SetShaderUniform(pipeline.fragment, "uResolution", glm::vec2(framebuffer.width, framebuffer.height));
            
            GPU::DrawArrays(3);

            TryAppendAbstractPinMap(result, "Output", framebuffer);
        }


        return result;
    }

    void LinearBlur::AbstractRenderProperties() {
        RenderAttributeProperty("Angle", {
            IconMetadata(ICON_FA_ROTATE)
        });
        RenderAttributeProperty("Intensity", {
            IconMetadata(ICON_FA_PERCENT),
            SliderStepMetadata(0.01f)
        });
        RenderAttributeProperty("Opacity", {
            IconMetadata(ICON_FA_DROPLET),
            SliderBaseMetadata(100),
            SliderRangeMetadata(0, 100)
        });
        RenderAttributeProperty("Samples", {
            IconMetadata(ICON_FA_GEARS)
        });
    }

    void LinearBlur::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);   
    }

    Json LinearBlur::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool LinearBlur::AbstractDetailsAvailable() {
        return false;
    }

    std::string LinearBlur::AbstractHeader() {
        return "Linear Blur";
    }

    std::string LinearBlur::Icon() {
        return ICON_FA_UP_RIGHT_AND_DOWN_LEFT_FROM_CENTER;
    }

    std::optional<std::string> LinearBlur::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::LinearBlur>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Linear Blur",
            .packageName = RASTER_PACKAGED "linear_blur",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    }
}