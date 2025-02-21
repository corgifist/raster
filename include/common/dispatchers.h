#pragma once

#include "raster.h"
#include "common/common.h"
#include "dylib.hpp"
#include "font/IconsFontAwesome5.h"
#include "attribute_metadata.h"

#include "common/typedefs.h"

namespace Raster {
    struct Dispatchers {
        static PropertyDispatchersCollection s_propertyDispatchers;
        static StringDispatchersCollection s_stringDispatchers;
        static PreviewDispatchersCollection s_previewDispatchers;
        static OverlayDispatchersCollection s_overlayDispatchers;
        static ConversionDispatchersCollection s_conversionDispatchers;

        static bool s_enableOverlays;
        static bool s_editingROI;
        static bool s_blockPopups;

        // Used in `Node Properties` window to generate a UI to change some attribute's value
        // Pass metadata objects to modify the behaviour of sliders & drags
        // Possible metadata object types:
        //   SliderRangeMetadata, FormatStringMetadata, SliderStepMetadata, Vec4ColorPickerMetadata
        static void DispatchProperty(NodeBase* t_owner, std::string t_attrbute, std::any& t_value, bool t_isAttributeExposed, std::vector<std::any> t_metadata = {});

        // Used to display a preview of some value in a tooltip
        static void DispatchString(std::any& t_attribute);

        // Used in `Rendering` window to display a preview of some value
        static void DispatchPreview(std::any& t_attribute);

        // Generates an overlay over a `DispatchPreview` UI
        // Can be useful for rendering some sorts of gizmos and bounds of objects
        static bool DispatchOverlay(std::any& t_attribute, Composition* t_composition, int t_attributeID, float t_zoom, glm::vec2 t_regionSize);

        // Converts one std::any into another std::any (e.g float -> int; AttributeID -> int; AssetID -> int)
        static std::optional<std::any> DispatchConversion(std::any& t_value, std::type_index t_targetType);

        // initialize all dispatchers
        static void Initialize();
    };
};