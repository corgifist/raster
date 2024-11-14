#include "angular_blur.h"

namespace Raster {

    std::optional<Pipeline> AngularBlur::s_pipeline;

    AngularBlur::AngularBlur() {
        NodeBase::Initialize();

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("Angle", 0.0f);
        SetupAttribute("Center", glm::vec2(0));
        SetupAttribute("Opacity", 1.0f);
        SetupAttribute("Samples", 50);

        AddInputPin("Base");
        AddOutputPin("Output");
    }


    AbstractPinMap AngularBlur::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};

        auto baseCandidate = TextureInteroperability::GetFramebuffer(GetDynamicAttribute("Base", t_contextData));
        auto angleCandidate = GetAttribute<float>("Angle", t_contextData);
        auto centerCandidate = GetAttribute<glm::vec2>("Center", t_contextData);
        auto samplesCandidate = GetAttribute<int>("Samples", t_contextData);
        auto opacityCandidate = GetAttribute<float>("Opacity", t_contextData);
        
        if (!s_pipeline.has_value()) {
            s_pipeline = GPU::GeneratePipeline(
                GPU::s_basicShader,
                GPU::GenerateShader(ShaderType::Fragment, "angular_blur/shader")
            );
        }

        if (s_pipeline.has_value() && baseCandidate.has_value() && angleCandidate.has_value() && centerCandidate.has_value() && samplesCandidate.has_value() && opacityCandidate.has_value()) {
            auto& pipeline = s_pipeline.value();
            auto& base = baseCandidate.value();
            auto angle = glm::radians(angleCandidate.value());
            auto& center = centerCandidate.value();
            auto& opacity = opacityCandidate.value();
            auto samples = (float) samplesCandidate.value();    

            auto& framebuffer = m_framebuffer.Get(baseCandidate);
            GPU::BindFramebuffer(framebuffer);
            GPU::BindPipeline(pipeline);

            GPU::BindTextureToShader(pipeline.fragment, "uTexture", base.attachments.at(0), 0);
            GPU::SetShaderUniform(pipeline.fragment, "uAngularBlurAngle", angle);
            GPU::SetShaderUniform(pipeline.fragment, "uSamples", samples);
            GPU::SetShaderUniform(pipeline.fragment, "uCenter", center);
            GPU::SetShaderUniform(pipeline.fragment, "uOpacity", opacity);

            GPU::SetShaderUniform(pipeline.fragment, "uResolution", glm::vec2(framebuffer.width, framebuffer.height));
            
            GPU::DrawArrays(3);

            TryAppendAbstractPinMap(result, "Output", framebuffer);
        }


        return result;
    }

    void AngularBlur::AbstractRenderProperties() {
        RenderAttributeProperty("Angle", {
            FormatStringMetadata("°"),
            SliderStepMetadata(0.1f)
        });
        RenderAttributeProperty("Center", {
            SliderStepMetadata(0.01f)
        });
        RenderAttributeProperty("Samples");
    }

    void AngularBlur::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);   
    }

    Json AngularBlur::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool AngularBlur::AbstractDetailsAvailable() {
        return false;
    }

    std::string AngularBlur::AbstractHeader() {
        return "Angular Blur";
    }

    std::string AngularBlur::Icon() {
        return ICON_FA_ROTATE;
    }

    std::optional<std::string> AngularBlur::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::AngularBlur>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Angular Blur",
            .packageName = RASTER_PACKAGED "angular_blur",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    }
}