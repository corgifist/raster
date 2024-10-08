#pragma once

#include "raster.h"

namespace Raster {
    struct SharedMutex {
        SharedMutex();

        void Lock();
        void Unlock();

    private:
        std::shared_ptr<std::mutex> m_mutex;
    };

    struct SharedLockGuard {
        SharedLockGuard(SharedMutex t_mutex);
        ~SharedLockGuard();
    private:
        SharedMutex m_sharedMutex;
    };
};