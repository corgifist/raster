#pragma once

#include "common/audio_samples.h"
#include "raster.h"
#include "common/audio_info.h"

namespace Raster {
    struct TimeStretcher {
        TimeStretcher();
        TimeStretcher(int t_sampleRate, int t_channels, bool t_highQuality = false);
        ~TimeStretcher();

        TimeStretcher(TimeStretcher const&) = delete;
        TimeStretcher& operator=(TimeStretcher const&) = delete;

        void Validate();
        void Reset();

        int AvailableSamples();

        void Push(SharedRawInterleavedAudioSamples t_samples);
        SharedRawInterleavedAudioSamples Pop();

        void SetTimeRatio(float t_ratio);
        float GetTimeRatio();

        void SetPitchRatio(float t_ratio);
        float GetPitchRatio();

        void UseHighQualityEngine(bool t_useHighQualityEngine);
        bool IsUsingHighQualityEngine();

    private:
        int m_channels;
        int m_sampleRate;
        bool m_highQuality;
        bool m_currentHighQuality;
        int m_handle;

        SharedRawDeinterleavedAudioSamples m_deinterleavedSamples;
    };
};