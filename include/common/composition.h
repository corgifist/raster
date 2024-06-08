#pragma once

#include "raster.h"
#include "typedefs.h"

namespace Raster {
    struct Composition {
        int id;
        std::string name, description;
        std::vector<AbstractNode> nodes;
        uint64_t beginFrame, endFrame;

        Composition();
        Composition(Json data);

        Json Serialize();
    };
};