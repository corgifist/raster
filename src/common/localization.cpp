#include "common/common.h"

namespace Raster {

    void Localization::Load(Json t_map) {
        Localization::m_map = t_map;
    }

    std::string Localization::GetString(std::string t_key) {
        if (m_map.find(t_key) == m_map.end()) {
            return "Unknown Key '" + t_key + "'. Patch Your Localization";
        }
        return m_map[t_key];
    }

    std::unordered_map<std::string, std::string> Localization::m_map;
};