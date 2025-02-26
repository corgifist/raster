#include "audio/time_stretcher.h"
#include "common/audio_info.h"
#include "common/audio_samples.h"
#include "common/randomizer.h"
#include "raster.h"
#include <memory>
#include <rubberband/RubberBandStretcher.h>
#include <unordered_map>
#include "common/randomizer.h"
#include "stretchers.h"

namespace Raster {
    TimeStretcher::TimeStretcher() {
        this->m_handle = 0;
        this->m_channels = 0;
        this->m_sampleRate = 0;
        this->m_highQuality = false;
        this->m_deinterleavedSamples = nullptr;
        this->m_currentHighQuality = false;
    }

    TimeStretcher::TimeStretcher(int t_sampleRate, int t_channels, bool t_highQuality) {
        // RASTER_LOG(FormatString("INSTANTIATING WITH %i %i %i", t_sampleRate, t_channels, (int) t_highQuality));
        this->m_handle = Stretchers::CreateStretcher(t_sampleRate, t_channels, t_highQuality);
        this->m_channels = t_channels;
        this->m_sampleRate = t_sampleRate;
        this->m_highQuality = t_highQuality;
        this->m_currentHighQuality = m_highQuality;
        this->m_deinterleavedSamples = MakeDeinterleavedAudioSamples(AudioInfo::s_periodSize, AudioInfo::s_channels);
    }

    TimeStretcher::~TimeStretcher() {
        Stretchers::DestroyStretcher(m_handle);
    }

    void TimeStretcher::Validate() {
        auto& stretcher = Stretchers::GetStretcher(m_handle);
        if (!m_handle || m_channels != AudioInfo::s_channels || m_sampleRate != AudioInfo::s_sampleRate || m_currentHighQuality != m_highQuality) {
            this->m_handle = Stretchers::CreateStretcher(AudioInfo::s_sampleRate, AudioInfo::s_channels, m_highQuality);
            this->m_channels = AudioInfo::s_channels;
            this->m_sampleRate = AudioInfo::s_sampleRate;
            this->m_currentHighQuality = m_highQuality;
        }
    }

    void TimeStretcher::Reset() {
        Stretchers::GetStretcher(m_handle).reset();
    }

    int TimeStretcher::AvailableSamples() {
        return Stretchers::GetStretcher(m_handle).available();
    }

    void TimeStretcher::Push(SharedRawInterleavedAudioSamples t_samples) {
        auto& stretcher = Stretchers::GetStretcher(m_handle);
        ValidateDeinterleavedAudioSamples(m_deinterleavedSamples, AudioInfo::s_periodSize, AudioInfo::s_channels);
        DeinterleaveAudioSamples(t_samples, m_deinterleavedSamples, AudioInfo::s_periodSize, AudioInfo::s_channels);

        std::vector<float*> planarBuffersData;
        for (int i = 0; i < m_deinterleavedSamples->size(); i++) {
            planarBuffersData.push_back(m_deinterleavedSamples->at(i).data());
        }

        stretcher.process(planarBuffersData.data(), AudioInfo::s_periodSize, false);
    }

    SharedRawInterleavedAudioSamples TimeStretcher::Pop() {
        auto& stretcher = Stretchers::GetStretcher(m_handle);
        ValidateDeinterleavedAudioSamples(m_deinterleavedSamples, AudioInfo::s_periodSize, AudioInfo::s_channels);

        std::vector<float*> planarBuffersData;
        for (int i = 0; i < m_deinterleavedSamples->size(); i++) {
            planarBuffersData.push_back(m_deinterleavedSamples->at(i).data());
        }
        if (stretcher.available() >= AudioInfo::s_periodSize) {
            stretcher.retrieve(planarBuffersData.data(), AudioInfo::s_periodSize);

            SharedRawInterleavedAudioSamples resultInterleavedBuffer = MakeInterleavedAudioSamples(AudioInfo::s_periodSize, AudioInfo::s_channels);
            InterleaveAudioSamples(m_deinterleavedSamples, resultInterleavedBuffer, AudioInfo::s_periodSize, AudioInfo::s_channels);
            return resultInterleavedBuffer;
        }
        return nullptr;
    }

    void TimeStretcher::UseHighQualityEngine(bool t_useHighQualityEngine) {
        m_highQuality = t_useHighQualityEngine;
    }

    bool TimeStretcher::IsUsingHighQualityEngine() {
        return m_currentHighQuality;
    }

    void TimeStretcher::SetPitchRatio(float t_ratio) {
        Stretchers::GetStretcher(m_handle).setPitchScale(t_ratio);
    }

    float TimeStretcher::GetPitchRatio() {
        return Stretchers::GetStretcher(m_handle).getPitchScale();
    }

    void TimeStretcher::SetTimeRatio(float t_ratio) {
        Stretchers::GetStretcher(m_handle).setTimeRatio(t_ratio);
    }

    float TimeStretcher::GetTimeRatio() {
        return Stretchers::GetStretcher(m_handle).getTimeRatio();
    }
}