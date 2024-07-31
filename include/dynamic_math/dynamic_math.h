#pragma once

#include "raster.h"

namespace Raster {
    struct DynamicMath {
        static std::optional<std::any> Sine(std::any t_value);
<<<<<<< HEAD
        static std::optional<std::any> Multiply(std::any t_a, std::any t_b);
=======
        static std::optional<std::any> Abs(std::any t_value);

        static std::optional<std::any> Multiply(std::any t_a, std::any t_b);
        static std::optional<std::any> Divide(std::any t_a, std::any t_b);
        static std::optional<std::any> Subtract(std::any t_a, std::any t_b);
        static std::optional<std::any> Add(std::any t_a, std::any t_b);
>>>>>>> 60e4df6 (Testing Git Commits)
    };
};