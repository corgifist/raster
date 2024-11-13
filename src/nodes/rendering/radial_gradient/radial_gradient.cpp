#include "radial_gradient.h"

namespace Raster {

    std::optional<Pipeline> RadialGradient::s_pipeline;

    RadialGradient::RadialGradient() {
        NodeBase::Initialize();

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("Position", glm::vec2(0.0));
        SetupAttribute("Radius", 0.5f);
        SetupAttribute("FirstColor", glm::vec4(glm::vec3(0), 1));
        SetupAttribute("SecondColor", glm::vec4(1));
        SetupAttribute("Opacity", 1.0f);
        SetupAttribute("OnlyScreenSpaceRendering", false);

        AddInputPin("Base");
        AddOutputPin("Output");
    }

    AbstractPinMap RadialGradient::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};

        if (!s_pipeline.has_value()) {
            s_pipeline = GPU::GeneratePipeline(
                GPU::s_basicShader,
                GPU::GenerateShader(ShaderType::Fragment, "radial_gradient/shader")
            );
        }
        
        auto baseCandidate = TextureInteroperability::GetFramebuffer(GetDynamicAttribute("Base", t_contextData));
        auto framebuffer = m_framebuffer.Get(baseCandidate);
        auto positionCandidate = GetAttribute<glm::vec2>("Position", t_contextData);
        auto radiusCandidate = GetAttribute<float>("Radius", t_contextData);
        auto firstColorCandidate = GetAttribute<glm::vec4>("FirstColor", t_contextData);
        auto secondColorCandidate = GetAttribute<glm::vec4>("SecondColor", t_contextData);
        auto opacityCandidate = GetAttribute<float>("Opacity", t_contextData);
        auto onlySpaceScreenRenderingCandidate = GetAttribute<bool>("OnlyScreenSpaceRendering", t_contextData);
        if (s_pipeline.has_value() && baseCandidate.has_value() && positionCandidate.has_value() && radiusCandidate.has_value() && firstColorCandidate.has_value() && secondColorCandidate.has_value()) {
            auto& pipeline = s_pipeline.value();
            auto& base = baseCandidate.value();
            auto& position = positionCandidate.value();
            auto& radius = radiusCandidate.value();
            auto& firstColor = firstColorCandidate.value();
            auto& secondColor = secondColorCandidate.value();
            auto& opacity = opacityCandidate.value();
            auto& onlySpaceScreenRendering = onlySpaceScreenRenderingCandidate.value();
            GPU::BindFramebuffer(framebuffer);
            GPU::BindPipeline(pipeline);
            GPU::ClearFramebuffer(0, 0, 0, 0);

            bool screenSpaceRendering = !(base.attachments.size() >= 2);
            if (onlySpaceScreenRendering) screenSpaceRendering = true;

            GPU::SetShaderUniform(pipeline.fragment, "uPosition", position);
            GPU::SetShaderUniform(pipeline.fragment, "uRadius", radius);
            GPU::SetShaderUniform(pipeline.fragment, "uFirstColor", firstColor);
            GPU::SetShaderUniform(pipeline.fragment, "uSecondColor", secondColor);
            GPU::SetShaderUniform(pipeline.fragment, "uResolution", glm::vec2(framebuffer.width, framebuffer.height));
            GPU::SetShaderUniform(pipeline.fragment, "uScreenSpaceRendering", screenSpaceRendering);
            GPU::SetShaderUniform(pipeline.fragment, "uOpacity", opacity);
            if (!screenSpaceRendering) {
                GPU::BindTextureToShader(pipeline.fragment, "uColorTexture", base.attachments.at(0), 0);
                GPU::BindTextureToShader(pipeline.fragment, "uUVTexture", base.attachments.at(1), 1);
            }

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
        RenderAttributeProperty("Opacity", {
            SliderRangeMetadata(0, 100),
            SliderBaseMetadata(100),
            FormatStringMetadata("%")
        });
        RenderAttributeProperty("OnlyScreenSpaceRendering");
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