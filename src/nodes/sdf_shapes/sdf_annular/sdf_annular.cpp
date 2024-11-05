#include "sdf_annular.h"
#include "common/node_category.h"
#include "common/workspace.h"

namespace Raster {

    SDFAnnular::SDFAnnular() {
        NodeBase::Initialize();

        this->m_mixedShape = SDFShape();
        this->m_firstShapeID = -1;

        SetupAttribute("A", SDFShape());
        SetupAttribute("Intensity", 0.5f);

        AddInputPin("A");

        AddOutputPin("Shape");
    }

    AbstractPinMap SDFAnnular::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};

        auto aCandidate = GetShape("A", t_contextData);
        auto intensityCandidate = GetAttribute<float>("Intensity", t_contextData);
        if (aCandidate.has_value() && intensityCandidate.has_value()) {
            auto a = aCandidate.value();
            auto intensity = intensityCandidate.value();

            TransformShapeUniforms(a, "Annular");
            if (m_firstShapeID != a.id) {
                static std::optional<std::string> s_mixBase;
                if (!s_mixBase.has_value()) {
                    s_mixBase = ReadFile(GPU::GetShadersPath() + "sdf_shapes/sdf_annular.frag");
                }
                std::string mixBase = s_mixBase.value_or("");

                m_mixedShape.uniforms.clear();
                m_mixedShape.id = Randomizer::GetRandomInteger();

                m_mixedShape.distanceFunctionName = "fSDFAnnular";
                m_mixedShape.distanceFunctionCode = "";
                TransformShape(a, "Annular");
                m_mixedShape.distanceFunctionCode += a.distanceFunctionCode + "\n\n";

                m_mixedShape.distanceFunctionCode += mixBase + "\n\n";
                m_mixedShape.distanceFunctionCode = ReplaceString(m_mixedShape.distanceFunctionCode, "SDF_ANNULAR_FUNCTION_PLACEHOLDER", a.distanceFunctionName);

                m_firstShapeID = a.id;
            }

            for (auto& uniform : a.uniforms) {
                m_mixedShape.uniforms.push_back(uniform);
            }

            m_mixedShape.uniforms.push_back({
                "float", "uSDFAnnularIntensity", intensity * 0.04f
            });

            TryAppendAbstractPinMap(result, "Shape", m_mixedShape);

            m_mixedShape.uniforms.clear();
        }

        return result;
    }

    void SDFAnnular::TransformShape(SDFShape& t_shape, std::string t_uniqueID) {
        t_shape.distanceFunctionCode = ReplaceString(t_shape.distanceFunctionCode, "\\b" + t_shape.distanceFunctionName + "\\b", t_shape.distanceFunctionName + t_uniqueID);
        t_shape.distanceFunctionName += t_uniqueID;
    }

    void SDFAnnular::TransformShapeUniforms(SDFShape& t_shape, std::string t_uniqueID) {
        for (auto& uniform : t_shape.uniforms) {
            t_shape.distanceFunctionCode = ReplaceString(t_shape.distanceFunctionCode, "\\b" + uniform.name + "\\b", uniform.name + t_uniqueID);
        }
        for (auto& uniform : t_shape.uniforms) {
            uniform.name += t_uniqueID;
        }
    }

    std::optional<SDFShape> SDFAnnular::GetShape(std::string t_attribute, ContextData& t_contextData) {
        auto candidate = GetDynamicAttribute(t_attribute, t_contextData);
        if (candidate.has_value() && candidate.value().type() == typeid(SDFShape)) {
            return std::any_cast<SDFShape>(candidate.value());
        }
        return std::nullopt;
    }

    void SDFAnnular::AbstractRenderProperties() {
        RenderAttributeProperty("Intensity", {
            SliderStepMetadata(0.01f)
        });
    }

    void SDFAnnular::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json SDFAnnular::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool SDFAnnular::AbstractDetailsAvailable() {
        return false;
    }

    std::string SDFAnnular::AbstractHeader() {
        return "SDF Annular";
    }

    std::string SDFAnnular::Icon() {
        return ICON_FA_BORDER_NONE;
    }

    std::optional<std::string> SDFAnnular::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::SDFAnnular>();
    }

    RASTER_DL_EXPORT void OnStartup() {
        Raster::NodeCategoryUtils::RegisterCategory(ICON_FA_SHAPES, "Shapes");

        Raster::Workspace::s_typeColors[ATTRIBUTE_TYPE(Raster::SDFShape)] = RASTER_COLOR32(52, 235, 222, 255);
        Raster::Workspace::s_typeNames[ATTRIBUTE_TYPE(Raster::SDFShape)] = "SDFShape";
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "SDF Annular",
            .packageName = RASTER_PACKAGED "sdf_annular",
            .category = Raster::NodeCategoryUtils::RegisterCategory(ICON_FA_SHAPES, "Shapes")
        };
    }
}