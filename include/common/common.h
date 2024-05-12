#include "raster.h"

namespace Raster {
    struct GenericPin {
        int linkID, pinID, connectedPinID;
    };
    struct NodeBase {
        int nodeID;
        std::vector<GenericPin> inputPins, outputPins;
    };

    struct Randomizer {
        static int GetRandomInteger();

        static std::random_device s_random_device;
        static std::mt19937 s_random;
        static std::uniform_int_distribution<std::mt19937::result_type> s_distribution;
    };

}