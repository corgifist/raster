#pragma once

#include "raster.h"
#include "dylib.hpp"
#include "font/IconsFontAwesome5.h"
#include "node_base.h"

namespace Raster {
    struct AttributeDispatchers {
        static void DispatchStringAttribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed);
        static void DispatchFloatAttribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed);
        static void DispatchIntAttribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed);
        static void DispatchVec4Attribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed);
        static void DispatchTransform2DAttribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed);
        static void DispatchSamplerSettingsAttribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed);
    };
};