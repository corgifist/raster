#pragma once

#include "raster.h"
#include "common/common.h"
#include "dylib.hpp"
#include "font/IconsFontAwesome5.h"

#include "common/typedefs.h"

namespace Raster {

    struct Dispatchers {
        static PropertyDispatchersCollection s_propertyDispatchers;
        static StringDispatchersCollection s_stringDispatchers;
        static PreviewDispatchersCollection s_previewDispatchers;
        static OverlayDispatchersCollection s_overlayDispatchers;

        static bool s_enableOverlays;

        static void DispatchProperty(NodeBase* t_owner, std::string t_attrbute, std::any& t_value, bool t_isAttributeExposed);
        static void DispatchString(std::any& t_attribute);
        static void DispatchPreview(std::any& t_attribute);
        static bool DispatchOverlay(std::any& t_attribute, Composition* t_composition, int t_attributeID, float t_zoom, glm::vec2 t_regionSize);
    };
};