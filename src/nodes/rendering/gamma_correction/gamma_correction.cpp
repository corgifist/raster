#include "gamma_correction.h"

namespace Raster {

    std::optional<Pipeline> GammaCorrection::s_pipeline;

    GammaCorrection::GammaCorrection() {
        NodeBase::Initialize();

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("Gamma", 1.0f);

        AddOutputPin("Output");
    }

    GammaCorrection::~GammaCorrection() {
        if (m_framebuffer.Get().handle) {
            m_framebuffer.Destroy();
        }
    }

    AbstractPinMap GammaCorrection::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};

        if (!s_pipeline.has_value()) {
            s_pipeline = GPU::GeneratePipeline(
                GPU::s_basicShader,
                GPU::GenerateShader(ShaderType::Fragment, "gamma_correction/shader")
            );
        }
        
        auto framebufferCandidate = TextureInteroperability::GetFramebuffer(GetDynamicAttribute("Base"));
        auto gammaCandidate = GetAttribute<float>("Gamma");
        if (s_pipeline.has_value() && gammaCandidate.has_value() && framebufferCandidate.has_value()) {
            auto& gamma = gammaCandidate.value();
            auto& base = framebufferCandidate.value();
            auto& pipeline = s_pipeline.value();
            Compositor::EnsureResolutionConstraintsForFramebuffer(m_framebuffer);

            GPU::BindPipeline(pipeline);
            auto& framebuffer = m_framebuffer.GetFrontFramebuffer();
            GPU::BindFramebuffer(framebuffer);
            GPU::ClearFramebuffer(0, 0, 0, 0);

            GPU::BindTextureToShader(pipeline.fragment, "uColor", base.attachments.at(0), 0);
            if (base.attachments.size() > 1) {
                GPU::BindTextureToShader(pipeline.fragment, "uUV", base.attachments.at(1), 1);
                GPU::SetShaderUniform(pipeline.fragment, "uUVAvailable", 1);
            } else {
                GPU::SetShaderUniform(pipeline.fragment, "uUVAvailable", 0);
            }
            GPU::SetShaderUniform(pipeline.fragment, "uGamma", gamma);
            GPU::SetShaderUniform(pipeline.fragment, "uResolution", glm::vec2(m_framebuffer.width, m_framebuffer.height));

            GPU::DrawArrays(3);
            
            TryAppendAbstractPinMap(result, "Output", framebuffer);
        }

        return result;
    }

    void GammaCorrection::AbstractRenderProperties() {
        RenderAttributeProperty("Gamma", {
            SliderStepMetadata(0.05f)
        });
    }

    void GammaCorrection::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json GammaCorrection::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool GammaCorrection::AbstractDetailsAvailable() {
        return false;
    }

    std::string GammaCorrection::AbstractHeader() {
        return "Gamma Correction";
    }

    std::string GammaCorrection::Icon() {
        return ICON_FA_SUN;
    }

    std::optional<std::string> GammaCorrection::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::GammaCorrection>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Gamma Correction",
            .packageName = RASTER_PACKAGED "gamma_correction",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    }
}