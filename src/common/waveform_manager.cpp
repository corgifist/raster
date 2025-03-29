#include "common/waveform_manager.h"
#include "common/audio_info.h"
#include "common/audio_memory_management.h"
#include "common/synchronized_value.h"
#include "common/workspace.h"
#include "raster.h"

#define WAVEFORM_PRECISION 256

namespace Raster {

    static bool s_running = false;
    static std::thread s_waveformManagerThread;
    static SynchronizedValue<std::vector<int>> s_refreshTargets;

    static SynchronizedValue<std::unordered_map<int, WaveformRecord>> s_waveformRecords;

    struct AudioAccumulator {
    private:
        std::vector<float> m_accumulator;
    
    public:
        AudioAccumulator() {}

        void PushSamples(std::vector<float>& t_samples) {
            for (auto& sample : t_samples) {
                m_accumulator.push_back(sample);
            }
        }

        std::optional<std::vector<float>> PopSamples(size_t t_samplesCount) {
            if (m_accumulator.size() < t_samplesCount) return std::nullopt;
            auto result = std::vector<float>(m_accumulator.data(), m_accumulator.data() + t_samplesCount);
            m_accumulator.erase(m_accumulator.begin(), m_accumulator.begin() + t_samplesCount);
            return result;
        }

        size_t GetSamplesCount() {
            return m_accumulator.size();
        }
    };

    void WaveformManager::Initialize() {
        s_running = true;
        RASTER_LOG("starting waveform manager");
        s_waveformManagerThread = std::thread(WaveformManager::ManagerLoop);
    }

    void WaveformManager::RequestWaveformRefresh(int t_compositionID) {
        auto compositionCandidate = Workspace::GetCompositionByID(t_compositionID);
        if (!compositionCandidate) return;
        // if (!compositionCandidate.value()->DoesAudioMixing()) return;
        s_refreshTargets.Lock();
        auto& refreshTargets = s_refreshTargets.GetReference();
        if (std::find(refreshTargets.begin(), refreshTargets.end(), t_compositionID) == refreshTargets.end())
            refreshTargets.push_back(t_compositionID);
        s_refreshTargets.Unlock();
    }

    static std::vector<float> s_waveformSamplesBuffer;

    static void ClearWaveformSamples() {
        for (auto& sample : s_waveformSamplesBuffer) {
            sample = 0.0f;
        }
        AudioMemoryManagement::Reset();
    }

    void WaveformManager::PushWaveformSamples(SharedRawInterleavedAudioSamples t_samples) {
        if (t_samples->size() != s_waveformSamplesBuffer.size()) {
            RASTER_LOG("waveform buffer size mismatch!");
            return;
        }
        for (int i = 0; i < t_samples->size(); i++) {
            s_waveformSamplesBuffer[i] += (*t_samples)[i];
        }
    }

    SynchronizedValue<std::unordered_map<int, WaveformRecord>>& WaveformManager::GetRecords() {
        return s_waveformRecords;
    }

    void WaveformManager::EraseRecord(int t_compositionID) {
        s_waveformRecords.Lock();
        auto& waveformRecords = s_waveformRecords.GetReference();
        if (waveformRecords.find(t_compositionID) != waveformRecords.end()) {
            waveformRecords.erase(t_compositionID);
        }
        s_waveformRecords.Unlock();
    }

