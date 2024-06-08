#pragma once

#include "raster.h"
#include "dylib.hpp"
#include "font/IconsFontAwesome5.h"

#include "typedefs.h"

namespace Raster {
    struct Dispatchers {
        static PropertyDispatchersCollection s_propertyDispatchers;
        static StringDispatchersCollection s_stringDispatchers;
        static PreviewDispatchersCollection s_previewDispatchers;
    };
};