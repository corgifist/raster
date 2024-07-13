#include "compositor/compositor.h"

namespace Raster {
    std::optional<Framebuffer> Compositor::primaryFramebuffer;
    float Compositor::previewResolutionScale = 1.0f;

    std::unordered_map<int, RenderableBundle> Compositor::s_bundles;
    std::vector<CompositorTarget> Compositor::s_targets;
    Pipeline Compositor::s_pipeline;

    void Compositor::Initialize() {
        s_pipeline = GPU::GeneratePipeline(
            GPU::GenerateShader(ShaderType::Vertex, "compositor/shader"),
            GPU::GenerateShader(ShaderType::Fragment, "compositor/shader")
        );
    }

    void Compositor::PerformComposition() {
        if (primaryFramebuffer.has_value()) {
            auto& framebuffer = primaryFramebuffer.value();
            GPU::BindFramebuffer(framebuffer);
            GPU::BindPipeline(s_pipeline);
            for (auto& target : s_targets) {
                GPU::BindTextureToShader(s_pipeline.fragment, "uColor", target.colorAttachment, 0);
                GPU::BindTextureToShader(s_pipeline.fragment, "uUV", target.uvAttachment, 1);
                GPU::SetShaderUniform(s_pipeline.fragment, "uResolution", {framebuffer.width, framebuffer.height});
                std::cout << "target rendering" << std::endl;
                GPU::DrawArrays(3);
            }
            s_targets.clear();
        }
    }

    void Compositor::ResizePrimaryFramebuffer(glm::vec2 t_resolution) {
        if (primaryFramebuffer.has_value()) {
            auto& framebuffer = primaryFramebuffer.value();
            GPU::DestroyTexture(framebuffer.attachments[0]);
            GPU::DestroyFramebuffer(framebuffer);
        }

        primaryFramebuffer = GenerateCompatibleFramebuffer(t_resolution);
        std::cout << "resized primary framebuffer " << t_resolution.x << "x" << t_resolution.y << std::endl;
    }

    void Compositor::EnsureResolutionConstraints() {
        if (Workspace::s_project.has_value()) {
            if (!primaryFramebuffer.has_value()) {
                ResizePrimaryFramebuffer(GetRequiredResolution());
            }
            auto requiredResolution = GetRequiredResolution();
            auto framebuffer = primaryFramebuffer.value();
            if (requiredResolution.x != framebuffer.width || requiredResolution.y != framebuffer.height) {
                ResizePrimaryFramebuffer(requiredResolution);
            }

            GPU::BindFramebuffer(framebuffer);
            auto& bg = Workspace::s_project.value().backgroundColor;
            GPU::ClearFramebuffer(bg.r, bg.g, bg.b, bg.a);
            GPU::BindFramebuffer(std::nullopt);
        }        
    }

    Framebuffer Compositor::GenerateCompatibleFramebuffer(glm::vec2 t_resolution) {
        return GPU::GenerateFramebuffer(t_resolution.x, t_resolution.y, std::vector<Texture>{
            GPU::GenerateTexture(t_resolution.x, t_resolution.y, TexturePrecision::Usual),
            GPU::GenerateTexture(t_resolution.x, t_resolution.y, TexturePrecision::Usual)
        });
    }

    glm::vec2 Compositor::GetRequiredResolution() {
        if (Workspace::s_project.has_value()) {
            auto& project = Workspace::s_project.value();
            return project.preferredResolution * previewResolutionScale;
        }
        return glm::vec2();
    }
};
