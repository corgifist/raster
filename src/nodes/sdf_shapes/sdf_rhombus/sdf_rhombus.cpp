#include "sdf_rhombus.h"
#include "node_category/node_category.h"
#include "common/workspace.h"

namespace Raster {

    SDFRhombus::SDFRhombus() {
        NodeBase::Initialize();

        SetupAttribute("Size", glm::vec2(0.5f, 0.5f));

        AddOutputPin("Shape");
    }

    AbstractPinMap SDFRhombus::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};

        auto sizeCandidate = GetAttribute<glm::vec2>("Size");
        if (sizeCandidate.has_value()) {
            auto& size = sizeCandidate.value();
            static SDFShape s_rhombus;
            if (s_rhombus.uniforms.empty()) {
                s_rhombus.distanceFunctionName = "fSDFRhombus";
                s_rhombus.distanceFunctionCode = ReadFile(GPU::GetShadersPath() + "sdf_shapes/sdf_rhombus.frag");
            }
            s_rhombus.uniforms = {
                SDFShapeUniform("vec2", "uSDFRhombusSize", size)
            };
            TryAppendAbstractPinMap(result, "Shape", s_rhombus);
        }

        return result;
    }

    void SDFRhombus::AbstractRenderProperties() {
        RenderAttributeProperty("Size");
    }

    void SDFRhombus::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);  
    }

    Json SDFRhombus::AbstractSerialize() {
        return SerializeAllAttributes();
    }

    bool SDFRhombus::AbstractDetailsAvailable() {
        return false;
    }

    std::string SDFRhombus::AbstractHeader() {
        return "SDF Rhombus";
    }

    std::string SDFRhombus::Icon() {
        return ICON_FA_SHAPES;
    }

    std::optional<std::string> SDFRhombus::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::SDFRhombus>();
    }

    RASTER_DL_EXPORT void OnStartup() {
        Raster::NodeCategoryUtils::RegisterCategory(ICON_FA_SHAPES, "Shapes");

        Raster::Workspace::s_typeColors[ATTRIBUTE_TYPE(Raster::SDFShape)] = RASTER_COLOR32(52, 235, 222, 255);
        Raster::Workspace::s_typeNames[ATTRIBUTE_TYPE(Raster::SDFShape)] = "SDFShape";
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "SDF Rhombus",
            .packageName = RASTER_PACKAGED "sdf_rhombus",
            .category = Raster::NodeCategoryUtils::RegisterCategory(ICON_FA_SHAPES, "Shapes")
        };
    }
}