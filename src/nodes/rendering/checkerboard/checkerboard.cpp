#include "checkerboard.h"

namespace Raster {

    std::optional<Pipeline> Checkerboard::s_pipeline;

    Checkerboard::Checkerboard() {
        NodeBase::Initialize();

        SetupAttribute("Base", Framebuffer());
        SetupAttribute("Color1", glm::vec4(0, 0, 0, 1.0f));
        SetupAttribute("Color2", glm::vec4(1, 1, 1, 1));
        SetupAttribute("Position", glm::vec2(0, 0));
        SetupAttribute("Size", glm::vec2(1, 1));
        SetupAttribute("Opacity", 1.0f);
        SetupAttribute("OnlyScreenSpaceRendering", false);

        AddInputPin("Base");
        AddOutputPin("Framebuffer");
    }

    AbstractPinMap Checkerboard::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};
        
        auto baseCandidate = GetAttribute<Framebuffer>("Base", t_contextData);
        auto firstColorCandidate = GetAttribute<glm::vec4>("Color1", t_contextData);
        auto secondColorCandidate = GetAttribute<glm::vec4>("Color2", t_contextData);
        auto positionCandidate = GetAttribute<glm::vec2>("Position", t_contextData);
        auto sizeCandidate = GetAttribute<glm::vec2>("Size", t_contextData);
        auto opacityCandidate = GetAttribute<float>("Opacity", t_contextData);
        auto onlyScreenSpaceRenderingCandidate = GetAttribute<bool>("OnlyScreenSpaceRendering", t_contextData);

        if (!s_pipeline.has_value()) {
            s_pipeline = GPU::GeneratePipeline(
                GPU::s_basicShader,
                GPU::GenerateShader(ShaderType::Fragment, "checkerboard/shader")
            );
        }
        auto& pipeline = s_pipeline.value();

        if (s_pipeline.has_value() && baseCandidate.has_value() && firstColorCandidate.has_value() && secondColorCandidate.has_value() && positionCandidate.has_value() && sizeCandidate.has_value() && onlyScreenSpaceRenderingCandidate.has_value()) {
            auto& base = baseCandidate.value();
            auto& firstColor = firstColorCandidate.value();
            auto& secondColor = secondColorCandidate.value();
            auto& position = positionCandidate.value();
            auto& size = sizeCandidate.value();
            auto& opacity = opacityCandidate.value();
            auto& onlyScreenSpaceRendering = onlyScreenSpaceRenderingCandidate.value();
            auto framebuffer = m_framebuffer.Get(baseCandidate);
            
            GPU::BindFramebuffer(framebuffer);
            GPU::BindPipeline(pipeline);

            bool screenSpaceRendering = !(base.attachments.size() >= 2);
            if (onlyScreenSpaceRendering) screenSpaceRendering = true;

            GPU::SetShaderUniform(pipeline.fragment, "uResolution", glm::vec2(framebuffer.width, framebuffer.height));
            GPU::SetShaderUniform(pipeline.fragment, "uFirstColor", firstColor);
            GPU::SetShaderUniform(pipeline.fragment, "uSecondColor", secondColor);
            GPU::SetShaderUniform(pipeline.fragment, "uPosition", position);
            GPU::SetShaderUniform(pipeline.fragment, "uSize", size);
            GPU::SetShaderUniform(pipeline.fragment, "uScreenSpaceRendering", screenSpaceRendering);
            GPU::SetShaderUniform(pipeline.fragment, "uOpacity", opacity);
            if (!screenSpaceRendering) {
                GPU::BindTextureToShader(pipeline.fragment, "uColorTexture", base.attachments.at(0), 0);
                GPU::BindTextureToShader(pipeline.fragment, "uUVTexture", base.attachments.at(1), 1);
            }

            GPU::DrawArrays(3);

            TryAppendAbstractPinMap(result, "Framebuffer", framebuffer);
        }

        return result;
    }

    void Checkerboard::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json Checkerboard::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    void Checkerboard::AbstractRenderProperties() {
        RenderAttributeProperty("Color1", {
            IconMetadata(ICON_FA_DROPLET)
        });
        RenderAttributeProperty("Color2", {
            IconMetadata(ICON_FA_DROPLET)
        });
        RenderAttributeProperty("Position", {
            IconMetadata(ICON_FA_UP_DOWN_LEFT_RIGHT),
            SliderStepMetadata(0.05f)
        });
        RenderAttributeProperty("Size", {
            IconMetadata(ICON_FA_UP_DOWN_LEFT_RIGHT),
            SliderStepMetadata(0.05f) 
        });
        RenderAttributeProperty("Opacity", {
            IconMetadata(ICON_FA_DROPLET),
            SliderBaseMetadata(100),
            SliderRangeMetadata(0, 100),
            FormatStringMetadata("%")
        });
        RenderAttributeProperty("OnlyScreenSpaceRendering", {
            IconMetadata(ICON_FA_IMAGE)
        });
    }

    bool Checkerboard::AbstractDetailsAvailable() {
        return false;
    }

    std::string Checkerboard::AbstractHeader() {
        return "Checkerboard";
    }

    std::string Checkerboard::Icon() {
        return ICON_FA_CHESS_BOARD;
    }

    std::optional<std::string> Checkerboard::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::Checkerboard>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Checkerboard",
            .packageName = RASTER_PACKAGED "checkerboard",
            .category = Raster::DefaultNodeCategories::s_rendering
        };
    }
}