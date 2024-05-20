#include "common/common.h"

namespace Raster {
    std::random_device Randomizer::s_random_device;
    std::mt19937 Randomizer::s_random(Randomizer::s_random_device());
    std::uniform_int_distribution<std::mt19937::result_type> Randomizer::s_distribution(1, INT32_MAX - 50);

    int Randomizer::GetRandomInteger() {
        return std::abs(int(s_distribution(s_random)));
    }

    GenericPin::GenericPin(std::string t_linkedAttribute, PinType t_type) {
        this->linkID = Randomizer::GetRandomInteger();
        this->pinID = Randomizer::GetRandomInteger();
        this->connectedPinID = -1;
        this->linkedAttribute = t_linkedAttribute;
        this->type = t_type;
    }

    GenericPin::GenericPin() {
        this->linkID = this->pinID = this->connectedPinID = -1;
    }


    std::unordered_map<std::string, internalDylib> Libraries::s_registry;
}