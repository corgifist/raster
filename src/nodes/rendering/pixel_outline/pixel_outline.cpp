#include "pixel_outline.h"

namespace Raster {

    std::optional<Pipeline> PixelOutline::s_pipeline;

    PixelOutline::PixelOutline() {
        NodeBase::Initialize();

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("OutlineColor", glm::vec4(1));
        SetupAttribute("BackgroundColor", glm::vec4(glm::vec3(0), 1));
        SetupAttribute("Intensity", 1);
        SetupAttribute("OnlyOutline", true);

        AddInputPin("Base");
        AddOutputPin("Output");
    }

    AbstractPinMap PixelOutline::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};

        auto baseCandidate = TextureInteroperability::GetFramebuffer(GetDynamicAttribute("Base", t_contextData));
        auto framebuffer = m_framebuffer.Get(baseCandidate);
        auto outlineColorCandidate = GetAttribute<glm::vec4>("OutlineColor", t_contextData);
        auto backgroundColorCandidate = GetAttribute<glm::vec4>("BackgroundColor", t_contextData);
        auto intensityCandidate = GetAttribute<float>("Intensity", t_contextData);
        auto onlyOutlineCandidate = GetAttribute<bool>("OnlyOutline", t_contextData);

        if (!s_pipeline.has_value()) {
            s_pipeline = GPU::GeneratePipeline(
                GPU::s_basicShader,
                GPU::GenerateShader(ShaderType::Fragment, "pixel_outline/shader")
            );
        }

        if (s_pipeline.has_value() && baseCandidate.has_value() && outlineColorCandidate.has_value() && backgroundColorCandidate.has_value() && intensityCandidate.has_value() && onlyOutlineCandidate.has_value()) {
            auto& pipeline = s_pipeline.value();
            auto& base = baseCandidate.value();
            auto& outlineColor = outlineColorCandidate.value();
            auto& backgroundColor = backgroundColorCandidate.value();
            auto& intensity = intensityCandidate.value();
            auto& onlyOutline = onlyOutlineCandidate.value();

            GPU::BindFramebuffer(framebuffer);
            GPU::BindPipeline(pipeline);
            GPU::ClearFramebuffer(0, 0, 0, 0);

            GPU::SetShaderUniform(pipeline.fragment, "uResolution", glm::vec2(framebuffer.width, framebuffer.height));
            GPU::SetShaderUniform(pipeline.fragment, "uBackgroundColor", backgroundColor);
            GPU::SetShaderUniform(pipeline.fragment, "uOutlineColor", outlineColor);
            GPU::SetShaderUniform(pipeline.fragment, "uIntensity", intensity);
            GPU::SetShaderUniform(pipeline.fragment, "uBackgroundAlpha", onlyOutline ? 0.0f : 1.0f);

            GPU::BindTextureToShader(pipeline.fragment, "uColorTexture", base.attachments.at(0), 0);

            GPU::DrawArrays(3);

            TryAppendAbstractPinMap(result, "Output", framebuffer);
        }

        return result;
    }

    void PixelOutline::AbstractRenderProperties() {
        RenderAttributeProperty("OutlineColor", {
            IconMetadata(ICON_FA_DROPLET)
        });
        RenderAttributeProperty("BackgroundColor", {
            IconMetadata(ICON_FA_DROPLET)
        });
        RenderAttributeProperty("Intensity", {
            SliderStepMetadata(0.1f),
            IconMetadata(ICON_FA_PERCENT)
        });
        RenderAttributeProperty("OnlyOutline", {
            IconMetadata(ICON_FA_BORDER_NONE)
        });
    }

    void PixelOutline::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);   
    }

    Json PixelOutline::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool PixelOutline::AbstractDetailsAvailable() {
        return false;
    }

    std::string PixelOutline::AbstractHeader() {
        return "Pixel Outline";
    }

    std::string PixelOutline::Icon() {
        return ICON_FA_BORDER_NONE;
    }

    std::optional<std::string> PixelOutline::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::PixelOutline>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Pixel Outline",
            .packageName = RASTER_PACKAGED "pixel_outline",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    }
}