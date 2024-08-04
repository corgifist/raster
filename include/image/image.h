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
        ImagePrecision precision;
        std::vector<uint8_t> data;
        uint32_t width; uint32_t height;
        int channels;

        Image();
    };

    struct ImageLoader {
        static std::optional<Image> Load(std::string t_path);
    };
};