#pragma once

#include "raster.h"
#include "common/randomizer.h"
#include <rubberband/RubberBandStretcher.h>

#define FAST_ENGINE_FLAGS RubberBand::RubberBandStretcher::OptionEngineFaster  \
                    | RubberBand::RubberBandStretcher::OptionProcessRealTime  \
                    | RubberBand::RubberBandStretcher::OptionPitchHighSpeed \
                    | RubberBand::RubberBandStretcher::OptionThreadingAuto \
                    | RubberBand::RubberBandStretcher::OptionWindowShort

#define QUALITY_ENGINE_FLAGS RubberBand::RubberBandStretcher::OptionProcessRealTime  \
                        | RubberBand::RubberBandStretcher::OptionPitchHighConsistency \
                        | RubberBand::RubberBandStretcher::OptionChannelsTogether \
                        | RubberBand::RubberBandStretcher::OptionFormantPreserved \
                        | RubberBand::RubberBandStretcher::OptionWindowLong \
                        | RubberBand::RubberBandStretcher::OptionThreadingAuto 

namespace Raster {
    struct Stretchers {
        static std::unordered_map<int, std::shared_ptr<RubberBand::RubberBandStretcher>> s_stretchers;

        static RubberBand::RubberBandStretcher& GetStretcher(int m_handle);

        static int CreateStretcher(int t_sampleRate, int t_channels, bool t_highQuality);

        static void DestroyStretcher(int t_handle);
    };
}