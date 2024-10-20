#include "common/ui_shared.h"


namespace Raster {
    float UIShared::s_timelinePixelsPerFrame = 0;
    std::unordered_map<int, float> UIShared::s_timelineAttributeHeights = {};
    bool UIShared::s_timelineAnykeyframeDragged = false;
    bool UIShared::s_timelineDragged = false;
    LastClickedObjectType UIShared::s_lastClickedObjectType = LastClickedObjectType::None;
    bool UIShared::s_timelineBlockPopup = false;
};
