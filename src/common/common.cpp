#include "common/common.h"

namespace Raster {
    std::random_device Randomizer::s_random_device;
    std::mt19937 Randomizer::s_random(Randomizer::s_random_device());
    std::uniform_int_distribution<std::mt19937::result_type> Randomizer::s_distribution(1, 1000000000000);

    int Randomizer::GetRandomInteger() {
        return s_distribution(s_random);
    }
}