#pragma once

#include "common/color_management.h"
#include "raster.h"

namespace Raster {
    struct Colorspace {
        std::string name;

        Colorspace() : name(ColorManagement::s_defaultColorspace) {}
        Colorspace(const char* t_name) : name(t_name) {}
        Colorspace(Json t_data) : name(t_data) {}

        Json Serialize() { return name; }
    };
};