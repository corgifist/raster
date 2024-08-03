#pragma once

#include "raster.h"
#include "dylib.hpp"
#include "font/IconsFontAwesome5.h"

namespace Raster {
    struct StringDispatchers {
        static void DispatchStringValue(std::any& t_attribute);
        static void DispatchTextureValue(std::any& t_attribute);
        static void DispatchFloatValue(std::any& t_attribute);
        static void DispatchVector4Value(std::any& t_attribute);
        static void DispatchVector3Value(std::any& t_attribute);
        static void DispatchFramebufferValue(std::any& t_attribute);
        static void DispatchIntValue(std::any& t_attribute);
        static void DispatchSamplerSettingsValue(std::any& t_attribute);
        static void DispatchTransform2DValue(std::any& t_attribute);
        static void DispatchBoolValue(std::any& t_attribute);
    };
};