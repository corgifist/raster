#include "box_blur.h"

namespace Raster {

    std::optional<Pipeline> BoxBlur::s_pipeline;

    BoxBlur::BoxBlur() {
        NodeBase::Initialize();

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("Intensity", glm::vec2(0.5f));
        SetupAttribute("Opacity", 1.0f);
        SetupAttribute("Samples", 50);

        AddInputPin("Base");
        AddOutputPin("Output");
    }

    AbstractPinMap BoxBlur::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};

        auto baseCandidate = TextureInteroperability::GetFramebuffer(GetDynamicAttribute("Base", t_contextData));
        auto intensityCandidate = GetAttribute<glm::vec2>("Intensity", t_contextData);
        auto opacityCandidate = GetAttribute<float>("Opacity", t_contextData);
        auto samplesCandidate = GetAttribute<int>("Samples", t_contextData);
        
       if (!s_pipeline.has_value()) {
            s_pipeline = GPU::GeneratePipeline(
                GPU::s_basicShader,
                GPU::GenerateShader(ShaderType::Fragment, "box_blur/shader")
            );
        }

        if (s_pipeline.has_value() && baseCandidate.has_value() && intensityCandidate.has_value() && samplesCandidate.has_value() && opacityCandidate.has_value()) {
            auto& pipeline = s_pipeline.value();
            auto& base = baseCandidate.value();
            auto& intensity = intensityCandidate.value();
            auto& opacity = opacityCandidate.value();
            auto samples = (float) samplesCandidate.value();    
            auto framebuffer = m_framebuffer.Get(baseCandidate);

            intensity *= 0.1f;
            intensity *= glm::vec2(framebuffer.width, framebuffer.height);

            if (base.attachments.size() >= 1) {
                GPU::BindFramebuffer(framebuffer);
                GPU::BindPipeline(pipeline);
                
                GPU::ClearFramebuffer(0, 0, 0, 0); 

                GPU::BindTextureToShader(pipeline.fragment, "uTexture", base.attachments.at(0), 0);
                GPU::SetShaderUniform(pipeline.fragment, "uBoxBlurIntensity", intensity);
                GPU::SetShaderUniform(pipeline.fragment, "uSamples", samples);
                GPU::SetShaderUniform(pipeline.fragment, "uOpacity", opacity);

                GPU::SetShaderUniform(pipeline.fragment, "uResolution", glm::vec2(framebuffer.width, framebuffer.height));
                
                GPU::DrawArrays(3);
            }

            TryAppendAbstractPinMap(result, "Output", framebuffer);
        }


        return result;
    }

    void BoxBlur::AbstractRenderProperties() {
        RenderAttributeProperty("Intensity", {
            SliderStepMetadata(0.01f),
            IconMetadata(ICON_FA_PERCENT)
        });
        RenderAttributeProperty("Opacity", {
            IconMetadata(ICON_FA_DROPLET),
            SliderBaseMetadata(100),
            SliderRangeMetadata(0, 100),
            FormatStringMetadata("%")
        });
        RenderAttributeProperty("Samples", {
            IconMetadata(ICON_FA_GEARS)
        });
    }

    void BoxBlur::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);   
    }

    Json BoxBlur::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool BoxBlur::AbstractDetailsAvailable() {
        return false;
    }

    std::string BoxBlur::AbstractHeader() {
        return "Box Blur";
    }

    std::string BoxBlur::Icon() {
        return ICON_FA_SQUARE;
    }

    std::optional<std::string> BoxBlur::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::BoxBlur>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Box Blur",
            .packageName = RASTER_PACKAGED "box_blur",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    }
}