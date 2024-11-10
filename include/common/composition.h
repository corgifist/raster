#pragma once

#include "raster.h"
#include "typedefs.h"
#include "node_base.h"
#include "attributes.h"

namespace Raster {
    struct Composition {
        int id;
        std::string name, description;
        std::unordered_map<int, AbstractNode> nodes;
        std::vector<AbstractAttribute> attributes;
        float beginFrame, endFrame;
        std::string blendMode;
        float opacity;
        int opacityAttributeID;
        bool enabled;
        uint32_t colorMark;
        bool audioEnabled;

        Composition();
        Composition(Json data);

        float GetOpacity(bool* attributeOpacityUsed = nullptr, bool* correctOpacityTypeUsed = nullptr);

        void Traverse(ContextData t_context = {});
        void OnTimelineSeek();

        std::vector<int> GetUsedAudioBuses();

        Json Serialize();
    };
};