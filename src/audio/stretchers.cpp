#include "stretchers.h"

namespace Raster {
    std::unordered_map<int, std::shared_ptr<RubberBand::RubberBandStretcher>> Stretchers::s_stretchers;

    RubberBand::RubberBandStretcher& Stretchers::GetStretcher(int m_handle) {
        return *s_stretchers[m_handle];
    }

    int Stretchers::CreateStretcher(int t_sampleRate, int t_channels, bool t_highQuality) {
        int id = Randomizer::GetRandomInteger();
        s_stretchers[id] = std::make_shared<RubberBand::RubberBandStretcher>(t_sampleRate, t_channels, t_highQuality ? QUALITY_ENGINE_FLAGS : FAST_ENGINE_FLAGS);
        return id;
    }

    void Stretchers::DestroyStretcher(int t_handle) {
        if (s_stretchers.find(t_handle) != s_stretchers.end()) {
            s_stretchers.erase(t_handle);
        }
    }
};