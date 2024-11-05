#include "common/dynamic_serialization.h"
#include "common/generic_audio_decoder.h"

#define TYPE_CONTAINER(type) std::type_index(typeid(type))
#define TYPE_NAME(type) #type
#define TYPE_NAME_PAIR(type) {TYPE_NAME(type), #type}

namespace Raster {

    static std::unordered_map<std::type_index, std::string> s_typeNames = {
        RASTER_TYPE_NAME(int),
        RASTER_TYPE_NAME(float),
        RASTER_TYPE_NAME(std::string),
        RASTER_TYPE_NAME(glm::vec2),
        RASTER_TYPE_NAME(glm::vec3),
        RASTER_TYPE_NAME(glm::vec4),
        RASTER_TYPE_NAME(Transform2D),
        RASTER_TYPE_NAME(SamplerSettings),
        RASTER_TYPE_NAME(GenericAudioDecoder),
        RASTER_TYPE_NAME(bool)
    };

    std::unordered_map<std::type_index, SerializationFunction> DynamicSerialization::s_serializers = {
        {TYPE_CONTAINER(int), DynamicSerialization::SerializeInt},
        {TYPE_CONTAINER(float), DynamicSerialization::SerializeFloat},
        {TYPE_CONTAINER(std::string), DynamicSerialization::SerializeString},
        {TYPE_CONTAINER(glm::vec2), DynamicSerialization::SerializeVec2},
        {TYPE_CONTAINER(glm::vec3), DynamicSerialization::SerializeVec3},
        {TYPE_CONTAINER(glm::vec4), DynamicSerialization::SerializeVec4},
        {TYPE_CONTAINER(Transform2D), DynamicSerialization::SerializeTransform2D},
        {TYPE_CONTAINER(SamplerSettings), DynamicSerialization::SerializeSamplerSettings},
        {TYPE_CONTAINER(GenericAudioDecoder), DynamicSerialization::SerializeGenericAudioDecoder},
        {TYPE_CONTAINER(bool), DynamicSerialization::SerializeBool}
    };

    std::unordered_map<std::string, DeserializationFunction> DynamicSerialization::s_deserializers = {
        {TYPE_NAME(int), DynamicSerialization::DeserializeInt},
        {TYPE_NAME(float), DynamicSerialization::DeserializeFloat},
        {TYPE_NAME(std::string), DynamicSerialization::DeserializeString},
        {TYPE_NAME(glm::vec2), DynamicSerialization::DeserializeVec2},
        {TYPE_NAME(glm::vec3), DynamicSerialization::DeserializeVec3},
        {TYPE_NAME(glm::vec4), DynamicSerialization::DeserializeVec4},
        {TYPE_NAME(Transform2D), DynamicSerialization::DeserializeTransform2D},
        {TYPE_NAME(SamplerSettings), DynamicSerialization::DeserializeSamplerSettings},
        {TYPE_NAME(GenericAudioDecoder), DynamicSerialization::DeserializeGenericAudioDecoder},
        {TYPE_NAME(bool), DynamicSerialization::DeserializeBool}
    };

    Json DynamicSerialization::SerializeInt(std::any& t_value) {
        return std::any_cast<int>(t_value);
    }

    Json DynamicSerialization::SerializeFloat(std::any& t_value) {
        return std::any_cast<float>(t_value);
    }

    Json DynamicSerialization::SerializeString(std::any& t_value) {
        return std::any_cast<std::string>(t_value);
    }

    Json DynamicSerialization::SerializeVec2(std::any& t_value) {
        auto vector = std::any_cast<glm::vec2>(t_value);
        return {
            vector.x, vector.y
        };
    }

    Json DynamicSerialization::SerializeVec3(std::any& t_value) {
        auto vector = std::any_cast<glm::vec3>(t_value);
        return {
            vector.x, vector.y, vector.z
        };
    }

    Json DynamicSerialization::SerializeVec4(std::any& t_value) {
        auto vector = std::any_cast<glm::vec4>(t_value);
        return {
            vector.x, vector.y, vector.z, vector.w
        };
    }

    Json DynamicSerialization::SerializeTransform2D(std::any& t_value) {
        auto transform = std::any_cast<Transform2D>(t_value);
        return transform.Serialize();
    }

    Json DynamicSerialization::SerializeSamplerSettings(std::any& t_value) {
        auto samplerSettings = std::any_cast<SamplerSettings>(t_value);
        return samplerSettings.Serialize();
    }

    Json DynamicSerialization::SerializeGenericAudioDecoder(std::any& t_value) {
        return std::any_cast<GenericAudioDecoder>(t_value).assetID;
    }

    Json DynamicSerialization::SerializeBool(std::any& t_value) {
        return std::any_cast<bool>(t_value);
    }

    std::any DynamicSerialization::DeserializeInt(Json t_data) {
        return t_data.get<int>();
    }

    std::any DynamicSerialization::DeserializeFloat(Json t_data) {
        return t_data.get<float>();
    }

    std::any DynamicSerialization::DeserializeString(Json t_data) {
        return t_data.get<std::string>();
    }

    std::any DynamicSerialization::DeserializeVec2(Json t_data) {
        return glm::vec2(t_data[0].get<float>(), t_data[1].get<float>());
    }

    std::any DynamicSerialization::DeserializeVec3(Json t_data) {
        return glm::vec3(t_data[0].get<float>(), t_data[1].get<float>(), t_data[2].get<float>());
    }

    std::any DynamicSerialization::DeserializeVec4(Json t_data) {
        return glm::vec4(t_data[0].get<float>(), t_data[1].get<float>(), t_data[2].get<float>(), t_data[3].get<float>());
    }

    std::any DynamicSerialization::DeserializeTransform2D(Json t_data) {
        return Transform2D(t_data);
    }

    std::any DynamicSerialization::DeserializeSamplerSettings(Json t_data) {
        return SamplerSettings(t_data);
    }

    std::any DynamicSerialization::DeserializeGenericAudioDecoder(Json t_data) {
        GenericAudioDecoder decoder;
        decoder.assetID = t_data;
        return decoder;
    }

    std::any DynamicSerialization::DeserializeBool(Json t_data) {
        return t_data.get<bool>();
    }

    std::optional<Json> DynamicSerialization::Serialize(std::any& t_value) {
        for (auto& serializer : s_serializers) {
            if (serializer.first == std::type_index(t_value.type()) && s_typeNames.find(serializer.first) != s_typeNames.end()) {
                auto data = serializer.second(t_value);
                return Json{
                    {"Type", s_typeNames[serializer.first]},
                    {"Data", data}
                };
            }
        }
        return std::nullopt;
    }

    std::optional<std::any> DynamicSerialization::Deserialize(Json t_data) {
        if (!t_data.is_object()) return std::nullopt;
        if (!t_data.contains("Type")) return std::nullopt;
        if (!t_data.contains("Data")) return std::nullopt;

        auto type = t_data["Type"].get<std::string>();
        auto data = t_data["Data"];
        for (auto& deserializer : s_deserializers) {
            if (deserializer.first == type) {
                return deserializer.second(data);
            }
        }
        return std::nullopt;
    }
};