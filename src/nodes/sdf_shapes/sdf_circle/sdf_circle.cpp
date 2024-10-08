#include "sdf_circle.h"
#include "node_category/node_category.h"
#include "common/workspace.h"

namespace Raster {

    SDFCircle::SDFCircle() {
        NodeBase::Initialize();

        SetupAttribute("Radius", 0.5f);

        AddOutputPin("Shape");
    }

    AbstractPinMap SDFCircle::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};

        auto radiusCandidate = GetAttribute<float>("Radius");
        if (radiusCandidate.has_value()) {
            auto& radius = radiusCandidate.value();
            static SDFShape s_circleShape;
            if (s_circleShape.uniforms.empty()) {
                s_circleShape.distanceFunctionName = "fSDFCircle";
                s_circleShape.distanceFunctionCode = ReadFile(GPU::GetShadersPath() + "sdf_shapes/sdf_circle.frag");
            }
            s_circleShape.uniforms = {
                SDFShapeUniform("float", "uSDFCircleRadius", radius)
            };
            TryAppendAbstractPinMap(result, "Shape", s_circleShape);
        }

        return result;
    }

    void SDFCircle::AbstractRenderProperties() {
        RenderAttributeProperty("Radius", {
            SliderStepMetadata(0.01f)
        });
    }

    void SDFCircle::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json SDFCircle::AbstractSerialize() {
        return SerializeAllAttributes();
    }


    bool SDFCircle::AbstractDetailsAvailable() {
        return false;
    }

    std::string SDFCircle::AbstractHeader() {
        return "SDF Circle";
    }

    std::string SDFCircle::Icon() {
        return ICON_FA_CIRCLE;
    }

    std::optional<std::string> SDFCircle::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::SDFCircle>();
    }

    RASTER_DL_EXPORT void OnStartup() {
        Raster::NodeCategoryUtils::RegisterCategory(ICON_FA_SHAPES, "Shapes");

        Raster::Workspace::s_typeColors[ATTRIBUTE_TYPE(Raster::SDFShape)] = RASTER_COLOR32(52, 235, 222, 255);
        Raster::Workspace::s_typeNames[ATTRIBUTE_TYPE(Raster::SDFShape)] = "SDFShape";
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "SDF Circle",
            .packageName = RASTER_PACKAGED "sdf_circle",
            .category = Raster::NodeCategoryUtils::RegisterCategory(ICON_FA_SHAPES, "Shapes")
        };
    }
}