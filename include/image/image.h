#pragma once

#include "raster.h"
#include "common/common.h"

namespace Raster {
    enum class ImagePrecision {
        Usual, // RGBA8
        Half, // RGBA16
        Full // RGBA32F
    };

    struct Image {
    public:
        ImagePrecision precision;
        std::vector<uint8_t> data;
        uint32_t width; uint32_t height;
        int channels;

        Image();
    };

    struct ImageLoader {
    public:
        static std::optional<Image> Load(std::string t_path);

        static std::string GetImplementationName();
        static std::vector<std::string> GetSupportedExtensions();
    };

    struct ImageWriter {
        static bool Write(std::string t_path, Image& t_image);
        static std::optional<std::string> GetError();
    };

    struct AsyncImageLoader {
    public:
        AsyncImageLoader();
        AsyncImageLoader(std::string t_path);

        bool IsReady();
        bool IsInitialized();
        std::optional<std::shared_ptr<Image>> Get();

    private:
        std::future<std::optional<std::shared_ptr<Image>>> m_future;
        bool m_initialized;
    };
};