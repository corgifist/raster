#include "sdf_transform.h"
#include "common/node_category.h"
#include "common/workspace.h"
#include "common/transform2d.h"

namespace Raster {

    SDFTransform::SDFTransform() {
        NodeBase::Initialize();

        this->m_mixedShape = SDFShape();
        this->m_firstShapeID = -1;

        SetupAttribute("A", SDFShape());
        SetupAttribute("Transform", Transform2D());

        AddInputPin("A");

        AddOutputPin("Shape");
    }

    AbstractPinMap SDFTransform::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};

        auto aCandidate = GetShape("A", t_contextData);
        auto transformCandidate = GetAttribute<Transform2D>("Transform", t_contextData);
        if (aCandidate.has_value() && transformCandidate.has_value()) {
            auto a = aCandidate.value();
            auto transform = transformCandidate.value();
            transform.size = 1.0f / transform.size;

            auto uvPosition = transform.DecomposePosition();
            auto uvSize = transform.DecomposeSize();
            auto uvAngle = transform.DecomposeRotation();

            uvPosition.x *= -1;
            uvPosition *= 0.5f;

            TransformShapeUniforms(a, "Transform" + std::to_string(a.id));
            if (m_firstShapeID != a.id) {
                static std::optional<std::string> s_mixBase;
                if (!s_mixBase.has_value()) {
                    s_mixBase = ReadFile(GPU::GetShadersPath() + "sdf_shapes/sdf_transform.frag");
                }
                std::string mixBase = s_mixBase.value_or("");

                m_mixedShape.uniforms.clear();
                m_mixedShape.id = Randomizer::GetRandomInteger();

                m_mixedShape.distanceFunctionName = "fSDFTransform";
                m_mixedShape.distanceFunctionCode = "";
                TransformShape(a, "Transform" + std::to_string(a.id));
                m_mixedShape.distanceFunctionCode += a.distanceFunctionCode + "\n\n";

                m_mixedShape.distanceFunctionCode += mixBase + "\n\n";
                m_mixedShape.distanceFunctionCode = ReplaceString(m_mixedShape.distanceFunctionCode, "SDF_TRANSFORM_FUNCTION_PLACEHOLDER", a.distanceFunctionName);

                m_firstShapeID = a.id;
            }

            for (auto& uniform : a.uniforms) {
                m_mixedShape.uniforms.push_back(uniform);
            }

            m_mixedShape.uniforms.push_back({
                "vec2", "uSDFUvPosition", uvPosition
            });

            m_mixedShape.uniforms.push_back({
                "vec2", "uSDFUvSize", uvSize
            });

            m_mixedShape.uniforms.push_back({
                "float", "uSDFUvAngle", glm::radians(uvAngle)
            });

            TryAppendAbstractPinMap(result, "Shape", m_mixedShape);

            m_mixedShape.uniforms.clear();
        }

        return result;
    }

    void SDFTransform::TransformShape(SDFShape& t_shape, std::string t_uniqueID) {
        t_shape.distanceFunctionCode = ReplaceString(t_shape.distanceFunctionCode, "\\b" + t_shape.distanceFunctionName + "\\b", t_shape.distanceFunctionName + t_uniqueID);
        t_shape.distanceFunctionName += t_uniqueID;
    }

    void SDFTransform::TransformShapeUniforms(SDFShape& t_shape, std::string t_uniqueID) {
        for (auto& uniform : t_shape.uniforms) {
            t_shape.distanceFunctionCode = ReplaceString(t_shape.distanceFunctionCode, "\\b" + uniform.name + "\\b", uniform.name + t_uniqueID);
        }
        for (auto& uniform : t_shape.uniforms) {
            uniform.name += t_uniqueID;
        }
    }

    std::optional<SDFShape> SDFTransform::GetShape(std::string t_attribute, ContextData& t_contextData) {
        auto candidate = GetDynamicAttribute(t_attribute, t_contextData);
        if (candidate.has_value() && candidate.value().type() == typeid(SDFShape)) {
            return std::any_cast<SDFShape>(candidate.value());
        }
        return std::nullopt;
    }

    void SDFTransform::AbstractRenderProperties() {
        RenderAttributeProperty("Transform");
    }

    void SDFTransform::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json SDFTransform::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool SDFTransform::AbstractDetailsAvailable() {
        return false;
    }

    std::string SDFTransform::AbstractHeader() {
        return "SDF Transform";
    }

    std::string SDFTransform::Icon() {
        return ICON_FA_SHAPES " " ICON_FA_UP_DOWN_LEFT_RIGHT;
    }

    std::optional<std::string> SDFTransform::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::SDFTransform>();
    }

    RASTER_DL_EXPORT void OnStartup() {
        Raster::NodeCategoryUtils::RegisterCategory(ICON_FA_SHAPES, "Shapes");

        Raster::Workspace::s_typeColors[ATTRIBUTE_TYPE(Raster::SDFShape)] = RASTER_COLOR32(52, 235, 222, 255);
        Raster::Workspace::s_typeNames[ATTRIBUTE_TYPE(Raster::SDFShape)] = "SDFShape";
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "SDF Transform",
            .packageName = RASTER_PACKAGED "sdf_transform",
            .category = Raster::NodeCategoryUtils::RegisterCategory(ICON_FA_SHAPES, "Shapes")
        };
    }
}