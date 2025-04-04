#pragma once

#include "raster.h"
#include "synchronized_value.h"

namespace Raster {
    template<typename T>
    struct ThreadUniqueValue {
    private:
        SynchronizedValue<std::unordered_map<std::thread::id, T>> m_values;
    public:
        T& Get() {
            m_values.Lock();
            auto id = std::this_thread::get_id();
            auto& values = m_values.GetReference();
            if (values.find(id) == values.end()) {
                values[id] = T();
            }
            auto& result = values[id];
            m_values.Unlock();
            return result;
        }
    };
};