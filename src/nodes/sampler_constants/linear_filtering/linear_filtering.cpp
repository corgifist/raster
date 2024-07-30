#include "sampler_constants_base/sampler_constants_base.h"
#include "gpu/gpu.h"

extern "C" {
    Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::SamplerConstantsBase>(Raster::SamplerConstantsBase(Raster::TextureFilteringMode::Linear));
    }

    Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Linear Sampler Filtering",
            .packageName = RASTER_PACKAGED "linear_sampler_filtering_constant",
            .category = Raster::NodeCategory::SamplerConstants
        };
    }
}