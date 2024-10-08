#include "compositor/compositor.h"

namespace Raster {
    std::optional<Framebuffer> Compositor::primaryFramebuffer;
    float Compositor::previewResolutionScale = 1.0f;

    std::unordered_map<int, RenderableBundle> Compositor::s_bundles;
    std::vector<CompositorTarget> Compositor::s_targets;
    Blending Compositor::s_blending;
    Pipeline Compositor::s_pipeline;

    void Compositor::Initialize() {
        s_pipeline = GPU::GeneratePipeline(
            GPU::GenerateShader(ShaderType::Vertex, "compositor/shader"),
            GPU::GenerateShader(ShaderType::Fragment, "compositor/shader")
        );

        s_blending = Blending(ReadJson("misc/blending.json"));
        s_blending.GenerateBlendingPipeline();
    }

    void Compositor::PerformComposition(std::vector<int> t_allowedCompositions) {
        if (!primaryFramebuffer.has_value()) return;
        if (t_allowedCompositions.empty()) {
            PerformManualComposition(s_targets, primaryFramebuffer.value());
            return;
        }

        std::vector<CompositorTarget> filteredTargets;
        for (auto& target : s_targets) {
            if (std::find(t_allowedCompositions.begin(), t_allowedCompositions.end(), target.compositionID) != t_allowedCompositions.end()) {
                filteredTargets.push_back(target);
            }
        }

        PerformManualComposition(filteredTargets, primaryFramebuffer.value());
    }

    void Compositor::PerformManualComposition(std::vector<CompositorTarget> t_targets, Framebuffer& t_fbo, std::optional<glm::vec4> t_backgroundColor) {
        auto& framebuffer = t_fbo;
        GPU::BindFramebuffer(framebuffer);
        GPU::BindPipeline(s_pipeline);
        auto& bg = t_backgroundColor.has_value() ? t_backgroundColor.value() : Workspace::s_project.value().backgroundColor;
        GPU::ClearFramebuffer(bg.r, bg.g, bg.b, bg.a);
        for (auto& target : t_targets) {
            auto blendingModeCandidate = s_blending.GetModeByCodeName(target.blendMode);
            if (blendingModeCandidate.has_value()) {
                auto& blendingMode = blendingModeCandidate.value();
                auto blendedResult = s_blending.PerformManualBlending(blendingMode, framebuffer.attachments[0], target.colorAttachment, target.opacity, bg);
                GPU::BindFramebuffer(framebuffer);
                GPU::BindPipeline(s_pipeline);
                GPU::BindTextureToShader(s_pipeline.fragment, "uColor", blendedResult.attachments[0], 0);
                GPU::BindTextureToShader(s_pipeline.fragment, "uUV", target.uvAttachment, 1);
                GPU::SetShaderUniform(s_pipeline.fragment, "uResolution", {framebuffer.width, framebuffer.height});
                GPU::SetShaderUniform(s_pipeline.fragment, "uOpacity", 1.0f);
                GPU::DrawArrays(3);
            } else {
                GPU::BindTextureToShader(s_pipeline.fragment, "uColor", target.colorAttachment, 0);
                GPU::BindTextureToShader(s_pipeline.fragment, "uUV", target.uvAttachment, 1);
                GPU::SetShaderUniform(s_pipeline.fragment, "uOpacity", target.opacity);
                GPU::SetShaderUniform(s_pipeline.fragment, "uResolution", {framebuffer.width, framebuffer.height});
                GPU::DrawArrays(3);
            }
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
            s_targets.clear();
        }        
    }

    Framebuffer Compositor::GenerateCompatibleFramebuffer(glm::vec2 t_resolution) {
        return GPU::GenerateFramebuffer(t_resolution.x, t_resolution.y, std::vector<Texture>{
            GPU::GenerateTexture(t_resolution.x, t_resolution.y, 4, TexturePrecision::Usual),
            GPU::GenerateTexture(t_resolution.x, t_resolution.y, 4, TexturePrecision::Usual)
        });
    }

    void Compositor::EnsureResolutionConstraintsForFramebuffer(Framebuffer& t_fbo) {
        auto requiredResolution = GetRequiredResolution();
        if (!t_fbo.handle) {
            t_fbo = GenerateCompatibleFramebuffer(requiredResolution);
            return;
        }
        if (t_fbo.width != requiredResolution.x || t_fbo.height != requiredResolution.y) {
            for (auto& attachment : t_fbo.attachments) {
                GPU::DestroyTexture(attachment);
            }
            GPU::DestroyFramebuffer(t_fbo);
            t_fbo = GenerateCompatibleFramebuffer(requiredResolution);
        }
    }

    glm::vec2 Compositor::GetRequiredResolution() {
        if (Workspace::s_project.has_value()) {
            auto& project = Workspace::s_project.value();
            return glm::trunc(project.preferredResolution * previewResolutionScale);
        }
        return glm::vec2();
    }
};
