#include "common/common.h"
#include "font/font.h"
#include "raster.h"

#include "make_framebuffer.h"
#include "common/generic_resolution.h"

namespace Raster {

    std::optional<Pipeline> MakeFramebuffer::s_pipeline;

    MakeFramebuffer::MakeFramebuffer() {
        NodeBase::Initialize();

        AddOutputPin("Value");

        SetupAttribute("BackgroundColor", glm::vec4(1));
        SetupAttribute("Resolution", GenericResolution());
        SetupAttribute("BackgroundTexture", Texture());
    }

    MakeFramebuffer::~MakeFramebuffer() {
        if (m_internalFramebuffer.has_value()) {
            auto& framebuffer = m_internalFramebuffer.value();
            framebuffer.Destroy();
        }
    }

    AbstractPinMap MakeFramebuffer::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};

        if (!s_pipeline.has_value()) {
            s_pipeline = GPU::GeneratePipeline(
                GPU::GenerateShader(ShaderType::Vertex, "make_framebuffer/shader"),
                GPU::GenerateShader(ShaderType::Fragment, "make_framebuffer/shader")
            );
        }

        auto backgroundColorCandidate = GetAttribute<glm::vec4>("BackgroundColor", t_contextData);
        auto resolutionCandidate = GetAttribute<glm::vec2>("Resolution", t_contextData);
        if (backgroundColorCandidate.has_value() && resolutionCandidate.has_value()) {
            auto& backgroundColor = backgroundColorCandidate.value();
            auto requiredResolution = resolutionCandidate.value() * Compositor::previewResolutionScale;
            if (!m_internalFramebuffer.has_value() || m_internalFramebuffer.value().width != (int) requiredResolution.x || m_internalFramebuffer.value().height != (int) requiredResolution.y) {
                if (m_internalFramebuffer.has_value()) {
                    auto& framebuffer = m_internalFramebuffer.value();
                    framebuffer.Destroy();
                }
                m_internalFramebuffer = Compositor::GenerateCompatibleDoubleBufferedFramebuffer(requiredResolution);
            }

            if (m_internalFramebuffer.has_value() && s_pipeline.has_value()) {
                auto backgroundTextureCandidate = GetAttribute<Texture>("BackgroundTexture", t_contextData);
                auto& framebuffer = m_internalFramebuffer.value();
                GPU::BindFramebuffer(framebuffer.Get());
                GPU::ClearFramebuffer(backgroundColor.r, backgroundColor.g, backgroundColor.b, backgroundColor.a);
                if (backgroundTextureCandidate.has_value() && backgroundTextureCandidate.value().handle) {
                    auto& pipeline = s_pipeline.value();
                    auto& texture = backgroundTextureCandidate.value();
                    GPU::BindPipeline(pipeline);
                    GPU::SetShaderUniform(pipeline.fragment, "uColor", backgroundColor);
                    GPU::SetShaderUniform(pipeline.fragment, "uResolution", glm::vec2(framebuffer.width, framebuffer.height));
                    GPU::BindTextureToShader(pipeline.fragment, "uTexture", texture, 0);
                    GPU::DrawArrays(3);
                }
                TryAppendAbstractPinMap(result, "Value", framebuffer.GetFrontFramebuffer());
            }
        }

        return result;
    }

    void MakeFramebuffer::AbstractRenderProperties() {
        RenderAttributeProperty("BackgroundColor");
        RenderAttributeProperty("Resolution");
    }

    void MakeFramebuffer::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);    
    }

    Json MakeFramebuffer::AbstractSerialize() {
        return SerializeAllAttributes();
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