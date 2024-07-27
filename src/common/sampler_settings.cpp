#include "common/sampler_settings.h"

namespace Raster {
    SamplerSettings::SamplerSettings() {
        this->filteringMode = TextureFilteringMode::Linear;
        this->wrappingMode = TextureWrappingMode::Repeat;
    }

    SamplerSettings::SamplerSettings(Json t_data) {
        this->filteringMode = static_cast<TextureFilteringMode>(t_data["FilteringMode"].get<int>());
        this->wrappingMode = static_cast<TextureWrappingMode>(t_data["WrappingMode"].get<int>());
    }

    Json SamplerSettings::Serialize() {
        return {
            {"FilteringMode", static_cast<int>(filteringMode)},
            {"WrappingMode", static_cast<int>(wrappingMode)}
        };
    }
};