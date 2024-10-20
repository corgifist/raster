#include "lens_distortion.h"

namespace Raster {

    std::optional<Pipeline> LensDistortion::s_pipeline;

    LensDistortion::LensDistortion() {
        NodeBase::Initialize();

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("K1", 1.2f);
        SetupAttribute("K2", 1.0f);
        SetupAttribute("K3", -3.2f);
        SetupAttribute("Edge", 0.7f);
        SetupAttribute("Dispersion", 0.03f);
        SetupAttribute("DarkEdges", true);

        AddOutputPin("Output");
    }

    LensDistortion::~LensDistortion() {
        if (m_framebuffer.Get().handle) {
            m_framebuffer.Destroy();
        }
    }

    AbstractPinMap LensDistortion::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};
        
        auto baseCandidate = TextureInteroperability::GetFramebuffer(GetDynamicAttribute("Base"));
        auto k1Candidate = GetAttribute<float>("K1");
        auto k2Candidate = GetAttribute<float>("K2");
        auto k3Candidate = GetAttribute<float>("K3");
        auto edgeCandidate = GetAttribute<float>("Edge");
        auto dispersionCandidate = GetAttribute<float>("Dispersion");
        auto darkEdgesCandidate = GetAttribute<bool>("DarkEdges");

        if (!s_pipeline.has_value()) {
            s_pipeline = GPU::GeneratePipeline(
                GPU::s_basicShader,
                GPU::GenerateShader(ShaderType::Fragment, "lens_distortion/shader")
            );
        }

        if (s_pipeline.has_value() && baseCandidate.has_value() && k1Candidate.has_value() && k2Candidate.has_value() && k3Candidate.has_value() && darkEdgesCandidate.has_value()) {
            auto& pipeline = s_pipeline.value();
            auto& base = baseCandidate.value();
            auto& k1 = k1Candidate.value();
            auto& k2 = k2Candidate.value();
            auto& k3 = k3Candidate.value();
            auto& edge = edgeCandidate.value();
            auto& dispersion = dispersionCandidate.value();
            auto& darkEdges = darkEdgesCandidate.value();
            Compositor::EnsureResolutionConstraintsForFramebuffer(m_framebuffer);
            auto& framebuffer = m_framebuffer.GetFrontFramebuffer();
            GPU::BindFramebuffer(framebuffer);
            GPU::BindPipeline(pipeline);
            GPU::ClearFramebuffer(0, 0, 0, 0);

            if (base.attachments.size() >= 1) {
                GPU::BindTextureToShader(pipeline.fragment, "uTexture", base.attachments.at(0), 0);
                GPU::SetShaderUniform(pipeline.fragment, "uK1", k1);
                GPU::SetShaderUniform(pipeline.fragment, "uK2", k2);
                GPU::SetShaderUniform(pipeline.fragment, "uK3", k3);
                GPU::SetShaderUniform(pipeline.fragment, "uEdge", edge);
                GPU::SetShaderUniform(pipeline.fragment, "uDispersion", dispersion);
                GPU::SetShaderUniform(pipeline.fragment, "uDarkEdges", darkEdges ? 1 : 0);
                GPU::SetShaderUniform(pipeline.fragment, "uResolution", glm::vec2(m_framebuffer.width, m_framebuffer.height));
            }

            GPU::DrawArrays(3);

            TryAppendAbstractPinMap(result, "Output", framebuffer);

        }

        return result;
    }

    void LensDistortion::AbstractRenderProperties() {
        RenderAttributeProperty("K1");
        RenderAttributeProperty("K2");
        RenderAttributeProperty("K3");
        RenderAttributeProperty("DarkEdges");
        RenderAttributeProperty("Edge");
        RenderAttributeProperty("Dispersion");
    }

    void LensDistortion::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json LensDistortion::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool LensDistortion::AbstractDetailsAvailable() {
        return false;
    }

    std::string LensDistortion::AbstractHeader() {
        return "Lens Distortion";
    }

    std::string LensDistortion::Icon() {
        return ICON_FA_CAMERA;
    }

    std::optional<std::string> LensDistortion::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::LensDistortion>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Lens Distortion",
            .packageName = RASTER_PACKAGED "lens_distortion",
            .category = Raster::DefaultNodeCategories::s_utilities
        };
    }
}