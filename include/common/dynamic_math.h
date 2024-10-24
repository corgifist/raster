#pragma once

#include "raster.h"

namespace Raster {
    struct DynamicMath {
        static std::optional<std::any> Sine(std::any t_value);
        static std::optional<std::any> Abs(std::any t_value);
        static std::optional<std::any> Trunc(std::any t_value);
        static std::optional<std::any> Ceil(std::any t_value);
        static std::optional<std::any> Floor(std::any t_value);
        static std::optional<std::any> Sign(std::any t_value);
        static std::optional<std::any> Negate(std::any t_value);

        static std::optional<std::any> Multiply(std::any t_a, std::any t_b);
        static std::optional<std::any> Divide(std::any t_a, std::any t_b);
        static std::optional<std::any> Subtract(std::any t_a, std::any t_b);
        static std::optional<std::any> Add(std::any t_a, std::any t_b);

        static std::optional<std::any> Posterize(std::any t_a, std::any t_b);

        static std::optional<std::any> Mix(std::any t_a, std::any t_b, std::any t_percentage);
    };
};