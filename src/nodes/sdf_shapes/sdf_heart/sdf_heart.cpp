#include "sdf_heart.h"
#include "node_category/node_category.h"
#include "common/workspace.h"

namespace Raster {

    SDFHeart::SDFHeart() {
        NodeBase::Initialize();

        AddOutputPin("Shape");

        SetupAttribute("Size", 0.5f);
    }

    AbstractPinMap SDFHeart::AbstractExecute(AbstractPinMap t_accumulator) {
        AbstractPinMap result = {};

        auto sizeCandidate = GetAttribute<float>("Size");
        if (sizeCandidate.has_value()) {
            auto& size = sizeCandidate.value();
            static SDFShape s_heartShape;
            if (s_heartShape.uniforms.empty()) {
                s_heartShape.distanceFunctionName = "fSDFHeart";
                s_heartShape.distanceFunctionCode = ReadFile(GPU::GetShadersPath() + "sdf_shapes/sdf_heart.frag");
            }
            s_heartShape.uniforms = {
                {"float", "uSDFHeartSize", size}
            };
            TryAppendAbstractPinMap(result, "Shape", s_heartShape);
        }

        return result;
    }

    void SDFHeart::AbstractRenderProperties() {
        RenderAttributeProperty("Size");
    }

    void SDFHeart::AbstractLoadSerialized(Json t_data) {
        SetAttributeValue("Size", t_data["Size"].get<float>());   
    }

    Json SDFHeart::AbstractSerialize() {
        return {
            {"Size", RASTER_ATTRIBUTE_CAST(float, "Size")}
        };
    }

    bool SDFHeart::AbstractDetailsAvailable() {
        return false;
    }

    std::string SDFHeart::AbstractHeader() {
        return "SDF Heart";
    }

    std::string SDFHeart::Icon() {
        return ICON_FA_HEART;
    }

    std::optional<std::string> SDFHeart::Footer() {
        return std::nullopt;
    }
}

extern "C" {
    Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::SDFHeart>();
    }

    void OnStartup() {
        Raster::NodeCategoryUtils::RegisterCategory(ICON_FA_SHAPES, "Shapes");

        Raster::Workspace::s_typeColors[ATTRIBUTE_TYPE(Raster::SDFShape)] = RASTER_COLOR32(52, 235, 222, 255);
        Raster::Workspace::s_typeNames[ATTRIBUTE_TYPE(Raster::SDFShape)] = "SDFShape";
    }

    Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "SDF Heart",
            .packageName = RASTER_PACKAGED "sdf_heart",
            .category = Raster::NodeCategoryUtils::RegisterCategory(ICON_FA_SHAPES, "Shapes")
        };
    }
}