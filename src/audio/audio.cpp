#include "audio/audio.h"
#include "common/audio_discretization_options.h"
#include "common/audio_samples.h"

#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "common/workspace.h"
#include "common/audio_info.h"
#include "common/threads.h"
#include <rubberband/RubberBandStretcher.h>

namespace Raster {

    AudioBackendInfo Audio::s_backendInfo;
    AudioDiscretizationOptions Audio::s_currentOptions;

    static int s_channelCount, s_sampleRate;
    static ma_device s_device;
    static bool s_audioActive;

    static std::optional<std::future<int>> s_slowedDownAudioPass;

    static std::optional<AudioDiscretizationOptions> s_internalAudioOptions;

    static int PerformAudioPass() {
        auto& project = Workspace::GetProject();
        auto& buses = project.audioBuses;
        int mainBusID = -1;

        auto firstTime = std::chrono::high_resolution_clock::now();

        // restoring the main audio bus to the default value
        project.audioBusesMutex->lock();
        for (auto& bus : buses) {
            if (bus.samples.size() != AudioInfo::s_periodSize * AudioInfo::s_channels) {
                bus.samples.resize(AudioInfo::s_periodSize * AudioInfo::s_channels);
            }
            if (bus.main) mainBusID = bus.id;
            for (int i = 0; i < AudioInfo::s_periodSize * AudioInfo::s_channels; i++) {
                bus.samples[i] = 0.0f;
            }
        }
        project.audioBusesMutex->unlock();

        project.Traverse({
            {"AUDIO_PASS", true},
            {"AUDIO_PASS_ID", AudioInfo::s_audioPassID},
            {"INCREMENT_EPF", false},
            {"RESET_WORKSPACE_STATE", false},
            {"ALLOW_MEDIA_DECODING", true},
            {"ONLY_AUDIO_NODES", true}
        });


        project.audioBusesMutex->lock();

        // redirecting audio buses
        for (auto& bus : buses) {
            if (!bus.main && bus.redirectID >= 0) {
                auto redirectBusCandidate = Workspace::GetAudioBusByID(bus.redirectID);
                if (redirectBusCandidate.has_value()) {
                    auto& redirectBus = redirectBusCandidate.value();
                    if (redirectBus->samples.size() > 0 && bus.samples.size() > 0) {
                        for (int i = 0; i < AudioInfo::s_periodSize * AudioInfo::s_channels; i++) {
                            redirectBus->samples[i] += bus.samples[i];
                        }
                    }
                }
            }
        }
        project.audioBusesMutex->unlock();
        AudioInfo::s_audioPassID++;
        return (float) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - firstTime).count();
    }

    static void CopyFromMainBus(void* t_output) {
        auto& buses = Workspace::GetProject().audioBuses;
        for (auto& bus : buses) {
            if (bus.main) {
                memcpy((float*) t_output, bus.samples.data(), AudioInfo::s_periodSize * AudioInfo::s_channels * sizeof(float));
                break;
            }
        }
    }

    static RubberBand::RubberBandStretcher s_stretcher(AudioInfo::s_sampleRate, AudioInfo::s_channels, RubberBand::RubberBandStretcher::OptionEngineFaster 
                                                                                                    | RubberBand::RubberBandStretcher::OptionProcessRealTime 
                                                                                                    | RubberBand::RubberBandStretcher::OptionPitchHighSpeed
                                                                                                    | RubberBand::RubberBandStretcher::OptionThreadingNever);

    static void PushToStretcher(float* t_interleavedSamples) {
        static SharedRawDeinterleavedAudioSamples s_planarBuffers = MakeDeinterleavedAudioSamples(AudioInfo::s_periodSize, AudioInfo::s_channels);
        ValidateDeinterleavedAudioSamples(s_planarBuffers, AudioInfo::s_periodSize, AudioInfo::s_channels);

        std::vector<float*> rawPlanarBuffers;
        for (auto& planarBuffer : *s_planarBuffers) {
            rawPlanarBuffers.push_back(planarBuffer.data());
        }

        ma_deinterleave_pcm_frames(ma_format_f32, AudioInfo::s_channels, AudioInfo::s_periodSize, t_interleavedSamples, (void**) rawPlanarBuffers.data());
        s_stretcher.process(rawPlanarBuffers.data(), AudioInfo::s_periodSize, false);
    }

    static void raster_data_callback(ma_device* t_device, void* t_output, const void* t_input, ma_uint32 t_frameCount) {
        Threads::s_audioThreadID = std::this_thread::get_id();
        float* fOutput = (float*) t_output;
        auto allowedMsPerCall = (1.0 / ((double) AudioInfo::s_sampleRate / (double) AudioInfo::s_periodSize)) * 1000 / 2;
        if (s_slowedDownAudioPass.has_value() && IsFutureReady(s_slowedDownAudioPass.value())) {
            auto& future = s_slowedDownAudioPass.value();
            int passMs = future.get();
            static SharedRawInterleavedAudioSamples s_samples = MakeInterleavedAudioSamples(AudioInfo::s_periodSize, AudioInfo::s_channels);
            CopyFromMainBus(s_samples->data());
            if (passMs > allowedMsPerCall) {
                s_stretcher.setTimeRatio(s_stretcher.getTimeRatio() * 1.8f);
                PushToStretcher(s_samples->data());
                s_slowedDownAudioPass = std::nullopt;
            } else {
                AudioInfo::s_audioPassID++;
                memcpy(t_output, s_samples->data(), AudioInfo::s_periodSize * AudioInfo::s_channels * sizeof(float));
                s_slowedDownAudioPass = std::nullopt;
                return;
            }
        }
        if (s_slowedDownAudioPass.has_value() && !IsFutureReady(s_slowedDownAudioPass.value())) {
            s_stretcher.setTimeRatio(s_stretcher.getTimeRatio() * 1.6f);
        }
        if (s_stretcher.available() >= AudioInfo::s_periodSize) {
            static SharedRawDeinterleavedAudioSamples s_channels = MakeDeinterleavedAudioSamples(AudioInfo::s_periodSize, AudioInfo::s_channels);
            ValidateDeinterleavedAudioSamples(s_channels, AudioInfo::s_periodSize, AudioInfo::s_channels);

            std::vector<float*> channelPointers;
            for (auto& channel : *s_channels) {
                channelPointers.push_back(channel.data());
            }

            s_stretcher.retrieve(channelPointers.data(), AudioInfo::s_periodSize);
            ma_interleave_pcm_frames(ma_format_f32, AudioInfo::s_channels, AudioInfo::s_periodSize, (const void**) channelPointers.data(), t_output);
            if (s_stretcher.available() <= AudioInfo::s_channels) {
                s_slowedDownAudioPass = std::async(std::launch::async, []() {
                    return PerformAudioPass();
                });
            }
            return;
        }


        if (Workspace::IsProjectLoaded() && Workspace::GetProject().playing) {
            auto timeDifference = PerformAudioPass();
            CopyFromMainBus(t_output);
            if (timeDifference > allowedMsPerCall) {
                s_stretcher.reset();
                s_stretcher.setTimeRatio(1 + (timeDifference * 1.5f / allowedMsPerCall));
                PushToStretcher((float*) t_output);
            }
        }
    }

    void Audio::Initialize() {
        s_audioActive = false;
        s_backendInfo.name = "miniaudio";
        s_backendInfo.version = MA_VERSION_STRING;
    }

    void Audio::CreateAudioInstance() {
        ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
        deviceConfig.playback.format = ma_format_f32;
        deviceConfig.playback.channels = Audio::s_currentOptions.desiredChannelsCount;
        deviceConfig.sampleRate = Audio::s_currentOptions.desiredSampleRate;
        deviceConfig.dataCallback = raster_data_callback;
        deviceConfig.pUserData = nullptr;
        deviceConfig.periodSizeInFrames = 4096;
        deviceConfig.performanceProfile = 
            Audio::s_currentOptions.performanceProfile == AudioPerformanceProfile::Conservative ? 
                    ma_performance_profile_conservative : ma_performance_profile_low_latency;

        AudioInfo::s_channels = Audio::s_currentOptions.desiredChannelsCount;
        AudioInfo::s_sampleRate = Audio::s_currentOptions.desiredSampleRate;
        AudioInfo::s_periodSize = deviceConfig.periodSizeInFrames;
    
        if (ma_device_init(nullptr, &deviceConfig, &s_device) != MA_SUCCESS) {
            RASTER_LOG("failed to create audio playback!");
        } else {
            if (ma_device_start(&s_device) != MA_SUCCESS) {
                RASTER_LOG("failed to start audio playback!");
                ma_device_uninit(&s_device);
            } else {
                s_audioActive = true;
            }
        }
        RASTER_LOG(FormatString("creating audio instance with options: %i|%i|%i", s_currentOptions.desiredSampleRate, 
                                                                                s_currentOptions.desiredChannelsCount,
                                                                                static_cast<int>(s_currentOptions.performanceProfile)));
        s_internalAudioOptions = Audio::s_currentOptions;
    }

    bool Audio::IsAudioInstanceActive() {
        return s_audioActive;
    }

    bool Audio::UpdateAudioInstance() {
        if (!s_internalAudioOptions.has_value()) {
            CreateAudioInstance();
            return true;
        }
        auto& audioOptions = s_internalAudioOptions.value();
        if (s_internalAudioOptions->desiredChannelsCount != Audio::s_currentOptions.desiredChannelsCount ||
            s_internalAudioOptions->desiredSampleRate != Audio::s_currentOptions.desiredSampleRate ||
            s_internalAudioOptions->performanceProfile != Audio::s_currentOptions.performanceProfile) {
                TerminateAudioInstance();
                CreateAudioInstance();
                return true;
            }
        return false;
    }

    void Audio::TerminateAudioInstance() {
        if (!IsAudioInstanceActive()) return;
        ma_device_uninit(&s_device);
        s_audioActive = false;
    }
};