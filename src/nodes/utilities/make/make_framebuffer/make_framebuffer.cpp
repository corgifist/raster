#include "common/common.h"
#include "font/font.h"
#include "raster.h"

#include "make_framebuffer.h"

namespace Raster {

    std::optional<Pipeline> MakeFramebuffer::s_pipeline;

    MakeFramebuffer::MakeFramebuffer() {
        NodeBase::Initialize();

        AddOutputPin("Value");

        SetupAttribute("BackgroundColor", glm::vec4(1));
        SetupAttribute("BackgroundTexture", Texture());

        if (!s_pipeline.has_value()) {
            s_pipeline = GPU::GeneratePipeline(
                GPU::GenerateShader(ShaderType::Vertex, "make_framebuffer/shader"),
                GPU::GenerateShader(ShaderType::Fragment, "make_framebuffer/shader")
            );
        }
    }

    MakeFramebuffer::~MakeFramebuffer() {
        if (m_internalFramebuffer.has_value()) {
            auto& framebuffer = m_internalFramebuffer.value();
            for (auto& attachment : framebuffer.attachments) {
                GPU::DestroyTexture(attachment);
            }
            GPU::DestroyFramebuffer(framebuffer);
        }
    }

    AbstractPinMap MakeFramebuffer::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};

        auto backgroundColorCandidate = GetAttribute<glm::vec4>("BackgroundColor");
        if (backgroundColorCandidate.has_value()) {
            auto& backgroundColor = backgroundColorCandidate.value();
            auto requiredResolution = Compositor::GetRequiredResolution();
            if (!m_internalFramebuffer.has_value() || m_internalFramebuffer.value().width != (int) requiredResolution.x || m_internalFramebuffer.value().height != (int) requiredResolution.y) {
                if (m_internalFramebuffer.has_value()) {
                    auto& framebuffer = m_internalFramebuffer.value();
                    for (auto& attachment : framebuffer.attachments) {
                        GPU::DestroyTexture(attachment);
                    }
                    GPU::DestroyFramebuffer(framebuffer);
                }
                m_internalFramebuffer = Compositor::GenerateCompatibleFramebuffer(requiredResolution);
            }

            if (m_internalFramebuffer.has_value() && s_pipeline.has_value()) {
                auto backgroundTextureCandidate = GetAttribute<Texture>("BackgroundTexture");
                auto& framebuffer = m_internalFramebuffer.value();
                GPU::BindFramebuffer(framebuffer);
                std::cout << backgroundColor.r << " " << backgroundColor.g << " " << backgroundColor.b << std::endl;
                GPU::ClearFramebuffer(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
                if (backgroundTextureCandidate.has_value() && backgroundTextureCandidate.value().handle) {
                    auto& pipeline = s_pipeline.value();
                    auto& texture = backgroundTextureCandidate.value();
                    GPU::BindPipeline(pipeline);
                    GPU::SetShaderUniform(pipeline.fragment, "uColor", backgroundColor);
                    GPU::SetShaderUniform(pipeline.fragment, "uResolution", requiredResolution);
                    GPU::BindTextureToShader(pipeline.fragment, "uTexture", texture, 0);
                    GPU::DrawArrays(3);
                }
                TryAppendAbstractPinMap(result, "Value", framebuffer);
            }
        }

        return result;
    }

    void MakeFramebuffer::AbstractRenderProperties() {
        RenderAttributeProperty("BackgroundColor");
    }

    void MakeFramebuffer::AbstractLoadSerialized(Json t_data) {
        SetAttributeValue("BackgroundColor", glm::vec4(t_data["BackgroundColor"][0], t_data["BackgroundColor"][1], t_data["BackgroundColor"][2], t_data["BackgroundColor"][3]));    
    }

    Json MakeFramebuffer::AbstractSerialize() {
        auto color = RASTER_ATTRIBUTE_CAST(glm::vec4, "BackgroundColor");
        return {
            {"BackgroundColor", {color.r, color.g, color.b, color.a}}
        };
    }

    bool MakeFramebuffer::AbstractDetailsAvailable() {
        return false;
    }

    std::string MakeFramebuffer::AbstractHeader() {
        return "Make Framebuffer";
    }

    std::string MakeFramebuffer::Icon() {
        return ICON_FA_IMAGE;
    }

    std::optional<std::string> MakeFramebuffer::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::MakeFramebuffer>();
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Make Framebuffer",
            .packageName = RASTER_PACKAGED "make_framebuffer",
            .category = Raster::DefaultNodeCategories::s_utilities
        };
    }
}