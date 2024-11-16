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
        SetupAttribute("Opacity", 1.0f);
        SetupAttribute("OnlyScreenSpaceRendering", false);

        AddInputPin("Base");
        AddOutputPin("Output");
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
        auto opacityCandidate = GetAttribute<float>("Opacity", t_contextData);
        auto onlyScreenSpaceRenderingCandidate = GetAttribute<bool>("OnlyScreenSpaceRendering", t_contextData);
        if (s_pipeline.has_value() && baseCandidate.has_value() && offsetCandidate.has_value() && angleCandidate.has_value() && scaleCandidate.has_value() && colorCandidate.has_value() && opacityCandidate.has_value() && onlyScreenSpaceRenderingCandidate.has_value()) {
            auto& pipeline = s_pipeline.value();
            auto& base = baseCandidate.value();
            auto& angle = angleCandidate.value();
            auto& scale = scaleCandidate.value();
            auto& offset = offsetCandidate.value();
            auto& color = colorCandidate.value();
            auto& opacity = opacityCandidate.value();
            auto& onlyScreenSpaceRendering = onlyScreenSpaceRenderingCandidate.value();

            auto& framebuffer = m_framebuffer.Get(baseCandidate);
            bool useScreenSpaceRendering = !(base.attachments.size() >= 2);
            if (onlyScreenSpaceRendering) useScreenSpaceRendering = true;

            GPU::BindFramebuffer(framebuffer);
            GPU::BindPipeline(pipeline);
            
            GPU::SetShaderUniform(pipeline.fragment, "uResolution", glm::vec2(framebuffer.width, framebuffer.height));
            GPU::SetShaderUniform(pipeline.fragment, "uAngle", glm::radians(angle));
            GPU::SetShaderUniform(pipeline.fragment, "uScale", scale);
            GPU::SetShaderUniform(pipeline.fragment, "uOffset", offset);
            GPU::SetShaderUniform(pipeline.fragment, "uColor", color);
            GPU::SetShaderUniform(pipeline.fragment, "uOpacity", opacity);

            GPU::BindTextureToShader(pipeline.fragment, "uTexture", base.attachments.at(0), 0);
            if (!useScreenSpaceRendering) {
                GPU::BindTextureToShader(pipeline.fragment, "uUVTexture", base.attachments.at(1), 1);
            }
            GPU::SetShaderUniform(pipeline.fragment, "uScreenSpaceRendering", useScreenSpaceRendering);

            GPU::DrawArrays(3);

            TryAppendAbstractPinMap(result, "Output", framebuffer);
        }


        return result;
    }

    void Halftone::AbstractRenderProperties() {
        RenderAttributeProperty("Angle", {
            IconMetadata(ICON_FA_ROTATE),
            SliderStepMetadata(0.5f)
        });
        RenderAttributeProperty("Scale", {
            IconMetadata(ICON_FA_UP_DOWN_LEFT_RIGHT),
            SliderStepMetadata(0.05f)
        });
        RenderAttributeProperty("Offset", {
            IconMetadata(ICON_FA_UP_DOWN_LEFT_RIGHT),
            SliderStepMetadata(0.05f)
        });
        RenderAttributeProperty("Color", {
            IconMetadata(ICON_FA_DROPLET)
        });
        RenderAttributeProperty("Opacity", {
            IconMetadata(ICON_FA_DROPLET),
            SliderBaseMetadata(100),
            SliderRangeMetadata(0, 100),
            FormatString("%")
        });
        RenderAttributeProperty("OnlyScreenSpaceRendering", {
            IconMetadata(ICON_FA_IMAGE)
        });
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