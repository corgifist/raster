#pragma once

#include "raster.h"

namespace Raster {
    struct DynamicMath {
        static std::optional<std::any> Sine(std::any t_value);
        static std::optional<std::any> Multiply(std::any t_a, std::any t_b);
    };
};