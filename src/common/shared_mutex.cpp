#include "common/shared_mutex.h"

namespace Raster {
    SharedMutex::SharedMutex() {
        this->m_mutex = std::make_shared<std::mutex>();
    }

    void SharedMutex::Lock() {
        this->m_mutex->lock();
    }

    void SharedMutex::Unlock() {
        this->m_mutex->unlock();
    }

    SharedLockGuard::SharedLockGuard(SharedMutex t_mutex) {
        this->m_sharedMutex = t_mutex;
        m_sharedMutex.Lock();
    }

    SharedLockGuard::~SharedLockGuard() {
        m_sharedMutex.Unlock();
    }
}