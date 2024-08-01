#pragma once
#include "raster.h"
#include "typedefs.h"

namespace Raster {
    struct Localization {
        static void Load(Json t_map);
        static std::string GetString(std::string t_key);

        protected:
        static std::unordered_map<std::string, std::string> m_map;
    };
};