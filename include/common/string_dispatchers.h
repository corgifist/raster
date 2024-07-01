#pragma once

#include "raster.h"
#include "dylib.hpp"
#include "font/IconsFontAwesome5.h"

namespace Raster {
    struct StringDispatchers {
        static void DispatchStringValue(std::any& t_attribute);
        static void DispatchTextureValue(std::any& t_attribute);
        static void DispatchFloatValue(std::any& t_attribute);
    };
};