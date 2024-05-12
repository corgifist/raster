#pragma once

#include "raster.h"

namespace Raster {
    struct Font {
        static size_t s_fontSize;
        static std::vector<uint32_t> s_fontBytes;
    };
}