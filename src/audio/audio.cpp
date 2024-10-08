#include "audio/audio.h"

#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"
#include "common/workspace.h"

namespace Raster {

    AudioBackendInfo Audio::s_backendInfo;
    int Audio::s_samplesCount = 0;
    int Audio::s_globalAudioOffset = 0;
    std::mutex Audio::s_audioMutex;

    static int s_channelCount, s_sampleRate;
    static ma_device s_device;

    static void raster_data_callback(ma_device* t_device, void* t_output, const void* t_input, ma_uint32 t_frameCount) {
        Audio::s_audioMutex.lock();
        Audio::s_samplesCount = t_frameCount;
        float* fOutput = (float*) t_output;
        static int s_audioPassID = 0;
        s_audioPassID++;

        if (Workspace::IsProjectLoaded()) {
            auto& project = Workspace::GetProject();
            auto& buses = project.audioBuses;
            int mainBusID = -1;

            // restoring the main audio bus to the default value
            for (auto& bus : buses) {
                if (bus.main) {
                    mainBusID = bus.id;
                    if (bus.samples.size() != 4096 * Audio::GetChannelCount()) {
                        bus.samples.resize(4096 * Audio::GetChannelCount());
                    }
                    for (int i = 0; i < t_frameCount * Audio::GetChannelCount(); i++) {
                        bus.samples[i] = 0.0f;
                    }
                    break;
                }
            }


            ContextData context;
            context["AUDIO_PASS"] = true;
            context["AUDIO_PASS_ID"] = s_audioPassID;

            project.Traverse(context);

            project.audioBusesMutex->lock();

            // redirecting audio buses
            for (auto& bus : buses) {
                if (!bus.main && bus.redirectID >= 0) {
                    auto redirectBusCandidate = Workspace::GetAudioBusByID(bus.redirectID);
                    if (redirectBusCandidate.has_value()) {
                        auto& redirectBus = redirectBusCandidate.value();
                        for (int i = 0; i < t_frameCount * Audio::GetChannelCount(); i++) {
                            redirectBus->samples[i] += bus.samples[i];
                        }
                    }
                }
            }

            // copying samples from the main bus to the audio backend
            for (auto& bus : buses) {
                if (bus.main) {
                    memcpy(fOutput, bus.samples.data(), t_frameCount * Audio::GetChannelCount() * sizeof(float));
                    break;
                }
            }

            project.audioBusesMutex->unlock();
        }
        Audio::s_audioMutex.unlock();
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
        deviceConfig.noPreSilencedOutputBuffer = true;
        deviceConfig.periodSizeInFrames = 1024;
    
        if (ma_device_init(nullptr, &deviceConfig, &s_device) != MA_SUCCESS) {
            std::cout << "failed to create audio playback!" << std::endl;
        } else {
            if (ma_device_start(&s_device) != MA_SUCCESS) {
                std::cout << "failed to start audio playback!" << std::endl;
            }
        }

        s_backendInfo.name = "miniaudio";
        s_backendInfo.version = MA_VERSION_STRING;
    }

    int Audio::GetChannelCount() {
        return s_channelCount;
    }
    
    int Audio::GetSampleRate() {
        return s_sampleRate;
    }

    int Audio::ClampAudioIndex(int t_index) {
        if (t_index >= s_samplesCount) return ClampAudioIndex(t_index - s_samplesCount);
        return t_index;
    }

    SharedRawAudioSamples Audio::MakeRawAudioSamples() {
        return std::make_shared<std::vector<float>>(Audio::s_samplesCount * Audio::GetChannelCount());
    }
};