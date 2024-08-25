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

        AddInputPin("Base");
        AddOutputPin("Framebuffer");

        if (!s_pipeline.has_value()) {
            s_pipeline = GPU::GeneratePipeline(
                GPU::GenerateShader(ShaderType::Vertex, "checkerboard/shader"),
                GPU::GenerateShader(ShaderType::Fragment, "checkerboard/shader")
            );
        }
    }

    Checkerboard::~Checkerboard() {
        m_framebuffer.Destroy();
    }

    AbstractPinMap Checkerboard::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};
        
        auto baseCandidate = GetAttribute<Framebuffer>("Base");
        auto firstColorCandidate = GetAttribute<glm::vec4>("Color1");
        auto secondColorCandidate = GetAttribute<glm::vec4>("Color2");
        auto positionCandidate = GetAttribute<glm::vec2>("Position");
        auto sizeCandidate = GetAttribute<glm::vec2>("Size");

        auto framebuffer = m_framebuffer.Get(baseCandidate);
        auto& pipeline = s_pipeline.value();

        if (firstColorCandidate.has_value() && secondColorCandidate.has_value() && positionCandidate.has_value() && sizeCandidate.has_value()) {
            auto& firstColor = firstColorCandidate.value();
            auto& secondColor = secondColorCandidate.value();
            auto& position = positionCandidate.value();
            auto& size = sizeCandidate.value();
            GPU::BindFramebuffer(framebuffer);
            GPU::BindPipeline(pipeline);

            GPU::SetShaderUniform(pipeline.fragment, "uResolution", glm::vec2(framebuffer.width, framebuffer.height));
            GPU::SetShaderUniform(pipeline.fragment, "uFirstColor", firstColor);
            GPU::SetShaderUniform(pipeline.fragment, "uSecondColor", secondColor);
            GPU::SetShaderUniform(pipeline.fragment, "uPosition", position);
            GPU::SetShaderUniform(pipeline.fragment, "uSize", size);

            GPU::DrawArrays(3);

            TryAppendAbstractPinMap(result, "Framebuffer", framebuffer);
        }

        return result;
    }

    void Checkerboard::AbstractLoadSerialized(Json t_data) {
        SetAttributeValue("Color1", glm::vec4(t_data["Color1"][0], t_data["Color1"][1], t_data["Color1"][2], t_data["Color1"][3]));    
        SetAttributeValue("Color2", glm::vec4(t_data["Color2"][0], t_data["Color2"][1], t_data["Color2"][2], t_data["Color2"][3])); 
        SetAttributeValue("Position", glm::vec2(t_data["Position"][0], t_data["Position"][1]));
        SetAttributeValue("Size", glm::vec2(t_data["Size"][0], t_data["Size"][1]));   
    }

    Json Checkerboard::AbstractSerialize() {
        auto color1 = RASTER_ATTRIBUTE_CAST(glm::vec4, "Color1");
        auto color2 = RASTER_ATTRIBUTE_CAST(glm::vec4, "Color2");
        auto position = RASTER_ATTRIBUTE_CAST(glm::vec2, "Position");
        auto size = RASTER_ATTRIBUTE_CAST(glm::vec2, "Size");
        return {
            {"Color1", {color1.r, color1.g, color1.b, color1.a}},
            {"Color2", {color2.r, color2.g, color2.b, color2.a}},
            {"Position", {position.x, position.y}},
            {"Size", {size.x, size.y}}
        };
    }

    void Checkerboard::AbstractRenderProperties() {
        RenderAttributeProperty("Color1");
        RenderAttributeProperty("Color2");
        RenderAttributeProperty("Position");
        RenderAttributeProperty("Size");
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