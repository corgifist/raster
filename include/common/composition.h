#pragma once

#include "raster.h"
#include "typedefs.h"
#include "node_base.h"
#include "attributes.h"
#include "composition_mask.h"

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

        // true if composition was seeked to it's beginning
        // false otherwise
        bool identityState;

        // -1, if locked composition ID is specified, then the composition
        // will automatically set beginFrame and endFrame to the beginFrame and endFrame
        // of composition with ID that's equals to lockedCompositionID
        int lockedCompositionID; 

        std::vector<CompositionMask> masks;

        // changes when cutting compositions
        float cutTimeOffset;

        Composition();
        Composition(Json data);

        float GetOpacity(bool* attributeOpacityUsed = nullptr, bool* correctOpacityTypeUsed = nullptr);

        void Traverse(ContextData t_context = {});
        void OnTimelineSeek();

        std::vector<int> GetUsedAudioBuses();
        bool DoesAudioMixing();
        bool DoesRendering();

        Json Serialize();
    };
};