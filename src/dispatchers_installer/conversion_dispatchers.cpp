#include "conversion_dispatchers.h"

#include "common/asset_id.h"

namespace Raster {
    std::optional<std::any> ConversionDispatchers::ConvertAssetIDToInt(std::any& t_value) {
        return std::any_cast<AssetID>(t_value).id;
    }

    std::optional<std::any> ConversionDispatchers::ConvertFloatToInt(std::any& t_value) {
        return (int) std::any_cast<float>(t_value);
    }

    std::optional<std::any> ConversionDispatchers::ConvertIntToFloat(std::any& t_value) {
        return (float) std::any_cast<int>(t_value);
    }

    std::optional<std::any> ConversionDispatchers::ConvertVec3ToVec4(std::any& t_value) {
        auto vec3 = std::any_cast<glm::vec3>(t_value);
        glm::vec4 result = glm::vec4(vec3, 1.0f);
        return result;
    }
};