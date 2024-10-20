#pragma once

#include "raster.h"
#include "IconsFontAwesome5.h"

struct ImFont;

namespace Raster {
    struct Font {
        static size_t s_fontSize;
        static std::vector<uint32_t> s_fontBytes;

        static size_t s_fontAwesomeSize;
        static std::vector<uint32_t> s_fontAwesomeBytes;

        static std::unordered_map<std::string, std::string> s_iconMap;
        static std::string GetIcon(std::string key);

        static ImFont* s_denseFont, *s_normalFont;
    };
}