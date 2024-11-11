#include "radial_gradient.h"

namespace Raster {

    std::optional<Pipeline> RadialGradient::s_pipeline;

    RadialGradient::RadialGradient() {
        NodeBase::Initialize();

        SetupAttribute("Position", glm::vec2(0.0));
        SetupAttribute("Radius", 0.5f);
        SetupAttribute("FirstColor", glm::vec4(glm::vec3(0), 1));
        SetupAttribute("SecondColor", glm::vec4(1));

        AddOutputPin("Output");
    }

    RadialGradient::~RadialGradient() {
        if (m_framebuffer.Get().handle) {
            m_framebuffer.Destroy();
        }
    }

    AbstractPinMap RadialGradient::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};

        if (!s_pipeline.has_value()) {
            s_pipeline = GPU::GeneratePipeline(
                GPU::s_basicShader,
                GPU::GenerateShader(ShaderType::Fragment, "radial_gradient/shader")
            );
        }
        
        auto positionCandidate = GetAttribute<glm::vec2>("Position", t_contextData);
        auto radiusCandidate = GetAttribute<float>("Radius", t_contextData);
        auto firstColorCandidate = GetAttribute<glm::vec4>("FirstColor", t_contextData);
        auto secondColorCandidate = GetAttribute<glm::vec4>("SecondColor", t_contextData);
        if (s_pipeline.has_value() && positionCandidate.has_value() && radiusCandidate.has_value() && firstColorCandidate.has_value() && secondColorCandidate.has_value()) {
            auto& pipeline = s_pipeline.value();
            auto& framebuffer = m_framebuffer.GetFrontFramebuffer();
            auto& position = positionCandidate.value();
            auto& radius = radiusCandidate.value();
            auto& firstColor = firstColorCandidate.value();
            auto& secondColor = secondColorCandidate.value();
            Compositor::EnsureResolutionConstraintsForFramebuffer(m_framebuffer);
            GPU::BindFramebuffer(framebuffer);
            GPU::BindPipeline(pipeline);

            GPU::SetShaderUniform(pipeline.fragment, "uPosition", position);
            GPU::SetShaderUniform(pipeline.fragment, "uRadius", radius);
            GPU::SetShaderUniform(pipeline.fragment, "uFirstColor", firstColor);
            GPU::SetShaderUniform(pipeline.fragment, "uSecondColor", secondColor);
            GPU::SetShaderUniform(pipeline.fragment, "uResolution", glm::vec2(framebuffer.width, framebuffer.height));

            GPU::DrawArrays(3);
            
            TryAppendAbstractPinMap(result, "Output", framebuffer);
        }

        return result;
    }

    void RadialGradient::AbstractRenderProperties() {
        RenderAttributeProperty("Position", {
            SliderStepMetadata(0.05f)
        });
        RenderAttributeProperty("Radius", {
            SliderStepMetadata(0.05f)
        });
        RenderAttributeProperty("FirstColor");
        RenderAttributeProperty("SecondColor");
    }

    void RadialGradient::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json RadialGradient::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool RadialGradient::AbstractDetailsAvailable() {
        return false;
    }

    std::string RadialGradient::AbstractHeader() {
        return "Radial Gradient";
    }

    std::string RadialGradient::Icon() {
        return ICON_FA_CIRCLE;
    }

    std::optional<std::string> RadialGradient::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::RadialGradient>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Radial Gradient",
            .packageName = RASTER_PACKAGED "radial_gradient",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    }
}