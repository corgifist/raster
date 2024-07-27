#include "sampler_constants_base/sampler_constants_base.h"
#include "gpu/gpu.h"

extern "C" {
    Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::SamplerConstantsBase>(Raster::SamplerConstantsBase(Raster::TextureFilteringMode::Nearest));
    }

    Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Nearest Sampler Filtering",
            .packageName = RASTER_PACKAGED "nearest_sampler_filtering_constant",
            .category = Raster::NodeCategory::SamplerConstants
        };
    }
}