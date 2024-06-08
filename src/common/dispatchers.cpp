#include "common/common.h"
#include "gpu/gpu.h"

namespace Raster {
    PropertyDispatchersCollection Dispatchers::s_propertyDispatchers = {
        {ATTRIBUTE_TYPE(std::string), AttributeDispatchers::DispatchStringAttribute}
    };

    StringDispatchersCollection Dispatchers::s_stringDispatchers = {
        {ATTRIBUTE_TYPE(std::string), StringDispatchers::DispatchStringValue},
        {ATTRIBUTE_TYPE(Texture), StringDispatchers::DispatchTextureValue}
    };

    PreviewDispatchersCollection Dispatchers::s_previewDispatchers = {
        {ATTRIBUTE_TYPE(std::string), PreviewDispatchers::DispatchStringValue},
        {ATTRIBUTE_TYPE(Texture), PreviewDispatchers::DispatchTextureValue}
    };
};