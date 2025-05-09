#pragma once

#include "raster.h"
#include "common/localization.h"

namespace Raster {
    struct GPUInfo {
        std::string renderer;
        std::string version;
        int maxTextureSize;
        int maxViewportX, maxViewportY;

        void* display;
    };

    enum class TexturePrecision {
        Full, // RGBA32F
        Half, // RGBA16F
        Usual, // RGBA8
    };

    enum class TextureDimensions {
        _2D, _3D
    };

    enum class ShaderType {
        Vertex, Fragment, Compute
    };

    enum class TextureWrappingAxis {
        S, T
    };

    enum class TextureWrappingMode {
        Repeat, MirroredRepeat, ClampToEdge, ClampToBorder
    };

    enum TextureFilteringOperation {
        Magnify, Minify
    };

    enum TextureFilteringMode {
        Linear, Nearest
    };

    struct Sampler {
        void* handle;
        TextureWrappingMode sMode, tMode;
        TextureFilteringMode magnifyMode, minifyMode;

        Sampler();
        Sampler(uint64_t handle);
    };

    struct Texture {
        uint32_t width, height;
        int channels;
        TexturePrecision precision;
        TextureDimensions dimensions;
        int depth;
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

        std::string GetShortPrecisionInfo() {
            switch (channels) {
                case 1: {
                    if (precision == TexturePrecision::Usual) return "R8";
                    if (precision == TexturePrecision::Half) return "R16F";
                    if (precision == TexturePrecision::Full) return "R32F";
                    break;
                }
                case 2: {
                    if (precision == TexturePrecision::Usual) return "RG8";
                    if (precision == TexturePrecision::Half) return "RG16F";
                    if (precision == TexturePrecision::Full) return "RG32F";
                    break;
                }
                case 3: {
                    if (precision == TexturePrecision::Usual) return "RGB8";
                    if (precision == TexturePrecision::Half) return "RGB16F";
                    if (precision == TexturePrecision::Full) return "RGB32F";
                    break;
                }
                case 4: {
                    if (precision == TexturePrecision::Usual) return "RGBA8";
                    if (precision == TexturePrecision::Half) return "RGBA16F";
                    if (precision == TexturePrecision::Full) return "RGBA32F";
                    break;
                }
            }
            return "?";
        }
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

        Shader();
        Shader(ShaderType type, void* handle);
    };

    struct Pipeline {
        Shader vertex, fragment, compute;
        void* handle;
    };

    enum class ArrayBufferUsage {
        Static, Dynamic
    };

    enum ArrayBufferType {
        Typical, ShaderStorageBuffer
    };

    struct ArrayBuffer {
        void* handle;
        size_t size;
        ArrayBufferUsage usage;
        ArrayBufferType type;

        ArrayBuffer() : handle(nullptr), size(0), usage(ArrayBufferUsage::Static), type(ArrayBufferType::Typical) {}
    };

    struct GPU {
        static GPUInfo info;
        static Shader s_basicShader;
        static Pipeline s_kernelPreviewPipeline;
        static Texture s_imageConvolutionPreviewTexture;

        static void Initialize();
        static void SetRenderingFunction(std::function<void()> t_function);
        static void StartRenderingThread();
        static bool MustTerminate();
        static void BeginFrame();
        static void EndFrame();
        static void InitializeImGui();

        static void* ReserveContext();
        static void DestroyContext(void* context);
        static void SetupContextState();
        static void SetCurrentContext(void* context);

        static void Flush();
        
        static double GetTime();

        static std::vector<std::string> GetDragDropPaths();

        static void EnableClipping();
        static void DisableClipping();
        static void SetClipRect(glm::vec2 upperLeft, glm::vec2 bottomRight);

        static Texture ImportTexture(const char* path);
        static Texture GenerateTexture(uint32_t width, uint32_t height, int channels, TexturePrecision precision = TexturePrecision::Usual, bool mipmapped = false, TextureDimensions dimensions = TextureDimensions::_2D, int depth = 1);
        static void GenerateMipmaps(Texture texture);
        static void UpdateTexture(Texture texture, uint32_t x, uint32_t y, uint32_t w, uint32_t h, int channels, void* pixels, int z = 0);
        static void DestroyTexture(Texture texture);
        static void BindTextureToShader(Shader shader, std::string name, Texture texture, int unit);
        static void BlitTexture(Texture base, Texture blit);

