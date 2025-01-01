#include "common/layouts.h"

namespace Raster {
    static std::optional<std::string> s_layout = std::nullopt;

    std::optional<std::string> Layouts::GetRequestedLayout() {
        auto layout = s_layout;
        s_layout = std::nullopt;
        return layout;
    }

    void Layouts::RequestLayout(std::string t_path) {
        s_layout = t_path;
    }
};