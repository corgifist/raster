#include "compositor/blending.h"

namespace Raster {

    BlendingMode::BlendingMode() {}
    BlendingMode::BlendingMode(Json mode) {
        this->name = mode["Name"];
        this->codename = mode["CodeName"];
        this->formula = mode["Formula"];
        this->functions = mode.contains("Functions") ? mode["Functions"] : "";
        this->icon = mode.contains("Icon") ? mode["Icon"] : "";
    }

    Json BlendingMode::Serialize() {
        return {
            {"Name",name},
            {"CodeName", codename},
            {"Formula", formula},
            {"Functions", functions},
            {"Icon", icon}
        };
    }

    Blending::Blending() {
        
    }

    Blending::Blending(Json t_json) {
        for (auto& mode : t_json["BlendingModes"]) {
            modes.push_back(BlendingMode(mode));
        }
    }

    void Blending::GenerateBlendingPipeline() {
        std::string accumulatedFunctions = "\n";
        std::string accumulatedCode = "\n";
        int modeIndex = 0;
        for (auto& mode : modes) {
            accumulatedCode += FormatString("\tif (uBlendMode == %i) return (%s);\n", modeIndex, mode.formula.c_str());
            std::string functionsPath = GPU::GetShadersPath() + mode.functions + ".frag";
            if (std::filesystem::exists(functionsPath)) {
                accumulatedFunctions += "\n" + ReadFile(functionsPath) + "\n";
            }
            modeIndex++;
        }

        std::string codeBase = ReadFile(GPU::GetShadersPath() + "compositor/blending_base.frag");
        codeBase = ReplaceString(codeBase, RASTER_BLENDING_PLACEHOLDER, accumulatedCode);
        codeBase = ReplaceString(codeBase, RASTER_BLENDING_FUNCTIONS_PLACEHOLDER, accumulatedFunctions);
        WriteFile(GPU::GetShadersPath() + "compositor/blending.frag", codeBase);

        pipelineCandidate = GPU::GeneratePipeline(
            GPU::GenerateShader(ShaderType::Vertex, "compositor/blending"),
            GPU::GenerateShader(ShaderType::Fragment, "compositor/blending")
        );
    }

    Framebuffer Blending::PerformBlending(BlendingMode& mode, Texture base, Texture blend, float opacity) {
        if (!pipelineCandidate.has_value()) return Framebuffer{};
        EnsureResolutionConstraints(base);

        auto& framebuffer = framebufferCandidate.value();
        auto& pipeline = pipelineCandidate.value();
        GPU::BindFramebuffer(framebuffer);
        GPU::BindPipeline(pipeline);
        GPU::ClearFramebuffer(0, 0, 0, 1);
        GPU::SetShaderUniform(pipeline.fragment, "uResolution", {base.width, base.height});
        GPU::SetShaderUniform(pipeline.fragment, "uBlendMode", GetModeIndexByCodeName(mode.codename).value());
        GPU::SetShaderUniform(pipeline.fragment, "uOpacity", opacity);

        GPU::BindTextureToShader(pipeline.fragment, "uBase", base, 0);
        GPU::BindTextureToShader(pipeline.fragment, "uBlend", blend, 1);
        GPU::DrawArrays(3);

        return framebuffer;
    }

    void Blending::EnsureResolutionConstraints(Texture& texture) {
        bool mustReinitialize = !framebufferCandidate.has_value();
        if (!mustReinitialize && framebufferCandidate.has_value()) {
            auto& framebuffer = framebufferCandidate.value();
            if (framebuffer.width != texture.width || framebuffer.height != texture.height) {
                mustReinitialize = true;
            }
        }

        if (mustReinitialize) {
            if (framebufferCandidate.has_value()) {
                auto& framebuffer = framebufferCandidate.value();
                GPU::DestroyTexture(framebuffer.attachments[0]);
                GPU::DestroyFramebuffer(framebuffer);
            }

            framebufferCandidate = GPU::GenerateFramebuffer(texture.width, texture.height, {
                GPU::GenerateTexture(texture.width, texture.height)
            });
        }
    }

    std::optional<int> Blending::GetModeIndexByCodeName(std::string t_codename) {
        int index = 0;
        for (auto& mode : modes) {
            if (mode.codename == t_codename) return index;
            index++;
        }
        return std::nullopt;
    }

    std::optional<BlendingMode> Blending::GetModeByCodeName(std::string t_codename) {
        for (auto& mode : modes) {
            if (mode.codename == t_codename) return mode;
        }
        return std::nullopt;
    }

    Json Blending::Serialize() {
        Json result = {
            {"BlendingModes", {}}
        };
        for (auto& mode : modes) {
            result["BlendingModes"].push_back(mode.Serialize());
        }
        return result;
    }
};