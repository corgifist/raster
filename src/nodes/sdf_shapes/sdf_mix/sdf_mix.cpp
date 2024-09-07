#include "sdf_mix.h"
#include "node_category/node_category.h"
#include "common/workspace.h"

namespace Raster {

    SDFMix::SDFMix() {
        NodeBase::Initialize();

        this->m_mixedShape = SDFShape();
        this->m_firstShapeID = -1;
        this->m_secondShapeID = -1;

        SetupAttribute("A", SDFShape());
        SetupAttribute("B", SDFShape());
        SetupAttribute("Phase", 0.5f);

        AddInputPin("A");
        AddInputPin("B");

        AddOutputPin("Shape");
    }

    AbstractPinMap SDFMix::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};

        auto aCandidate = GetShape("A");
        auto bCandidate = GetShape("B");
        auto phaseCandidate = GetAttribute<float>("Phase");
        if (aCandidate.has_value() && bCandidate.has_value() && phaseCandidate.has_value()) {
            auto a = aCandidate.value();
            auto b = bCandidate.value();
            auto phase = phaseCandidate.value();

            TransformShapeUniforms(a, "Mix1");
            TransformShapeUniforms(b, "Mix2");
            if (m_firstShapeID != a.id || m_secondShapeID != b.id) {
                static std::optional<std::string> s_mixBase;
                if (!s_mixBase.has_value()) {
                    s_mixBase = ReadFile(GPU::GetShadersPath() + "sdf_shapes/sdf_mix.frag");
                }
                std::string mixBase = s_mixBase.value_or("");

                m_mixedShape.uniforms.clear();
                m_mixedShape.id = Randomizer::GetRandomInteger();

                m_mixedShape.distanceFunctionName = "fSDFMix";
                m_mixedShape.distanceFunctionCode = "";
                TransformShape(a, "Mix1");
                TransformShape(b, "Mix2");
                m_mixedShape.distanceFunctionCode += a.distanceFunctionCode + "\n\n";
                m_mixedShape.distanceFunctionCode += b.distanceFunctionCode + "\n\n";

                m_mixedShape.distanceFunctionCode += mixBase + "\n\n";
                m_mixedShape.distanceFunctionCode = ReplaceString(m_mixedShape.distanceFunctionCode, "SDF_MIX_FIRST_FUNCTION_PLACEHOLDER", a.distanceFunctionName);
                m_mixedShape.distanceFunctionCode = ReplaceString(m_mixedShape.distanceFunctionCode, "SDF_MIX_SECOND_FUNCTION_PLACEHOLDER", b.distanceFunctionName);

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
                "float", "uSDFMixPhase", phase
            });

            TryAppendAbstractPinMap(result, "Shape", m_mixedShape);

            m_mixedShape.uniforms.clear();
        }

        return result;
    }

    void SDFMix::TransformShape(SDFShape& t_shape, std::string t_uniqueID) {
        t_shape.distanceFunctionCode = ReplaceString(t_shape.distanceFunctionCode, "\\b" + t_shape.distanceFunctionName + "\\b", t_shape.distanceFunctionName + t_uniqueID);
        t_shape.distanceFunctionName += t_uniqueID;
    }

    void SDFMix::TransformShapeUniforms(SDFShape& t_shape, std::string t_uniqueID) {
        for (auto& uniform : t_shape.uniforms) {
            t_shape.distanceFunctionCode = ReplaceString(t_shape.distanceFunctionCode, "\\b" + uniform.name + "\\b", uniform.name + t_uniqueID);
        }
        for (auto& uniform : t_shape.uniforms) {
            uniform.name += t_uniqueID;
        }
    }

    std::optional<SDFShape> SDFMix::GetShape(std::string t_attribute) {
        auto candidate = GetDynamicAttribute(t_attribute);
        if (candidate.has_value() && candidate.value().type() == typeid(SDFShape)) {
            return std::any_cast<SDFShape>(candidate.value());
        }
        return std::nullopt;
    }

    void SDFMix::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data); 
    }

    Json SDFMix::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    void SDFMix::AbstractRenderProperties() {
        RenderAttributeProperty("Phase");
    }

    bool SDFMix::AbstractDetailsAvailable() {
        return false;
    }

    std::string SDFMix::AbstractHeader() {
        return "SDF Mix";
    }

    std::string SDFMix::Icon() {
        return ICON_FA_DROPLET;
    }

    std::optional<std::string> SDFMix::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::SDFMix>();
    }

    RASTER_DL_EXPORT void OnStartup() {
        Raster::NodeCategoryUtils::RegisterCategory(ICON_FA_SHAPES, "Shapes");

        Raster::Workspace::s_typeColors[ATTRIBUTE_TYPE(Raster::SDFShape)] = RASTER_COLOR32(52, 235, 222, 255);
        Raster::Workspace::s_typeNames[ATTRIBUTE_TYPE(Raster::SDFShape)] = "SDFShape";
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "SDF Mix",
            .packageName = RASTER_PACKAGED "sdf_mix",
            .category = Raster::NodeCategoryUtils::RegisterCategory(ICON_FA_SHAPES, "Shapes")
        };
    }
}