#pragma once
#include "raster.h"
#include "common/typedefs.h"
#include "common/composition.h"

namespace Raster {
    struct OverlayDispatchers {
        static std::string s_attributeName;

        static bool DispatchTransform2DValue(std::any& t_attribute, Composition* t_composition, int t_attributeID, float t_zoom, glm::vec2 t_regionSize);
        static bool DispatchLine2DValue(std::any& t_attribute, Composition* t_composition, int t_attributeID, float t_zoom, glm::vec2 t_regionSize);
        static bool DispatchROIValue(std::any& t_attribute, Composition* t_composition, int t_attributeID, float t_zoom, glm::vec2 t_regionSize);
        static bool DispatchBezierCurve(std::any& t_attribute, Composition* t_composition, int t_attributeID, float t_zoom, glm::vec2 t_regionSize);
    };
};