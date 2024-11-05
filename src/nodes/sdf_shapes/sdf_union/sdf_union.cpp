#include "sdf_union.h"
#include "common/node_category.h"
#include "common/workspace.h"

namespace Raster {

    SDFUnion::SDFUnion() {
        NodeBase::Initialize();

        this->m_mixedShape = SDFShape();
        this->m_firstShapeID = -1;
        this->m_secondShapeID = -1;

        SetupAttribute("A", SDFShape());
        SetupAttribute("B", SDFShape());
        SetupAttribute("Smoothness", 0.1f);
        SetupAttribute("Smooth", true);

        AddInputPin("A");
        AddInputPin("B");

        AddOutputPin("Shape");
    }

    AbstractPinMap SDFUnion::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};

        auto aCandidate = GetShape("A", t_contextData);
        auto bCandidate = GetShape("B", t_contextData);
        auto smoothCandidate = GetAttribute<bool>("Smooth", t_contextData);
        auto smoothnessCandidate = GetAttribute<float>("Smoothness", t_contextData);
        if (aCandidate.has_value() && bCandidate.has_value() && smoothCandidate.has_value() && smoothnessCandidate.has_value()) {
            auto a = aCandidate.value();
            auto b = bCandidate.value();
            auto smooth = smoothCandidate.value();
            auto smoothness = smoothnessCandidate.value();

            TransformShapeUniforms(a, "Union1");
            TransformShapeUniforms(b, "Union2");
            if (m_firstShapeID != a.id || m_secondShapeID != b.id) {
                static std::optional<std::string> s_unionBase;
                if (!s_unionBase.has_value()) {
                    s_unionBase = ReadFile(GPU::GetShadersPath() + "sdf_shapes/sdf_union.frag");
                }
                std::string mixBase = s_unionBase.value_or("");

                m_mixedShape.uniforms.clear();
                m_mixedShape.id = Randomizer::GetRandomInteger();

                m_mixedShape.distanceFunctionName = "fSDFUnion";
                m_mixedShape.distanceFunctionCode = "";
                TransformShape(a, "Union" + std::to_string(a.id));
                TransformShape(b, "Union" + std::to_string(b.id));
                m_mixedShape.distanceFunctionCode += a.distanceFunctionCode + "\n\n";
                m_mixedShape.distanceFunctionCode += b.distanceFunctionCode + "\n\n";

                m_mixedShape.distanceFunctionCode += mixBase + "\n\n";
                m_mixedShape.distanceFunctionCode = ReplaceString(m_mixedShape.distanceFunctionCode, "SDF_UNION_FIRST_FUNCTION_PLACEHOLDER", a.distanceFunctionName);
                m_mixedShape.distanceFunctionCode = ReplaceString(m_mixedShape.distanceFunctionCode, "SDF_UNION_SECOND_FUNCTION_PLACEHOLDER", b.distanceFunctionName);

                m_firstShapeID = a.id;
                m_secondShapeID = b.id;
            }

            for (auto& uniform : a.uniforms) {
                m_mixedShape.uniforms.push_back(uniform);
            }

            for (auto& uniform : b.uniforms) {
                m_mixedShape.uniforms.push_back(uniform);
            }

            m_mixedShape.uniforms.push_back({
                "bool", "uSDFUnionSmooth", smooth,
            });
            m_mixedShape.uniforms.push_back({
                "float", "uSDFUnionSmoothness", smoothness
            });

            TryAppendAbstractPinMap(result, "Shape", m_mixedShape);

            m_mixedShape.uniforms.clear();
        }

        return result;
    }

    void SDFUnion::TransformShape(SDFShape& t_shape, std::string t_uniqueID) {
        t_shape.distanceFunctionCode = ReplaceString(t_shape.distanceFunctionCode, "\\b" + t_shape.distanceFunctionName + "\\b", t_shape.distanceFunctionName + t_uniqueID);
        t_shape.distanceFunctionName += t_uniqueID;
    }

    void SDFUnion::TransformShapeUniforms(SDFShape& t_shape, std::string t_uniqueID) {
        for (auto& uniform : t_shape.uniforms) {
            t_shape.distanceFunctionCode = ReplaceString(t_shape.distanceFunctionCode, "\\b" + uniform.name + "\\b", uniform.name + t_uniqueID);
        }
        for (auto& uniform : t_shape.uniforms) {
            uniform.name += t_uniqueID;
        }
    }

    std::optional<SDFShape> SDFUnion::GetShape(std::string t_attribute, ContextData& t_contextData) {
        auto candidate = GetDynamicAttribute(t_attribute, t_contextData);
        if (candidate.has_value() && candidate.value().type() == typeid(SDFShape)) {
            return std::any_cast<SDFShape>(candidate.value());
        }
        return std::nullopt;
    }

    void SDFUnion::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data); 
    }

    Json SDFUnion::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    void SDFUnion::AbstractRenderProperties() {
        RenderAttributeProperty("Smooth");
        RenderAttributeProperty("Smoothness", {
            SliderRangeMetadata(0, 1)
        });
    }

    bool SDFUnion::AbstractDetailsAvailable() {
        return false;
    }

    std::string SDFUnion::AbstractHeader() {
        return "SDF Union";
    }

    std::string SDFUnion::Icon() {
        return ICON_FA_SHAPES " " ICON_FA_PLUS;
    }

    std::optional<std::string> SDFUnion::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::SDFUnion>();
    }

    RASTER_DL_EXPORT void OnStartup() {
        Raster::NodeCategoryUtils::RegisterCategory(ICON_FA_SHAPES, "Shapes");

        Raster::Workspace::s_typeColors[ATTRIBUTE_TYPE(Raster::SDFShape)] = RASTER_COLOR32(52, 235, 222, 255);
        Raster::Workspace::s_typeNames[ATTRIBUTE_TYPE(Raster::SDFShape)] = "SDFShape";
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "SDF Union",
            .packageName = RASTER_PACKAGED "sdf_union",
            .category = Raster::NodeCategoryUtils::RegisterCategory(ICON_FA_SHAPES, "Shapes")
        };
    }
}