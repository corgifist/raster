#pragma once
#include "raster.h"

#define ATTRIBUTE_TIMELINE_PAYLOAD "ATTRIBUTE_TIMELINE_PAYLOAD"

namespace Raster {
    struct UIShared {
        static float s_timelinePixelsPerFrame;
        static std::unordered_map<int, float> s_timelineAttributeHeights;
        static bool s_timelineAnykeyframeDragged;
        static bool s_timelineDragged;
    };
};