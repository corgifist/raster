#include "sdf_heart.h"
#include "common/node_category.h"
#include "common/workspace.h"

namespace Raster {

    SDFHeart::SDFHeart() {
        NodeBase::Initialize();

        AddOutputPin("Shape");

        SetupAttribute("Size", 0.5f);
    }

    AbstractPinMap SDFHeart::AbstractExecute(ContextData& t_contextData) {
        AbstractPinMap result = {};

        auto sizeCandidate = GetAttribute<float>("Size", t_contextData);
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
        RenderAttributeProperty("Size", {
            SliderStepMetadata(0.01f)
        });
    }

    void SDFHeart::AbstractLoadSerialized(Json t_data) {
        DeserializeAllAttributes(t_data);
    }

    Json SDFHeart::AbstractSerialize() {
        return SerializeAllAttributes();
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
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::SDFHeart>();
    }

    RASTER_DL_EXPORT void OnStartup() {
        Raster::NodeCategoryUtils::RegisterCategory(ICON_FA_SHAPES, "Shapes");

        Raster::Workspace::s_typeColors[ATTRIBUTE_TYPE(Raster::SDFShape)] = RASTER_COLOR32(52, 235, 222, 255);
        Raster::Workspace::s_typeNames[ATTRIBUTE_TYPE(Raster::SDFShape)] = "SDFShape";
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "SDF Heart",
            .packageName = RASTER_PACKAGED "sdf_heart",
            .category = Raster::NodeCategoryUtils::RegisterCategory(ICON_FA_SHAPES, "Shapes")
        };
    }
}