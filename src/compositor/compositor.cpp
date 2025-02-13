#include "compositor/compositor.h"
#include "common/composition_mask.h"
#include "gpu/gpu.h"
#include "raster.h"

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
        std::vector<int> skipCompositions;
        for (auto& target : t_targets) {
            auto blendingModeCandidate = blending.GetModeByCodeName(target.blendMode);
            auto colorAttachment = target.colorAttachment;
            auto uvAttachment = target.uvAttachment;
            if (std::find(skipCompositions.begin(), skipCompositions.end(), target.compositionID) != skipCompositions.end()) continue;
            if (!target.masks.empty()) {
                static Framebuffer s_maskAccumulatorFramebuffer;
                static Framebuffer s_combinedMaskFramebuffer;
                static Pipeline s_maskCombinerPipeline;
                static Pipeline s_maskApplierPipeline;
                EnsureResolutionConstraintsForFramebuffer(s_maskAccumulatorFramebuffer);
                EnsureResolutionConstraintsForFramebuffer(s_combinedMaskFramebuffer);
                GPU::BindFramebuffer(s_maskAccumulatorFramebuffer);
                GPU::ClearFramebuffer(0, 0, 0, 0);
                GPU::BindFramebuffer(s_combinedMaskFramebuffer);
                GPU::ClearFramebuffer(0, 0, 0, 0);
                if (!s_maskCombinerPipeline.handle) {
                    s_maskCombinerPipeline = GPU::GeneratePipeline(
                        GPU::s_basicShader, GPU::GenerateShader(ShaderType::Fragment, "mask_combiner/shader"));
                }
                if (!s_maskApplierPipeline.handle) {
                    s_maskApplierPipeline = GPU::GeneratePipeline(
                        GPU::s_basicShader, GPU::GenerateShader(ShaderType::Fragment, "mask_applier/shader"));
                }
                bool maskWasApplied = false;
                for (auto& mask : target.masks) {
                    std::optional<CompositorTarget> maskSource = std::nullopt;
                    for (auto& sourceTarget : t_targets) {
                        if (mask.compositionID == sourceTarget.compositionID) {
                            maskSource = sourceTarget;
                            break;
                        }
                    }
                    if (!maskSource) continue;
                    if (!maskSource->colorAttachment.handle) continue;
                    skipCompositions.push_back(maskSource->compositionID);
                    maskWasApplied = true;
                    if (mask.precompose) {
                        GPU::BindFramebuffer(s_maskAccumulatorFramebuffer);
                        GPU::BindPipeline(s_maskApplierPipeline);
                        GPU::ClearFramebuffer(0, 0, 0, 0);
                        GPU::SetShaderUniform(s_maskApplierPipeline.fragment, "uResolution", glm::vec2(s_maskAccumulatorFramebuffer.width, s_maskAccumulatorFramebuffer.height));
                        GPU::BindTextureToShader(s_maskApplierPipeline.fragment, "uBase", s_combinedMaskFramebuffer.attachments[0], 0);
                        GPU::BindTextureToShader(s_maskApplierPipeline.fragment, "uMask", maskSource->colorAttachment, 1);
                        GPU::DrawArrays(3);
                        GPU::BlitTexture(s_combinedMaskFramebuffer.attachments[0], s_maskAccumulatorFramebuffer.attachments[0]);
                        continue;
                    }
                    GPU::BindPipeline(s_maskCombinerPipeline);
                    GPU::BindFramebuffer(s_maskAccumulatorFramebuffer);
                    if (mask.op != MaskOperation::Normal) {
                        GPU::ClearFramebuffer(0, 0, 0, 0);
                    }
                    GPU::SetShaderUniform(s_maskCombinerPipeline.fragment, "uResolution", glm::vec2(s_maskAccumulatorFramebuffer.width, s_maskAccumulatorFramebuffer.height));
                    GPU::SetShaderUniform(s_maskCombinerPipeline.fragment, "uOp", static_cast<int>(mask.op));
                    GPU::BindTextureToShader(s_maskCombinerPipeline.fragment, "uColor", maskSource->colorAttachment, 0);
                    GPU::BindTextureToShader(s_maskCombinerPipeline.fragment, "uBase", s_combinedMaskFramebuffer.attachments[0], 1);
                    GPU::DrawArrays(3);
                    GPU::BlitTexture(s_combinedMaskFramebuffer.attachments[0], s_maskAccumulatorFramebuffer.attachments[0]);
                }
                if (maskWasApplied) {
                    GPU::BindPipeline(s_maskApplierPipeline);
                    GPU::BindFramebuffer(s_maskAccumulatorFramebuffer);
                    GPU::ClearFramebuffer(0, 0, 0, 0);
                    GPU::SetShaderUniform(s_maskApplierPipeline.fragment, "uResolution", glm::vec2(s_maskAccumulatorFramebuffer.width, s_maskAccumulatorFramebuffer.height));
                    GPU::BindTextureToShader(s_maskApplierPipeline.fragment, "uBase", colorAttachment, 0);
                    GPU::BindTextureToShader(s_maskApplierPipeline.fragment, "uMask", s_combinedMaskFramebuffer.attachments[0], 1);
                    GPU::DrawArrays(3);
                    colorAttachment = s_maskAccumulatorFramebuffer.attachments[0];
                }
                GPU::BindFramebuffer(framebuffer);
                GPU::BindPipeline(pipeline);
            }
            if (blendingModeCandidate.has_value()) {
                auto& blendingMode = blendingModeCandidate.value();
                auto blendedResult = blending.PerformManualBlending(blendingMode, framebuffer.attachments[0], colorAttachment, target.opacity, bg);
                GPU::BindFramebuffer(framebuffer);
                GPU::BindPipeline(pipeline);
                GPU::BindTextureToShader(pipeline.fragment, "uColor", blendedResult.attachments[0], 0);
                GPU::BindTextureToShader(pipeline.fragment, "uUV", uvAttachment, 1);
                GPU::SetShaderUniform(pipeline.fragment, "uResolution", {framebuffer.width, framebuffer.height});
                GPU::SetShaderUniform(pipeline.fragment, "uOpacity", 1.0f);
                GPU::DrawArrays(3);
            } else {
                GPU::BindTextureToShader(pipeline.fragment, "uColor", colorAttachment, 0);
                GPU::BindTextureToShader(pipeline.fragment, "uUV", uvAttachment, 1);
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
