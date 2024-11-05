#include "sdf_subtract.h"
#include "common/node_category.h"
#include "common/workspace.h"

namespace Raster {

    SDFSubtract::SDFSubtract() {
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

    AbstractPinMap SDFSubtract::AbstractExecute(ContextData& t_contextData) {
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

            TransformShapeUniforms(a, "Subtract" + std::to_string(a.id));
            TransformShapeUniforms(b, "Subtract" + std::to_string(b.id));
            if (m_firstShapeID != a.id || m_secondShapeID != b.id) {
                static std::optional<std::string> s_unionBase;
                if (!s_unionBase.has_value()) {
                    s_unionBase = ReadFile(GPU::GetShadersPath() + "sdf_shapes/sdf_subtract.frag");
                }
                std::string mixBase = s_unionBase.value_or("");

                m_mixedShape.uniforms.clear();
                m_mixedShape.id = Randomizer::GetRandomInteger();

                m_mixedShape.distanceFunctionName = "fSDFSubtract";
                m_mixedShape.distanceFunctionCode = "";
                TransformShape(a, "Subtract" + std::to_string(a.id));
                TransformShape(b, "Subtract" + std::to_string(b.id));
                m_mixedShape.distanceFunctionCode += a.distanceFunctionCode + "\n\n";
                m_mixedShape.distanceFunctionCode += b.distanceFunctionCode + "\n\n";

                m_mixedShape.distanceFunctionCode += mixBase + "\n\n";
                m_mixedShape.distanceFunctionCode = ReplaceString(m_mixedShape.distanceFunctionCode, "SDF_SUBTRACT_FIRST_FUNCTION_PLACEHOLDER", a.distanceFunctionName);
                m_mixedShape.distanceFunctionCode = ReplaceString(m_mixedShape.distanceFunctionCode, "SDF_SUBTRACT_SECOND_FUNCTION_PLACEHOLDER", b.distanceFunctionName);

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
                "bool", "uSDFSubtractSmooth", smooth,
            });
            m_mixedShape.uniforms.push_back({
                "float", "uSDFSubtractSmoothness", smoothness
            });

            TryAppendAbstractPinMap(result, "Shape", m_mixedShape);

            m_mixedShape.uniforms.clear();
        }

        return result;
    }

    void SDFSubtract::TransformShape(SDFShape& t_shape, std::string t_uniqueID) {
        t_shape.distanceFunctionCode = ReplaceString(t_shape.distanceFunctionCode, "\\b" + t_shape.distanceFunctionName + "\\b", t_shape.distanceFunctionName + t_uniqueID);
        t_shape.distanceFunctionName += t_uniqueID;
    }

    void SDFSubtract::TransformShapeUniforms(SDFShape& t_shape, std::string t_uniqueID) {
        for (auto& uniform : t_shape.uniforms) {
            t_shape.distanceFunctionCode = ReplaceString(t_shape.distanceFunctionCode, "\\b" + uniform.name + "\\b", uniform.name + t_uniqueID);
        }
        for (auto& uniform : t_shape.uniforms) {
            uniform.name += t_uniqueID;
        }
    }

    std::optional<SDFShape> SDFSubtract::GetShape(std::string t_attribute, ContextData& t_contextData) {
        auto candidate = GetDynamicAttribute(t_attribute, t_contextData);
        if (candidate.has_value() && candidate.value().type() == typeid(SDFShape)) {
            return std::any_cast<SDFShape>(candidate.value());
        }
        return std::nullopt;
    }

    void SDFSubtract::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data); 
    }

    Json SDFSubtract::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    void SDFSubtract::AbstractRenderProperties() {
        RenderAttributeProperty("Smooth");
        RenderAttributeProperty("Smoothness", {
            SliderRangeMetadata(0, 1)
        });
    }

    bool SDFSubtract::AbstractDetailsAvailable() {
        return false;
    }

    std::string SDFSubtract::AbstractHeader() {
        return "SDF Subtract";
    }

    std::string SDFSubtract::Icon() {
        return ICON_FA_SHAPES " " ICON_FA_MINUS;
    }

    std::optional<std::string> SDFSubtract::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::SDFSubtract>();
    }

    RASTER_DL_EXPORT void OnStartup() {
        Raster::NodeCategoryUtils::RegisterCategory(ICON_FA_SHAPES, "Shapes");

        Raster::Workspace::s_typeColors[ATTRIBUTE_TYPE(Raster::SDFShape)] = RASTER_COLOR32(52, 235, 222, 255);
        Raster::Workspace::s_typeNames[ATTRIBUTE_TYPE(Raster::SDFShape)] = "SDFShape";
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "SDF Subtract",
            .packageName = RASTER_PACKAGED "sdf_subtract",
            .category = Raster::NodeCategoryUtils::RegisterCategory(ICON_FA_SHAPES, "Shapes")
        };
    }
}