#include "common/common.h"
#include "font/font.h"
#include "raster.h"

#include "make_framebuffer.h"
#include "../../ImGui/imgui.h"

namespace Raster {

    std::optional<Pipeline> MakeFramebuffer::s_pipeline;

    MakeFramebuffer::MakeFramebuffer() {
        NodeBase::Initialize();
        NodeBase::GenerateFlowPins();

        AddOutputPin("Value");

        this->m_attributes["BackgroundColor"] = glm::vec4(1, 1, 1, 1);
        this->m_attributes["BackgroundTexture"] = Texture();

        if (!s_pipeline.has_value()) {
            s_pipeline = GPU::GeneratePipeline(
                GPU::GenerateShader(ShaderType::Vertex, "make_framebuffer/shader"),
                GPU::GenerateShader(ShaderType::Fragment, "make_framebuffer/shader")
            );
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
                GPU::BindFramebuffer(std::nullopt);
                TryAppendAbstractPinMap(result, "Value", framebuffer);
            }
        }

        return result;
    }

    void MakeFramebuffer::AbstractRenderProperties() {
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
    Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::MakeFramebuffer>();
    }

    Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Make Framebuffer",
            .packageName = RASTER_PACKAGED_PACKAGE "make_framebuffer",
            .category = Raster::NodeCategory::Utilities
        };
    }
}