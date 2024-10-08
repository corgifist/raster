#pragma once

#include "raster.h"
#include "typedefs.h"
#include "audio_samples.h"

namespace Raster {
    struct AudioCache {
    public:
        AudioCache();
        void SetCachedSamples(AudioSamples t_samples);
        std::optional<AudioSamples> GetCachedSamples();

    private:
        AudioSamples m_cachedSamples;
    };
};