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

        static Json SerializeInt(std::any& t_value);
        static Json SerializeFloat(std::any& t_value);
        static Json SerializeString(std::any& t_value);
        static Json SerializeVec2(std::any& t_value);
        static Json SerializeVec3(std::any& t_value);
        static Json SerializeVec4(std::any& t_value);
        static Json SerializeTransform2D(std::any& t_value);
        static Json SerializeSamplerSettings(std::any& t_value);
        static Json SerializeGenericAudioDecoder(std::any& t_value);
        static Json SerializeBool(std::any& t_value);
        static Json SerializeGenericResolution(std::any& t_value);
        static Json SerializeGradient1D(std::any& t_value);
        static Json SerializeChoice(std::any& t_value);

        static std::any DeserializeInt(Json t_data);
        static std::any DeserializeFloat(Json t_data);
        static std::any DeserializeString(Json t_data);
        static std::any DeserializeVec2(Json t_data);
        static std::any DeserializeVec3(Json t_data);
        static std::any DeserializeVec4(Json t_data);
        static std::any DeserializeTransform2D(Json t_data);
        static std::any DeserializeSamplerSettings(Json t_data);
        static std::any DeserializeGenericAudioDecoder(Json t_data);
        static std::any DeserializeBool(Json t_data);
        static std::any DeserializeGenericResolution(Json t_data);
        static std::any DeserializeGradient1D(Json t_data);
        static std::any DeserializeChoice(Json t_data);
    };
};