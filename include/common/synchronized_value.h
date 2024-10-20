#pragma once

#include "raster.h"
#include "shared_mutex.h"

namespace Raster {

    // Helper class which provides thread-safe way to store/modify some value
    template<typename T>
    struct SynchronizedValue {
    private:
        std::shared_ptr<T> m_value;
        SharedMutex m_mutex;

    public:

        void Set(T t_value) {
            SharedLockGuard sync(m_mutex);
            *this->m_value =  t_value;
        }

        T Get() {
            SharedLockGuard sync(m_mutex);
            return *this->m_value;   
        }

        void Lock() {
            m_mutex.Lock();
        }

        void Unlock() {
            m_mutex.Unlock();
        }

        T& GetReference() {
            return *this->m_value;
        }

        SynchronizedValue() {
            this->m_value = std::make_shared<T>();
        }
        SynchronizedValue(T t_value) : SynchronizedValue() {
            Set(t_value);
        }
    };
};