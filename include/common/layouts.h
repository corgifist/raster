#pragma once
#include "raster.h"

namespace Raster {
    struct Layouts {
    public:
        static std::optional<std::string> GetRequestedLayout();
        static void RequestLayout(std::string t_path);
    };
}