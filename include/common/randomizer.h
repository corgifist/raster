#pragma once
#include "raster.h"

namespace Raster {
    struct Randomizer {
        static int GetRandomInteger();

        static std::random_device s_random_device;
        static std::mt19937 s_random;
        static std::uniform_int_distribution<std::mt19937::result_type> s_distribution;
    };
};