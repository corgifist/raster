#include "halftone.h"

namespace Raster {

    std::optional<Pipeline> Halftone::s_pipeline;

    Halftone::Halftone() {
        NodeBase::Initialize();

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("Angle", 0.0f);
        SetupAttribute("Scale", 0.5f);
        SetupAttribute("Offset", glm::vec2(0));
        SetupAttribute("Color", glm::vec4(1));

        AddInputPin("Base");
        AddOutputPin("Output");
    }

    Halftone::~Halftone() {
        if (m_framebuffer.Get().handle) {
            m_framebuffer.Destroy();
        }
    }

    AbstractPinMap Halftone::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};

        if (!s_pipeline.has_value()) {
            s_pipeline = GPU::GeneratePipeline(
                GPU::s_basicShader,
                GPU::GenerateShader(ShaderType::Fragment, "halftone/shader")
            );
        }

        auto baseCandidate = TextureInteroperability::GetFramebuffer(GetDynamicAttribute("Base", t_contextData));
        auto angleCandidate = GetAttribute<float>("Angle", t_contextData);
        auto scaleCandidate = GetAttribute<float>("Scale", t_contextData);
        auto offsetCandidate = GetAttribute<glm::vec2>("Offset", t_contextData);
        auto colorCandidate = GetAttribute<glm::vec4>("Color", t_contextData);
        if (s_pipeline.has_value() && baseCandidate.has_value() && offsetCandidate.has_value() && angleCandidate.has_value() && scaleCandidate.has_value() && colorCandidate.has_value()) {
            auto& pipeline = s_pipeline.value();
            auto& base = baseCandidate.value();
            auto& angle = angleCandidate.value();
            auto& scale = scaleCandidate.value();
            auto& offset = offsetCandidate.value();
            auto& color = colorCandidate.value();

            Compositor::EnsureResolutionConstraintsForFramebuffer(m_framebuffer);

            auto& framebuffer = m_framebuffer.GetFrontFramebuffer();
            GPU::BindFramebuffer(framebuffer);
            GPU::BindPipeline(pipeline);
            GPU::ClearFramebuffer(0, 0, 0, 0);
            
            GPU::SetShaderUniform(pipeline.fragment, "uResolution", glm::vec2(m_framebuffer.width, m_framebuffer.height));
            GPU::SetShaderUniform(pipeline.fragment, "uAngle", glm::radians(angle));
            GPU::SetShaderUniform(pipeline.fragment, "uScale", scale);
            GPU::SetShaderUniform(pipeline.fragment, "uOffset", offset);
            GPU::SetShaderUniform(pipeline.fragment, "uColor", color);

            GPU::BindTextureToShader(pipeline.fragment, "uTexture", base.attachments.at(0), 0);

            GPU::DrawArrays(3);

            TryAppendAbstractPinMap(result, "Output", framebuffer);
        }


        return result;
    }

    void Halftone::AbstractRenderProperties() {
        RenderAttributeProperty("Angle");
        RenderAttributeProperty("Scale");
        RenderAttributeProperty("Offset");
        RenderAttributeProperty("Color");
    }

    void Halftone::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);   
    }

    Json Halftone::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool Halftone::AbstractDetailsAvailable() {
        return false;
    }

    std::string Halftone::AbstractHeader() {
        return "Halftone";
    }

    std::string Halftone::Icon() {
        return ICON_FA_BRAILLE;
    }

    std::optional<std::string> Halftone::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::Halftone>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Halftone",
            .packageName = RASTER_PACKAGED "halftone",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    }
}