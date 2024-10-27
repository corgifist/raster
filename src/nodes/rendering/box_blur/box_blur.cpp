#include "box_blur.h"

namespace Raster {

    std::optional<Pipeline> BoxBlur::s_pipeline;

    BoxBlur::BoxBlur() {
        NodeBase::Initialize();

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("Intensity", glm::vec2(0.5f));
        SetupAttribute("Samples", 50);

        AddInputPin("Base");
        AddOutputPin("Output");
    }

    BoxBlur::~BoxBlur() {
        if (m_framebuffer.Get().handle) {
            m_framebuffer.Destroy();
        }
    }

    AbstractPinMap BoxBlur::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};

        auto baseCandidate = TextureInteroperability::GetFramebuffer(GetDynamicAttribute("Base"));
        auto intensityCandidate = GetAttribute<glm::vec2>("Intensity");
        auto samplesCandidate = GetAttribute<int>("Samples");
        
       if (!s_pipeline.has_value()) {
            s_pipeline = GPU::GeneratePipeline(
                GPU::s_basicShader,
                GPU::GenerateShader(ShaderType::Fragment, "box_blur/shader")
            );
        }

        if (s_pipeline.has_value() && baseCandidate.has_value() && intensityCandidate.has_value() && samplesCandidate.has_value()) {
            auto& pipeline = s_pipeline.value();
            auto& base = baseCandidate.value();
            auto& intensity = intensityCandidate.value();
            auto samples = (float) samplesCandidate.value();    

            Compositor::EnsureResolutionConstraintsForFramebuffer(m_framebuffer);

            intensity *= 0.1f;
            intensity *= glm::vec2(m_framebuffer.width, m_framebuffer.height);

            auto framebuffer = m_framebuffer.GetFrontFramebuffer();
            if (base.attachments.size() >= 1) {
                GPU::BindFramebuffer(framebuffer);
                GPU::BindPipeline(pipeline);
                GPU::ClearFramebuffer(0, 0, 0, 0);

                GPU::BindTextureToShader(pipeline.fragment, "uTexture", base.attachments.at(0), 0);
                GPU::SetShaderUniform(pipeline.fragment, "uBoxBlurIntensity", intensity);
                GPU::SetShaderUniform(pipeline.fragment, "uSamples", samples);

                GPU::SetShaderUniform(pipeline.fragment, "uResolution", glm::vec2(m_framebuffer.width, m_framebuffer.height));
                
                GPU::DrawArrays(3);
            }

            TryAppendAbstractPinMap(result, "Output", framebuffer);
        }


        return result;
    }

    void BoxBlur::AbstractRenderProperties() {
        RenderAttributeProperty("Intensity", {
            SliderStepMetadata(0.01f)
        });
        RenderAttributeProperty("Samples");
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