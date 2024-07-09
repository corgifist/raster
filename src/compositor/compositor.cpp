#include "compositor/compositor.h"

namespace Raster {
    std::optional<Framebuffer> Compositor::primaryFramebuffer;
    float Compositor::previewResolutionScale = 1.0f;

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
