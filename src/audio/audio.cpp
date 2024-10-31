#include "audio/audio.h"

#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "common/workspace.h"
#include "common/audio_info.h"
#include "common/threads.h"

namespace Raster {

    AudioBackendInfo Audio::s_backendInfo;

    static int s_channelCount, s_sampleRate;
    static ma_device s_device;

    static void raster_data_callback(ma_device* t_device, void* t_output, const void* t_input, ma_uint32 t_frameCount) {
        Threads::s_audioThreadID = std::this_thread::get_id();
        SharedLockGuard guard(AudioInfo::s_mutex);
        float* fOutput = (float*) t_output;

        if (Workspace::IsProjectLoaded() && Workspace::GetProject().playing ) {
            auto& project = Workspace::GetProject();
            auto& buses = project.audioBuses;
            int mainBusID = -1;

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

            ContextData context;
            context["AUDIO_PASS"] = true;
            context["AUDIO_PASS_ID"] = AudioInfo::s_audioPassID;

            project.Traverse(context);

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