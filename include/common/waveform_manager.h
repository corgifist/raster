#pragma once

#include "common/audio_samples.h"
#include "common/synchronized_value.h"
#include "raster.h"
#include "common/audio_info.h"

namespace Raster {

    struct WaveformRecord {
        std::vector<float> data;
        int precision;
    };

    // this class is responsible for calculating / storing waveform data
    // waveform calculation is separated from main thread
    struct WaveformManager {
        static void Initialize();

        static void PushWaveformSamples(SharedRawInterleavedAudioSamples t_samples);
        static void RequestWaveformRefresh(int t_compositionID);

        static SynchronizedValue<std::unordered_map<int, WaveformRecord>>& GetRecords();
        static void EraseRecord(int t_compositionID);

        static void Terminate();
    
    private:
        static void ManagerLoop();
    };
};