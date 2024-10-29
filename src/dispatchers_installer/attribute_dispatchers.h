#pragma once

#include "raster.h"
#include "dylib.hpp"
#include "font/IconsFontAwesome5.h"
#include "common/node_base.h"

namespace Raster {
    struct AttributeDispatchers {
        static void DispatchStringAttribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed, std::vector<std::any> t_metadata);
        static void DispatchFloatAttribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed, std::vector<std::any> t_metadata);
        static void DispatchIntAttribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed, std::vector<std::any> t_metadata);
        static void DispatchVec4Attribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed, std::vector<std::any> t_metadata);
        static void DispatchVec3Attribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed, std::vector<std::any> t_metadata);
        static void DispatchVec2Attribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed, std::vector<std::any> t_metadata);
        static void DispatchTransform2DAttribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed, std::vector<std::any> t_metadata);
        static void DispatchSamplerSettingsAttribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed, std::vector<std::any> t_metadata);
        static void DispatchBoolAttribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed, std::vector<std::any> t_metadata);
        static void DispatchAssetIDAttribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed, std::vector<std::any> t_metadata);
        static void DispatchGenericAudioDecoderAttribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed, std::vector<std::any> t_metadata);
        static void DispatchAudioSamplesAttribute(NodeBase* t_owner, std::string t_attribute, std::any& t_value, bool t_isAttributeExposed, std::vector<std::any> t_metadata);
    };
};