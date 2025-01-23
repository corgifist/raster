#pragma once

#include "common/typedefs.h"
#include "raster.h"
#include <unordered_map>

#define MAX_CONTEXT_HEALTH 10

namespace Raster {
    template <typename T>
    struct AudioContextStorage {
        std::shared_ptr<T> GetContext(float t_key, ContextData& t_contextData) {
            auto& targetStorage = RASTER_GET_CONTEXT_VALUE(t_contextData, "WAVEFORM_PASS", bool) ? waveformStorage : storage;
            std::vector<float> deadReverbs;
            for (auto& reverb : targetStorage) {
                reverb.second->health--;
                if (reverb.second->health < 0) {
                    deadReverbs.push_back(reverb.first);
                }
            }

            for (auto& deadReverb : deadReverbs) {
                targetStorage.erase(deadReverb);
            }

            float offset = t_key;
            if (targetStorage.find(offset) != targetStorage.end()) {
                auto& reverb = targetStorage[offset];
                reverb->health = MAX_CONTEXT_HEALTH;
                return reverb;
            }

            std::shared_ptr<T> context = std::make_shared<T>();
            targetStorage[offset] = context;
            return targetStorage[offset];
        }
        std::unordered_map<float, std::shared_ptr<T>> storage;
        std::unordered_map<float, std::shared_ptr<T>> waveformStorage;
    };
};