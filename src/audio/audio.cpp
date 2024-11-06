#include "audio/audio.h"

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

    static int s_channelCount, s_sampleRate;
    static ma_device s_device;

    static void raster_data_callback(ma_device* t_device, void* t_output, const void* t_input, ma_uint32 t_frameCount) {
        Threads::s_audioThreadID = std::this_thread::get_id();
        float* fOutput = (float*) t_output;

        static RubberBand::RubberBandStretcher s_stretcher(AudioInfo::s_sampleRate, AudioInfo::s_channels, RubberBand::RubberBandStretcher::OptionEngineFaster 
                                                                                                        | RubberBand::RubberBandStretcher::OptionProcessRealTime 
                                                                                                        | RubberBand::RubberBandStretcher::OptionPitchHighSpeed
                                                                                                        | RubberBand::RubberBandStretcher::OptionThreadingNever);
        if (s_stretcher.available() >= AudioInfo::s_periodSize) {
            static std::vector<std::vector<float>> s_channels;
            if (s_channels.size() != AudioInfo::s_channels) {
                s_channels.resize(AudioInfo::s_channels);
                for (int i = 0; i < AudioInfo::s_channels; i++) {
                    auto& channelBuffer = s_channels[i];
                    channelBuffer.resize(AudioInfo::s_periodSize);
                }
            }

            std::vector<float*> channelPointers;
            for (auto& channel : s_channels) {
                channelPointers.push_back(channel.data());
            }

            s_stretcher.retrieve(channelPointers.data(), AudioInfo::s_periodSize);
            ma_interleave_pcm_frames(ma_format_f32, AudioInfo::s_channels, AudioInfo::s_periodSize, (const void**) channelPointers.data(), t_output);
            return;
        }

        // SharedLockGuard guard(AudioInfo::s_mutex);

        if (Workspace::IsProjectLoaded() && Workspace::GetProject().playing ) {
            auto& project = Workspace::GetProject();
            auto& buses = project.audioBuses;
            int mainBusID = -1;

            auto firstTime = std::chrono::high_resolution_clock::now();

            // restoring the main audio bus to the default value
            project.audioBusesMutex->lock();
            for (auto& bus : buses) {
                if (bus.samples.size() != 4096 * AudioInfo::s_channels) {
                    bus.samples.resize(4096 * AudioInfo::s_channels);
                }
                if (bus.main) mainBusID = bus.id;
                for (int i = 0; i < t_frameCount * AudioInfo::s_channels; i++) {
                    bus.samples[i] = 0.0f;
                }
            }
            project.audioBusesMutex->unlock();

            AudioInfo::s_audioPassID++;

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
                            for (int i = 0; i < t_frameCount * AudioInfo::s_channels; i++) {
                                redirectBus->samples[i] += bus.samples[i];
                            }
                        }
                    }
                }
            }

            // copying samples from the main bus to the audio backend
            for (auto& bus : buses) {
                if (bus.main) {
                    memcpy(fOutput, bus.samples.data(), t_frameCount * AudioInfo::s_channels * sizeof(float));
                    break;
                }
            }

            auto timeDifference = (float) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - firstTime).count();
            auto allowedMsPerCall = (1.0 / ((double) AudioInfo::s_sampleRate / (double) AudioInfo::s_periodSize)) * 1000 / 2;
            if (timeDifference > allowedMsPerCall) {
                s_stretcher.reset();
                s_stretcher.setTimeRatio(1 + (timeDifference * 1.5f / allowedMsPerCall));
                static std::vector<std::vector<float>> s_planarBuffers;
                if (s_planarBuffers.size() != AudioInfo::s_channels) {
                    s_planarBuffers.resize(AudioInfo::s_channels);
                    for (auto& planarBuffer : s_planarBuffers) {
                        planarBuffer.resize(AudioInfo::s_periodSize);
                    }
                }

                std::vector<float*> rawPlanarBuffers;
                for (auto& planarBuffer : s_planarBuffers) {
                    rawPlanarBuffers.push_back(planarBuffer.data());
                }

                ma_deinterleave_pcm_frames(ma_format_f32, AudioInfo::s_channels, AudioInfo::s_periodSize, t_output, (void**) rawPlanarBuffers.data());
                s_stretcher.process(rawPlanarBuffers.data(), AudioInfo::s_periodSize, false);
            }

            project.audioBusesMutex->unlock();
        }
    }

    void Audio::Initialize(int t_channelCount, int t_sampleRate) {
        s_channelCount = t_channelCount;
        s_sampleRate = t_sampleRate;

        ma_device_config deviceConfig = ma_device_config_init(ma_device_type_playback);
        deviceConfig.playback.format = ma_format_f32;
        deviceConfig.playback.channels = t_channelCount;
        deviceConfig.sampleRate = t_sampleRate;
        deviceConfig.dataCallback = raster_data_callback;
        deviceConfig.pUserData = nullptr;
        deviceConfig.periodSizeInFrames = 4096;

        AudioInfo::s_channels = t_channelCount;
        AudioInfo::s_sampleRate = t_sampleRate;
        AudioInfo::s_periodSize = deviceConfig.periodSizeInFrames;
    
        if (ma_device_init(nullptr, &deviceConfig, &s_device) != MA_SUCCESS) {
            RASTER_LOG("failed to create audio playback!");
        } else {
            if (ma_device_start(&s_device) != MA_SUCCESS) {
                RASTER_LOG("failed to start audio playback!");
            }
        }

        s_backendInfo.name = "miniaudio";
        s_backendInfo.version = MA_VERSION_STRING;
    }
};