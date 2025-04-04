#include "common/randomizer.h"

namespace Raster {
    int Randomizer::GetRandomInteger() {
        int value = std::abs((int) s_distribution(s_random));
        return value;
    }
};