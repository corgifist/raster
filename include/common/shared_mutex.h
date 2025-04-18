#pragma once

#include "raster.h"
#include "fast_mutex.h"

namespace Raster {
    struct SharedMutex {
        SharedMutex() {
            this->m_mutex = std::make_shared<FastMutex>();
        }

        void Lock() {
            this->m_mutex->lock();
        }
        void Unlock() {
            this->m_mutex->unlock();
         }

    private:
        std::shared_ptr<FastMutex> m_mutex;
    };

    struct SharedLockGuard {
        SharedLockGuard(SharedMutex t_mutex) {
            this->m_sharedMutex = t_mutex;
            m_sharedMutex.Lock();
        }
        ~SharedLockGuard() {
            this->m_sharedMutex.Unlock();
        }
    private:
        SharedMutex m_sharedMutex;
    };
};