

#include "layer2d.h"

#include "../../ImGui/imgui.h"

namespace Raster {

    std::optional<Pipeline> Layer2D::s_pipeline;

    Layer2D::Layer2D() {
        NodeBase::Initialize();

        AddInputPin("Base");
        AddOutputPin("Framebuffer");

        this->m_attributes["Color"] = glm::vec4(1, 1, 1, 1);
        this->m_attributes["Transform"] = Transform2D();
        this->m_attributes["Base"] = Framebuffer();
        this->m_attributes["Texture"] = Texture();
        this->m_attributes["SamplerSettings"] = SamplerSettings();

        if (!s_pipeline.has_value()) {
            s_pipeline = GPU::GeneratePipeline(
                GPU::GenerateShader(ShaderType::Vertex, "layer2d/shader"),
                GPU::GenerateShader(ShaderType::Fragment, "layer2d/shader")
            );
        }

        this->m_sampler = GPU::GenerateSampler();
    }

    Layer2D::~Layer2D() {
        GPU::DestroySampler(m_sampler);
    }

    AbstractPinMap Layer2D::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};

        auto& project = Workspace::s_project.value();

        auto& framebuffer = m_managedFramebuffer.Get(GetAttribute<Framebuffer>("Base"));
        auto transformCandidate = GetAttribute<Transform2D>("Transform");
        auto colorCandidate = GetAttribute<glm::vec4>("Color");
        auto textureCandidate = GetAttribute<Texture>("Texture");
        auto samplerSettingsCandidate = GetAttribute<SamplerSettings>("SamplerSettings");
        if (s_pipeline.has_value() && transformCandidate.has_value() && colorCandidate.has_value() && textureCandidate.has_value() && samplerSettingsCandidate.has_value()) {
            auto& pipeline = s_pipeline.value();
            auto transform = transformCandidate.value();
            auto& color = colorCandidate.value();
            auto& texture = textureCandidate.value();
            auto& samplerSettings = samplerSettingsCandidate.value();

            GPU::BindFramebuffer(framebuffer);
            GPU::BindPipeline(pipeline);

            transform.position.y *= -1;
            transform.angle *= -1;
            transform.anchor.y *= -1;
            GPU::SetShaderUniform(pipeline.vertex, "uMatrix", project.GetProjectionMatrix() * transform.GetTransformationMatrix());
            GPU::SetShaderUniform(pipeline.fragment, "uColor", color);
            GPU::SetShaderUniform(pipeline.fragment, "uTextureAvailable", texture.handle ? 1 : 0);
            if (texture.handle) {
                GPU::BindTextureToShader(pipeline.fragment, "uTexture", texture, 0);
                GPU::BindSampler(m_sampler, 0);
                bool filteringModeMatches = m_sampler.magnifyMode == samplerSettings.filteringMode && m_sampler.minifyMode == samplerSettings.filteringMode;
                bool wrappingModeMatches = m_sampler.sMode == samplerSettings.wrappingMode && m_sampler.tMode == samplerSettings.wrappingMode;
                if (!filteringModeMatches || !wrappingModeMatches) {
                    GPU::SetSamplerTextureFilteringMode(m_sampler, TextureFilteringOperation::Magnify, samplerSettings.filteringMode);
                    GPU::SetSamplerTextureFilteringMode(m_sampler, TextureFilteringOperation::Minify, samplerSettings.filteringMode);

                    GPU::SetSamplerTextureWrappingMode(m_sampler, TextureWrappingAxis::S, samplerSettings.wrappingMode);
                    GPU::SetSamplerTextureWrappingMode(m_sampler, TextureWrappingAxis::T, samplerSettings.wrappingMode);
                    std::cout << "updating sampler" << std::endl;
                }
            }
            GPU::DrawArrays(6);

            GPU::BindSampler(std::nullopt);

            TryAppendAbstractPinMap(result, "Framebuffer", framebuffer);
        }

        return result;
    }

    void Layer2D::AbstractRenderProperties() {
        RenderAttributeProperty("Transform");
        RenderAttributeProperty("Color");
        RenderAttributeProperty("SamplerSettings");
    }

    bool Layer2D::AbstractDetailsAvailable() {
        return false;
    }

    std::string Layer2D::AbstractHeader() {
        return "Layer2D";
    }

    std::string Layer2D::Icon() {
        return ICON_FA_IMAGE " " ICON_FA_UP_DOWN_LEFT_RIGHT;
    }

    std::optional<std::string> Layer2D::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::Layer2D>();
    }

    Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Layer2D",
            .packageName = RASTER_PACKAGED_PACKAGE "layer2d",
            .category = Raster::NodeCategory::Rendering
        };
    }
}