#pragma once

#include "raster.h"
#include "typedefs.h"
#include "node_base.h"
#include "attributes/attributes.h"

namespace Raster {
    struct Composition {
        int id;
        std::string name, description;
        std::vector<AbstractNode> nodes;
        std::vector<AbstractAttribute> attributes;
        float beginFrame, endFrame;
        std::string blendMode;
        float opacity;
        int opacityAttributeID;
        bool enabled;

        Composition();
        Composition(Json data);

        float GetOpacity(bool* attributeOpacityUsed = nullptr, bool* correctOpacityTypeUsed = nullptr);

        Json Serialize();
    };
};