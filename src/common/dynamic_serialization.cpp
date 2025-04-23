#include "common/dynamic_serialization.h"
#include "common/choice.h"
#include "common/colorspace.h"
#include "common/convolution_kernel.h"
#include "common/generic_audio_decoder.h"
#include "common/generic_resolution.h"
#include "common/gradient_1d.h"
#include "common/line2d.h"
#include "common/bezier_curve.h"
#include "raster.h"
#include "common/transform3d.h"

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
        RASTER_TYPE_NAME(bool),
        RASTER_TYPE_NAME(GenericResolution),
        RASTER_TYPE_NAME(Gradient1D),
        RASTER_TYPE_NAME(Choice),
        RASTER_TYPE_NAME(Line2D),
        RASTER_TYPE_NAME(BezierCurve),
        RASTER_TYPE_NAME(ConvolutionKernel),
        RASTER_TYPE_NAME(Colorspace),
        RASTER_TYPE_NAME(Transform3D)
    };

    static Json SerializeInt(std::any& t_value) {
        return std::any_cast<int>(t_value);
    }

    static Json SerializeFloat(std::any& t_value) {
        return std::any_cast<float>(t_value);
    }

    static Json SerializeString(std::any& t_value) {
        return std::any_cast<std::string>(t_value);
    }

    static Json SerializeVec2(std::any& t_value) {
        auto vector = std::any_cast<glm::vec2>(t_value);
        return {
            vector.x, vector.y
        };
    }

    static Json SerializeVec3(std::any& t_value) {
        auto vector = std::any_cast<glm::vec3>(t_value);
        return {
            vector.x, vector.y, vector.z
        };
    }

    static Json SerializeVec4(std::any& t_value) {
        auto vector = std::any_cast<glm::vec4>(t_value);
        return {
            vector.x, vector.y, vector.z, vector.w
        };
    }

    static Json SerializeTransform2D(std::any& t_value) {
        auto transform = std::any_cast<Transform2D>(t_value);
        return transform.Serialize();
    }

    static Json SerializeSamplerSettings(std::any& t_value) {
        auto samplerSettings = std::any_cast<SamplerSettings>(t_value);
        return samplerSettings.Serialize();
    }

    static Json SerializeGenericAudioDecoder(std::any& t_value) {
        return std::any_cast<GenericAudioDecoder>(t_value).assetID;
    }

    static Json SerializeBool(std::any& t_value) {
        return std::any_cast<bool>(t_value);
    }

    static Json SerializeGenericResolution(std::any& t_value) {
        return std::any_cast<GenericResolution>(t_value).Serialize();
    }

    static Json SerializeGradient1D(std::any& t_value) {
        return std::any_cast<Gradient1D>(t_value).Serialize();
    }

    static Json SerializeChoice(std::any &t_value) {
        return std::any_cast<Choice>(t_value).Serialize();
    }

    static Json SerializeLine2D(std::any& t_value) {
        return std::any_cast<Line2D>(t_value).Serialize();
    }

    static Json SerializeBezierCurve(std::any& t_value) {
        return std::any_cast<BezierCurve>(t_value).Serialize();
    }

    static Json SerializeConvolutionKernel(std::any& t_value) {
        return std::any_cast<ConvolutionKernel>(t_value).Serialize();
    }

    static Json SerializeColorspace(std::any& t_value) {
        return std::any_cast<Colorspace>(t_value).Serialize();
    }

    static Json SerializeTransform3D(std::any& t_transform) {
        return std::any_cast<Transform3D>(t_transform).Serialize();
    }

    static std::any DeserializeInt(Json t_data) {
        return t_data.get<int>();
    }

    static std::any DeserializeFloat(Json t_data) {
        return t_data.get<float>();
    }

    static std::any DeserializeString(Json t_data) {
        return t_data.get<std::string>();
    }

    static std::any DeserializeVec2(Json t_data) {
        return glm::vec2(t_data[0].get<float>(), t_data[1].get<float>());
    }

    static std::any DeserializeVec3(Json t_data) {
        return glm::vec3(t_data[0].get<float>(), t_data[1].get<float>(), t_data[2].get<float>());
    }

    static std::any DeserializeVec4(Json t_data) {
        return glm::vec4(t_data[0].get<float>(), t_data[1].get<float>(), t_data[2].get<float>(), t_data[3].get<float>());
    }

    static std::any DeserializeTransform2D(Json t_data) {
        return Transform2D(t_data);
    }

    static std::any DeserializeSamplerSettings(Json t_data) {
        return SamplerSettings(t_data);
    }

    static std::any DeserializeGenericAudioDecoder(Json t_data) {
        GenericAudioDecoder decoder;
        decoder.assetID = t_data;
        return decoder;
    }

    static std::any DeserializeBool(Json t_data) {
        return t_data.get<bool>();
    }

    static std::any DeserializeGenericResolution(Json t_data) {
        return GenericResolution(t_data);
    }

    static std::any DeserializeGradient1D(Json t_data) {
        return Gradient1D(t_data);
    }

    static std::any DeserializeChoice(Json t_data) {
        return Choice(t_data);
    }

    static std::any DeserializeLine2D(Json t_data) {
        return Line2D(t_data);
    }

    static std::any DeserializeBezierCurve(Json t_data) {
        return BezierCurve(t_data);
    }

    static std::any DeserializeConvolutionKernel(Json t_data) {
        return ConvolutionKernel(t_data);
    }

    static std::any DeserializeColorspace(Json t_data) {
        return Colorspace(t_data);
    }

    static std::any DeserializeTransform3D(Json t_data) {
        return Transform3D(t_data);
    }

    std::unordered_map<std::type_index, SerializationFunction> DynamicSerialization::s_serializers = {
        {TYPE_CONTAINER(int), SerializeInt},
        {TYPE_CONTAINER(float), SerializeFloat},
        {TYPE_CONTAINER(std::string), SerializeString},
        {TYPE_CONTAINER(glm::vec2), SerializeVec2},
        {TYPE_CONTAINER(glm::vec3), SerializeVec3},
        {TYPE_CONTAINER(glm::vec4), SerializeVec4},
        {TYPE_CONTAINER(Transform2D), SerializeTransform2D},
        {TYPE_CONTAINER(SamplerSettings), SerializeSamplerSettings},
        {TYPE_CONTAINER(GenericAudioDecoder), SerializeGenericAudioDecoder},
        {TYPE_CONTAINER(bool), SerializeBool},
        {TYPE_CONTAINER(GenericResolution), SerializeGenericResolution},
        {TYPE_CONTAINER(Gradient1D), SerializeGradient1D},
        {TYPE_CONTAINER(Choice), SerializeChoice},
        {TYPE_CONTAINER(Line2D), SerializeLine2D},
        {TYPE_CONTAINER(BezierCurve), SerializeBezierCurve},
        {TYPE_CONTAINER(ConvolutionKernel), SerializeConvolutionKernel},
        {TYPE_CONTAINER(Colorspace), SerializeColorspace},
        {TYPE_CONTAINER(Transform3D), SerializeTransform3D}
    };

    std::unordered_map<std::string, DeserializationFunction> DynamicSerialization::s_deserializers = {
        {TYPE_NAME(int), DeserializeInt},
        {TYPE_NAME(float), DeserializeFloat},
        {TYPE_NAME(std::string), DeserializeString},
        {TYPE_NAME(glm::vec2), DeserializeVec2},
        {TYPE_NAME(glm::vec3), DeserializeVec3},
        {TYPE_NAME(glm::vec4), DeserializeVec4},
        {TYPE_NAME(Transform2D), DeserializeTransform2D},
        {TYPE_NAME(SamplerSettings), DeserializeSamplerSettings},
        {TYPE_NAME(GenericAudioDecoder), DeserializeGenericAudioDecoder},
        {TYPE_NAME(bool), DeserializeBool},
        {TYPE_NAME(GenericResolution), DeserializeGenericResolution},
        {TYPE_NAME(Gradient1D), DeserializeGradient1D},
        {TYPE_NAME(Choice), DeserializeChoice},
        {TYPE_NAME(Line2D), DeserializeLine2D},
        {TYPE_NAME(BezierCurve), DeserializeBezierCurve},
        {TYPE_NAME(ConvolutionKernel), DeserializeConvolutionKernel},
        {TYPE_NAME(Colorspace), DeserializeColorspace},
        {TYPE_NAME(Transform3D), DeserializeTransform3D}
    };

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