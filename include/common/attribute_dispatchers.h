#pragma once

#include "raster.h"
#include "dylib.hpp"
#include "font/IconsFontAwesome5.h"
#include "node_base.h"

namespace Raster {
    struct AttributeDispatchers {
        static void DispatchStringAttribute(NodeBase* t_owner, std::string t_attrbute, std::any& t_value, bool t_isAttributeExposed);
    };
};