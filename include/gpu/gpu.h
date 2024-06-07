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

    struct Texture {
        uint32_t width, height;
        TexturePrecision precision;
        void* handle;

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

    struct GPU {
        static GPUInfo info;

        static void Initialize();
        static bool MustTerminate();
        static void BeginFrame();
        static void EndFrame();

        static Texture ImportTexture(const char* path);

        static Texture GenerateTexture(uint32_t width, uint32_t height, TexturePrecision precision = TexturePrecision::Usual);
        static void UpdateTexture(Texture texture, uint32_t x, uint32_t y, uint32_t w, uint32_t h, void* pixels);

        static void Terminate();

        static void SetWindowTitle(std::string title);

        static void* GetImGuiContext();
    };
}