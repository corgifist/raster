#pragma once
#include "raster.h"
#include "typedefs.h"

namespace Raster {
    struct PreviewDispatchers {
        static void DispatchStringValue(std::any& t_attribute);
        static void DispatchTextureValue(std::any& t_attribute);
        static void DispatchFloatValue(std::any& t_attribute);
    };
};