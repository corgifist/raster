#pragma once
#include "raster.h"

#define ATTRIBUTE_TIMELINE_PAYLOAD "ATTRIBUTE_TIMELINE_PAYLOAD"

namespace Raster {

    enum class LastClickedObjectType {
        Composition, Keyframe, Asset, None
    };

    struct UIShared {
        static float s_timelinePixelsPerFrame;
        static std::unordered_map<int, float> s_timelineAttributeHeights;
        static bool s_timelineAnykeyframeDragged;
        static bool s_timelineDragged;
        static LastClickedObjectType s_lastClickedObjectType;
        static bool s_timelineBlockPopup;
    };
};