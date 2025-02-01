#include "compositor/compositor.h"
#include "gpu/gpu.h"

namespace Raster {
    std::optional<DoubleBufferedFramebuffer> Compositor::primaryFramebuffer;
    float Compositor::previewResolutionScale = 1.0f;
    TexturePrecision Compositor::s_colorPrecision = TexturePrecision::Half;

    DoubleBufferedValue<std::unordered_map<int, RenderableBundle>> Compositor::s_bundles;
    SynchronizedValue<std::vector<CompositorTarget>> Compositor::s_targets;
    Blending Compositor::s_blending;
    Pipeline Compositor::s_pipeline;

    void Compositor::Initialize() {
        s_pipeline = GPU::GeneratePipeline(
            GPU::GenerateShader(ShaderType::Vertex, "compositor/shader"),
            GPU::GenerateShader(ShaderType::Fragment, "compositor/shader")
        );

        try {
            s_blending = Blending(ReadJson("blending.json"));
            s_blending.GenerateBlendingPipeline();
        } catch (std::exception& e) {
            print("failed to initialize blending pipeline!");
            print("\t" << e.what());
        }
    }

    void Compositor::PerformComposition(std::vector<int> t_allowedCompositions) {
        if (!primaryFramebuffer.has_value()) return;
        if (t_allowedCompositions.empty()) {
            PerformManualComposition(s_targets.Get(), primaryFramebuffer.value().Get());
            primaryFramebuffer.value().SwapBuffers();
            return;
        }

        std::vector<CompositorTarget> filteredTargets;
        for (auto& target : s_targets.Get()) {
            if (std::find(t_allowedCompositions.begin(), t_allowedCompositions.end(), target.compositionID) != t_allowedCompositions.end()) {
                filteredTargets.push_back(target);
            }
        }

        PerformManualComposition(filteredTargets, primaryFramebuffer.value().Get());
        primaryFramebuffer.value().SwapBuffers();
    }

    void Compositor::PerformManualComposition(std::vector<CompositorTarget> t_targets, Framebuffer& t_fbo, std::optional<glm::vec4> t_backgroundColor, std::optional<Blending> t_blending, std::optional<Pipeline> t_pipeline) {
        auto& framebuffer = t_fbo;
        auto pipeline = t_pipeline.value_or(s_pipeline);
        GPU::BindFramebuffer(framebuffer);
        GPU::BindPipeline(pipeline);
        auto& bg = t_backgroundColor.has_value() ? t_backgroundColor.value() : Workspace::s_project.value().backgroundColor;
        GPU::ClearFramebuffer(bg.r, bg.g, bg.b, bg.a);
        auto blending = t_blending.value_or(s_blending);
        for (auto& target : t_targets) {
            auto blendingModeCandidate = blending.GetModeByCodeName(target.blendMode);
            if (blendingModeCandidate.has_value()) {
                auto& blendingMode = blendingModeCandidate.value();
                auto blendedResult = blending.PerformManualBlending(blendingMode, framebuffer.attachments[0], target.colorAttachment, target.opacity, bg);
                GPU::BindFramebuffer(framebuffer);
                GPU::BindPipeline(pipeline);
                GPU::BindTextureToShader(pipeline.fragment, "uColor", blendedResult.attachments[0], 0);
                GPU::BindTextureToShader(pipeline.fragment, "uUV", target.uvAttachment, 1);
                GPU::SetShaderUniform(pipeline.fragment, "uResolution", {framebuffer.width, framebuffer.height});
                GPU::SetShaderUniform(pipeline.fragment, "uOpacity", 1.0f);
                GPU::DrawArrays(3);
            } else {
                GPU::BindTextureToShader(pipeline.fragment, "uColor", target.colorAttachment, 0);
                GPU::BindTextureToShader(pipeline.fragment, "uUV", target.uvAttachment, 1);
                GPU::SetShaderUniform(pipeline.fragment, "uOpacity", target.opacity);
                GPU::SetShaderUniform(pipeline.fragment, "uResolution", {framebuffer.width, framebuffer.height});
                GPU::DrawArrays(3);
            }
        }
    }

    void Compositor::ResizePrimaryFramebuffer(glm::vec2 t_resolution) {
        if (primaryFramebuffer.has_value()) {
            auto& framebuffer = primaryFramebuffer.value();
            framebuffer.Destroy();
        }

        primaryFramebuffer = GenerateCompatibleDoubleBufferedFramebuffer(t_resolution);
    }

    void Compositor::EnsureResolutionConstraints() {
        if (Workspace::s_project.has_value()) {
            if (!primaryFramebuffer.has_value()) {
                ResizePrimaryFramebuffer(GetRequiredResolution());
            }
            auto requiredResolution = GetRequiredResolution();
            auto framebuffer = primaryFramebuffer.value();
            if (requiredResolution.x != framebuffer.Get().width || requiredResolution.y != framebuffer.Get().height || s_colorPrecision != framebuffer.Get().attachments[0].precision) {
                ResizePrimaryFramebuffer(requiredResolution);
            }
            s_targets.Lock();
            s_targets.GetReference().clear();
            s_targets.Unlock();
        }        
    }

    Framebuffer Compositor::GenerateCompatibleFramebuffer(glm::vec2 t_resolution, std::optional<TexturePrecision> t_precision) {
        return GPU::GenerateFramebuffer(t_resolution.x, t_resolution.y, std::vector<Texture>{
            GPU::GenerateTexture(t_resolution.x, t_resolution.y, 4, t_precision.value_or(s_colorPrecision)),
            GPU::GenerateTexture(t_resolution.x, t_resolution.y, 4, TexturePrecision::Half)
        });
    }

    DoubleBufferedFramebuffer Compositor::GenerateCompatibleDoubleBufferedFramebuffer(glm::vec2 t_resolution, std::optional<TexturePrecision> t_precision) {
        return DoubleBufferedFramebuffer(t_resolution.x, t_resolution.y, t_precision.value_or(s_colorPrecision));
    }

    void Compositor::EnsureResolutionConstraintsForFramebuffer(Framebuffer& t_fbo) {
        auto requiredResolution = GetRequiredResolution();
        if (!t_fbo.handle) {
            t_fbo = GenerateCompatibleFramebuffer(requiredResolution);
            return;
        }
        if (t_fbo.width != requiredResolution.x || t_fbo.height != requiredResolution.y || t_fbo.attachments[0].precision != s_colorPrecision) {
            GPU::DestroyFramebufferWithAttachments(t_fbo);
            t_fbo = GenerateCompatibleFramebuffer(requiredResolution);
        }
    }

    void Compositor::EnsureResolutionConstraintsForFramebuffer(DoubleBufferedFramebuffer& t_fbo) {
        auto requiredResolution = GetRequiredResolution();
        if (!t_fbo.Get().handle) {
            t_fbo = GenerateCompatibleDoubleBufferedFramebuffer(requiredResolution);
            return;
        }
        if (t_fbo.Get().width != requiredResolution.x || t_fbo.Get().height != requiredResolution.y || t_fbo.Get().attachments[0].precision != s_colorPrecision) {
            t_fbo.Destroy();
            t_fbo = GenerateCompatibleDoubleBufferedFramebuffer(requiredResolution);
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
