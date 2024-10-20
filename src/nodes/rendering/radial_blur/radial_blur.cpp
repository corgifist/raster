#include "radial_blur.h"

namespace Raster {

    std::optional<Pipeline> RadialBlur::s_pipeline;

    RadialBlur::RadialBlur() {
        NodeBase::Initialize();

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("Intensity", glm::vec2(0.5f));
        SetupAttribute("Center", glm::vec2(0));
        SetupAttribute("Samples", 50);

        AddInputPin("Base");
        AddOutputPin("Output");
    }

    RadialBlur::~RadialBlur() {
        if (m_framebuffer.Get().handle) {
            m_framebuffer.Destroy();
        }
    }

    AbstractPinMap RadialBlur::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};

        auto baseCandidate = TextureInteroperability::GetFramebuffer(GetDynamicAttribute("Base"));
        auto intensityCandidate = GetAttribute<glm::vec2>("Intensity");
        auto centerCandidate = GetAttribute<glm::vec2>("Center");
        auto samplesCandidate = GetAttribute<int>("Samples");
        
        if (!s_pipeline.has_value()) {
            s_pipeline = GPU::GeneratePipeline(
                GPU::s_basicShader,
                GPU::GenerateShader(ShaderType::Fragment, "radial_blur/shader")
            );
        }

        if (s_pipeline.has_value() && baseCandidate.has_value() && intensityCandidate.has_value() && centerCandidate.has_value() && samplesCandidate.has_value()) {
            auto& pipeline = s_pipeline.value();
            auto& base = baseCandidate.value();
            auto& intensity = intensityCandidate.value();
            auto& center = centerCandidate.value();
            auto samples = (float) samplesCandidate.value();    

            Compositor::EnsureResolutionConstraintsForFramebuffer(m_framebuffer);

            intensity *= 0.1f;
            intensity *= glm::vec2(m_framebuffer.width, m_framebuffer.height);

            auto& framebuffer = m_framebuffer.GetFrontFramebuffer();
            GPU::BindFramebuffer(framebuffer);
            GPU::BindPipeline(pipeline);
            GPU::ClearFramebuffer(0, 0, 0, 0);

            GPU::BindTextureToShader(pipeline.fragment, "uTexture", base.attachments.at(0), 0);
            GPU::SetShaderUniform(pipeline.fragment, "uRadialBlurIntensity", intensity);
            GPU::SetShaderUniform(pipeline.fragment, "uSamples", samples);
            GPU::SetShaderUniform(pipeline.fragment, "uCenter", center);

            GPU::SetShaderUniform(pipeline.fragment, "uResolution", glm::vec2(m_framebuffer.width, m_framebuffer.height));
            
            GPU::DrawArrays(3);

            TryAppendAbstractPinMap(result, "Output", framebuffer);
        }


        return result;
    }

    void RadialBlur::AbstractRenderProperties() {
        RenderAttributeProperty("Intensity");
        RenderAttributeProperty("Center");
        RenderAttributeProperty("Samples");
    }

    void RadialBlur::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);   
    }

    Json RadialBlur::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool RadialBlur::AbstractDetailsAvailable() {
        return false;
    }

    std::string RadialBlur::AbstractHeader() {
        return "Radial Blur";
    }

    std::string RadialBlur::Icon() {
        return ICON_FA_EXPAND;
    }

    std::optional<std::string> RadialBlur::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::RadialBlur>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Radial Blur",
            .packageName = RASTER_PACKAGED "radial_blur",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    }
}