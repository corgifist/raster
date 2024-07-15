#pragma once

#include "raster.h"
#include "common/common.h"

namespace Raster {
    struct GPUInfo {
        std::string renderer;
        std::string version;

        void* display;
    };

    enum class TexturePrecision {
        Full, // RGBA32F
        Half, // RGBA16F
        Usual, // RGBA8
    };

    enum class ShaderType {
        Vertex, Fragment, Compute
    };

    struct Texture {
        uint32_t width, height;
        TexturePrecision precision;
        void* handle;

        Texture();

        std::string PrecisionToString() {
            switch (precision) {
                case TexturePrecision::Full: {
                    return Localization::GetString("FULL_PRECISION");
                }
                case TexturePrecision::Half: {
                    return Localization::GetString("HALF_PRECISION");
                }
                case TexturePrecision::Usual: {
                    return Localization::GetString("USUAL_PRECISION");
                }
            }
            return Localization::GetString("UNKNOWN_PRECISION");
        };
    };

    struct Framebuffer {
        uint32_t width, height;
        std::vector<Texture> attachments;
        void* handle;
        void* depthHandle;

        Framebuffer();
    };

    struct Shader {
        ShaderType type;
        void* handle;
    };

    struct Pipeline {
        Shader vertex, fragment, compute;
        void* handle;
    };

    struct GPU {
        static GPUInfo info;

        static void Initialize();
        static bool MustTerminate();
        static void BeginFrame();
        static void EndFrame();

        static Texture ImportTexture(const char* path);
        static Texture GenerateTexture(uint32_t width, uint32_t height, TexturePrecision precision = TexturePrecision::Usual);
        static void UpdateTexture(Texture texture, uint32_t x, uint32_t y, uint32_t w, uint32_t h, void* pixels);
        static void DestroyTexture(Texture texture);
        static void BindTextureToShader(Shader shader, std::string name, Texture texture, int unit);

        static Framebuffer GenerateFramebuffer(uint32_t width, uint32_t height, std::vector<Texture> attachments);
        static void DestroyFramebuffer(Framebuffer fbo);

        static Shader GenerateShader(ShaderType type, std::string name);

        // TODO: Implement compute pipeline
        static Pipeline GeneratePipeline(Shader vertexShader, Shader fragmentShader);
        static void BindPipeline(Pipeline pipeline);

        static int GetShaderUniformLocation(Shader shader, std::string name);

        static void SetShaderUniform(Shader shader, std::string name, int i);
        static void SetShaderUniform(Shader shader, std::string name, glm::vec4 vec);
        static void SetShaderUniform(Shader shader, std::string name, glm::vec2 vec);
        static void SetShaderUniform(Shader shader, std::string name, float f);

        static void DrawArrays(int count);
        
        static void BindFramebuffer(std::optional<Framebuffer> fbo);
        static void ClearFramebuffer(float r, float g, float b, float a);
        static void BlitFramebuffer(Framebuffer target, Texture texture);

        // e.g. "shaders/api/"
        static std::string GetShadersPath();

        static void Terminate();

        static void SetWindowTitle(std::string title);

        static void* GetImGuiContext();
    };
}