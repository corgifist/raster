#pragma once

#include "raster.h"

namespace Raster {
    struct ConversionDispatchers {
        static std::optional<std::any> ConvertAssetIDToInt(std::any& t_value);
        static std::optional<std::any> ConvertFloatToInt(std::any& t_value);
        static std::optional<std::any> ConvertIntToFloat(std::any& t_value);
        static std::optional<std::any> ConvertVec3ToVec4(std::any& t_value);
        static std::optional<std::any> ConvertAssetIDToTexture(std::any& t_value);
        static std::optional<std::any> ConvertGenericResolutionToVec2(std::any& t_value);
    };
};