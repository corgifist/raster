#include "sampler_constants_base/sampler_constants_base.h"
#include "gpu/gpu.h"

extern "C" {
    Raster::AbstractNode SpawnNode() {
        return (Raster::AbstractNode) std::make_shared<Raster::SamplerConstantsBase>(Raster::SamplerConstantsBase(Raster::TextureWrappingMode::Repeat));
    }

    Raster::NodeDescription GetDescription() {
        return Raster::NodeDescription{
            .prettyName = "Repeat Sampler Wrapping",
            .packageName = RASTER_PACKAGED "repeat_sampler_wrapping_constant",
            .category = Raster::NodeCategory::SamplerConstants
        };
    }
}