        static Framebuffer GenerateFramebuffer(uint32_t width, uint32_t height, std::vector<Texture> attachments);
        static void DestroyFramebuffer(Framebuffer fbo);
        static void DestroyFramebufferWithAttachments(Framebuffer fbo);

        static Shader GenerateShader(ShaderType type, std::string name, bool useBinaryCache = true);

        // TODO: Implement compute pipeline
        static Pipeline GeneratePipeline(Shader vertexShader, Shader fragmentShader);
        static void BindPipeline(Pipeline pipeline);

        static int GetShaderUniformLocation(Shader shader, std::string name);

        static void SetShaderUniform(Shader shader, std::string name, int i);
        static void SetShaderUniform(Shader shader, std::string name, glm::vec4 vec);
        static void SetShaderUniform(Shader shader, std::string name, glm::vec3 vec);
        static void SetShaderUniform(Shader shader, std::string name, glm::vec2 vec);
        static void SetShaderUniform(Shader shader, std::string name, float f);
        static void SetShaderUniform(Shader shader, std::string name, glm::mat2 mat);
        static void SetShaderUniform(Shader shader, std::string name, glm::mat3 mat);
        static void SetShaderUniform(Shader shader, std::string name, glm::mat4 mat);
        static void SetShaderUniform(Shader shader, std::string name, int size, float* f);
        static void SetShaderUniform(Shader shader, std::string name, int size, int* i);

        static void DrawArrays(int count);

        // reads pixels from currently bound framebuffer into void* data
        static void ReadPixels(int x, int y, int w, int h, int channels, TexturePrecision texturePrecision, void* data);

        static ArrayBuffer GenerateBuffer(size_t size, ArrayBufferType type = ArrayBufferType::Typical, ArrayBufferUsage usage = ArrayBufferUsage::Static);
        static void DestroyBuffer(ArrayBuffer& buffer);
        static void FillBuffer(ArrayBuffer& buffer, size_t offset, size_t size, void* data);
        static void BindBufferBase(ArrayBuffer& buffer, int binding);
        
        static void BindFramebuffer(std::optional<Framebuffer> fbo);
        static void ClearFramebuffer(float r, float g, float b, float a);
        static void BlitFramebuffer(Framebuffer target, Texture texture, int attachment = 0);

        static Sampler GenerateSampler(); 
        static void BindSampler(std::optional<Sampler> sampler, int unit = 0);
        static void SetSamplerTextureFilteringMode(Sampler& sampler, TextureFilteringOperation operation, TextureFilteringMode mode);
        static void SetSamplerTextureWrappingMode(Sampler& sampler, TextureWrappingAxis axis, TextureWrappingMode mode);
        static void DestroySampler(Sampler& sampler);

        static void DrawLines(std::vector<glm::vec2> points, std::vector<glm::vec4> colors, float width, glm::vec2 viewportSize, float antialiasing);

        static std::string TextureFilteringOperationToString(TextureFilteringOperation operation) {
            switch (operation) {
                case TextureFilteringOperation::Magnify: return "Magnify";
                default: return "Minify";
            }
        }

        static std::string TextureWrappingAxisToString(TextureWrappingAxis axis) {
            switch (axis) {
                case TextureWrappingAxis::S: return "S";
                default: return "T";
            }
        }


        static std::string TextureFilteringModeToString(TextureFilteringMode mode) {
            switch (mode) {
                case TextureFilteringMode::Linear: return "Linear";
                default: return "Nearest";
            }
        }

        static std::string TextureWrappingModeToString(TextureWrappingMode mode) {
            switch (mode) {
                case TextureWrappingMode::ClampToBorder: return "ClampToBorder";
                case TextureWrappingMode::ClampToEdge: return "ClampToEdge";
                case TextureWrappingMode::MirroredRepeat: return "MirroredRepeat";
                default: return "Repeat";
            }
        }


        static void DestroyShader(Shader shader);
        static void DestroyPipeline(Pipeline pipeline);

        // e.g. "shaders/api/"
        static std::string GetShadersPath();

        static void Terminate();

        static void SetWindowTitle(std::string title);

        static void* GetImGuiContext();
        static void* GetNFDWindowHandle(void* window);
    };
}