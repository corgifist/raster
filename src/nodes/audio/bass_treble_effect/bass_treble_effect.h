#pragma once

#include "raster.h"
#include "common/common.h"

#include "common/audio_samples.h"
#include "audio/audio.h"

#include "common/audio_cache.h"
#include "common/shared_mutex.h"

namespace Raster {

    struct BassTrebleCachedData
    {
        float samplerate;
        double treble;
        double bass;
        double gain;
        double slope, hzBass, hzTreble;
        double a0Bass, a1Bass, a2Bass, b0Bass, b1Bass, b2Bass;
        double a0Treble, a1Treble, a2Treble, b0Treble, b1Treble, b2Treble;
        double xn1Bass, xn2Bass, yn1Bass, yn2Bass;
        double xn1Treble, xn2Treble, yn1Treble, yn2Treble;

        BassTrebleCachedData();
    };

    struct BassTrebleEffect : public NodeBase {
        BassTrebleEffect();
        
        AbstractPinMap AbstractExecute(AbstractPinMap t_accumulator = {});
        void AbstractRenderProperties();
        bool AbstractDetailsAvailable();

        void AbstractLoadSerialized(Json t_data);
        Json AbstractSerialize();

        std::string AbstractHeader();
        std::string Icon();
        std::optional<std::string> Footer();

    private:
        void BassCoefficients(double hz, double slope, double gain, double samplerate, 
                                   double& a0, double& a1, double& a2,
                                   double& b0, double& b1, double& b2);

        void TrebleCoefficients(double hz, double slope, double gain, double samplerate, 
                                   double& a0, double& a1, double& a2,
                                   double& b0, double& b1, double& b2);

        BassTrebleCachedData m_data;
        AudioCache m_cache;
        SharedMutex m_mutex;
    };
};