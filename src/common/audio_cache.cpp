#include "common/audio_cache.h"

namespace Raster {
    AudioCache::AudioCache() {
    }

    void AudioCache::SetCachedSamples(AudioSamples t_samples) {
        this->m_cachedSamples = t_samples;
    }

    std::optional<AudioSamples> AudioCache::GetCachedSamples() {
        if (m_cachedSamples.samples) return m_cachedSamples;
        return std::nullopt;
    }
};