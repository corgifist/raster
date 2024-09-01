#include "gamma_correction.h"

namespace Raster {

    std::optional<Pipeline> GammaCorrection::s_pipeline;

    GammaCorrection::GammaCorrection() {
        NodeBase::Initialize();

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("Gamma", 1.0f);

        this->m_framebuffer = Framebuffer();

        if (!s_pipeline.has_value()) {
            s_pipeline = GPU::GeneratePipeline(
                GPU::GenerateShader(ShaderType::Vertex, "gamma_correction/shader"),
                GPU::GenerateShader(ShaderType::Fragment, "gamma_correction/shader")
            );
        }

        AddOutputPin("Output");
    }

    GammaCorrection::~GammaCorrection() {
        if (m_framebuffer.handle) {
            GPU::DestroyFramebufferWithAttachments(m_framebuffer);
        }
    }

    AbstractPinMap GammaCorrection::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};
        
        auto framebufferCandidate = TextureInteroperability::GetFramebuffer(GetDynamicAttribute("Base"));
        auto gammaCandidate = GetAttribute<float>("Gamma");
        if (s_pipeline.has_value() && gammaCandidate.has_value() && framebufferCandidate.has_value()) {
            auto& gamma = gammaCandidate.value();
            auto& framebuffer = framebufferCandidate.value();
            auto& pipeline = s_pipeline.value();
            Compositor::EnsureResolutionConstraintsForFramebuffer(m_framebuffer);

            GPU::BindPipeline(pipeline);
            GPU::BindFramebuffer(m_framebuffer);
            GPU::ClearFramebuffer(0, 0, 0, 0);

            GPU::BindTextureToShader(pipeline.fragment, "uColor", framebuffer.attachments.at(0), 0);
            if (framebuffer.attachments.size() > 1) {
                GPU::BindTextureToShader(pipeline.fragment, "uUV", framebuffer.attachments.at(1), 1);
            } else {
                GPU::SetShaderUniform(pipeline.fragment, "uUV", -1);
            }
            GPU::SetShaderUniform(pipeline.fragment, "uGamma", gamma);
            GPU::SetShaderUniform(pipeline.fragment, "uResolution", glm::vec2(m_framebuffer.width, m_framebuffer.height));

            GPU::DrawArrays(3);
            
            TryAppendAbstractPinMap(result, "Output", m_framebuffer);
        }

        return result;
    }

    void GammaCorrection::AbstractRenderProperties() {
        RenderAttributeProperty("Gamma");
    }

    void GammaCorrection::AbstractLoadSerialized(Json t_data) {
        RASTER_DESERIALIZE_WRAPPER(float, "Gamma");
    }

    Json GammaCorrection::AbstractSerialize() {
        return {
            RASTER_SERIALIZE_WRAPPER(float, "Gamma")
        };
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