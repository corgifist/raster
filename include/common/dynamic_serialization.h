#pragma once

#include "raster.h"
#include "common/transform2d.h"
#include "common/sampler_settings.h"

namespace Raster {
    using SerializationFunction = std::function<Json(std::any&)>;
    using DeserializationFunction = std::function<std::any(Json)>;

    struct DynamicSerialization {
        static std::unordered_map<std::type_index, SerializationFunction> s_serializers;
        static std::unordered_map<std::string, DeserializationFunction> s_deserializers;

        // Serialize some value into JSON. Returns std::nullopt if serialization for the data type you passed is unsupported
        static std::optional<Json> Serialize(std::any& t_value);

        // Deserializes JSON given by DynamicSerialization::Serialize. Returns std::nullopt if deserialization is unsupported
        static std::optional<std::any> Deserialize(Json t_data);
    };
};