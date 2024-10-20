#include "sampler_constants_base/sampler_constants_base.h"
#include "gpu/gpu.h"

extern "C" {
    RASTER_DL_EXPORT Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::SamplerConstantsBase>(Raster::SamplerConstantsBase(Raster::TextureWrappingMode::ClampToEdge));
    }

    RASTER_DL_EXPORT void OnStartup() {
        Raster::NodeCategoryUtils::RegisterCategory(ICON_FA_IMAGE, Raster::Localization::GetString("SAMPLER_CONSTANTS"));
    }

    RASTER_DL_EXPORT Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Clamp To Edge Wrapping",
            .packageName = RASTER_PACKAGED "clamp_to_edge_sampler_wrapping_constant",
            .category = Raster::NodeCategoryUtils::RegisterCategory(ICON_FA_IMAGE, Raster::Localization::GetString("SAMPLER_CONSTANTS"))
        };
    }
}