    static void ComputeWaveform(int t_compositionID) {
        // return;
        auto& project = Workspace::GetProject();
        auto compositionCandidate = Workspace::GetCompositionByID(t_compositionID);
        if (s_waveformSamplesBuffer.size() != AudioInfo::s_periodSize * AudioInfo::s_channels) {
            s_waveformSamplesBuffer.resize(AudioInfo::s_periodSize * AudioInfo::s_channels);
        }
        // DUMP_VAR(t_compositionID);
        if (compositionCandidate) {
            auto& composition = *compositionCandidate;
            if (!composition->enabled || !composition->audioEnabled || !composition->DoesAudioMixing()) {
                WaveformManager::EraseRecord(t_compositionID);
                return;
            }
            std::vector<float> accumulatedWaveform;
            int waveformPassID = 1;
            AudioAccumulator audioAccumulator;
            float currentFakeTime = composition->GetBeginFrame();
            bool firstCall = true;
            while (true) {
                if (!IsInBounds(currentFakeTime, composition->GetBeginFrame() - 1, composition->GetEndFrame() + 1)) {
                    // RASTER_LOG("fake time is out of bounds: " << currentFakeTime);
                    break;
                }
                if (!composition->enabled) break;
                project.ResetTimeTravel();
                project.SetFakeTime(composition->GetBeginFrame() + composition->MapTime(currentFakeTime - composition->GetBeginFrame()));
                // DUMP_VAR(currentFakeTime);
                ClearWaveformSamples();
                // RASTER_SYNCHRONIZED(Workspace::s_nodesMutex);
                composition->Traverse({
                    {"AUDIO_PASS", true},
                    {"AUDIO_PASS_ID", waveformPassID++},
                    {"WAVEFORM_PASS", true},
                    {"WAVEFORM_FIRST_PASS", firstCall},
                    {"INCREMENT_EPF", false},
                    {"RESET_WORKSPACE_STATE", false},
                    {"ALLOW_MEDIA_DECODING", true},
                    {"ONLY_AUDIO_NODES", true}, 
                    {"MANUAL_SPEED_CONTROL", true}
                });
                AudioMemoryManagement::Reset();
                firstCall = false;
                audioAccumulator.PushSamples(s_waveformSamplesBuffer);
                bool mustBreak = false;
                while (true) {
                    auto samplesCandidate = audioAccumulator.PopSamples(WAVEFORM_PRECISION * AudioInfo::s_channels);
                    // DUMP_VAR(audioAccumulator.GetSamplesCount());
                    if (samplesCandidate) {
                        auto& samples = *samplesCandidate;
                        float waveformAverage = 0.0;
                        for (int i = 0; i < samples.size(); i++) {
                            // DUMP_VAR(samples[i]);
                            /* float convertedDecibel = std::abs(60 - std::min(std::abs(LinearToDecibel(std::abs(samples[i]))), 60.0f));
                            waveformAverage = (waveformAverage + convertedDecibel) / 2.0f; */
                            waveformAverage = (waveformAverage + std::abs(samples[i])) / 2.0f;
                        }

                        // DUMP_VAR(waveformAverage);
                        accumulatedWaveform.push_back(std::clamp(waveformAverage, 0.0f, 1.0f));
                    } else {
                        // RASTER_LOG("audio accumulator samples are invalid");
                        break;
                    }
                }
                if (mustBreak) {
                    // RASTER_LOG("compelled terminating");
                    break;
                }
                currentFakeTime += ((float) AudioInfo::s_periodSize / (float) AudioInfo::s_sampleRate) * project.framerate;
            }
            // DUMP_VAR(currentFakeTime);
            project.ResetFakeTime();

            s_waveformRecords.Lock();
            WaveformRecord newRecord;
            newRecord.data = accumulatedWaveform;
            newRecord.precision = WAVEFORM_PRECISION;
            s_waveformRecords.GetReference()[t_compositionID] = newRecord;
            s_waveformRecords.Unlock();
        }
    }

    void WaveformManager::ManagerLoop() {
        while (s_running) {
            if (!Workspace::IsProjectLoaded()) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }
            auto& project = Workspace::GetProject();
            s_refreshTargets.Lock();
            auto refreshTargets = s_refreshTargets.GetReference();
            s_refreshTargets.GetReference().clear();
            s_refreshTargets.Unlock();
            if (refreshTargets.empty()) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }
            for (auto& refreshTarget : refreshTargets) {
                ComputeWaveform(refreshTarget);
            }
        }
    }

    void WaveformManager::Terminate() {
        s_running = false;
        s_waveformManagerThread.join();
    }
};