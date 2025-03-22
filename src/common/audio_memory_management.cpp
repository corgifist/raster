#include "common/audio_memory_management.h"
#include "common/thread_unique_value.h"
#include <thread>

namespace Raster {

    struct AudioHeapState {
        uint8_t* base;
        uint8_t* current;
        size_t size;
        AudioHeapState() : base(nullptr), current(nullptr), size(0) {}
    };

    static size_t s_requiredSize;
    static ThreadUniqueValue<AudioHeapState> s_heapState;
    static std::vector<uint8_t*> s_heaps;

    void AudioMemoryManagement::Initialize(size_t t_bytes) {
        RASTER_LOG("initializing " << t_bytes << " bytes for audio heap (" << t_bytes / 1024 / 1024 << " megabytes)");
        s_requiredSize = t_bytes;
    }

    void* AudioMemoryManagement::Allocate(size_t t_bytes) {
        auto& heap = s_heapState.Get();
        if (!heap.base) {
            RASTER_LOG("allocating " << s_requiredSize << " bytes for audio heap  (thread id: " << std::this_thread::get_id() << "; " << s_requiredSize / 1024 / 1024 << " megabytes)");
            heap.base = new uint8_t[s_requiredSize];
            heap.current = heap.base;
            heap.size = s_requiredSize;
            s_heaps.push_back(heap.base);
        }
        if (heap.size - (size_t) (heap.current - heap.base) < t_bytes) {
            RASTER_LOG("audio heap overflow!!!");
            RASTER_LOG("audio heap size: " << heap.size);
            RASTER_LOG("requested heap region size: " << t_bytes);
            return nullptr;
        }
        auto result = heap.current;
        heap.current += t_bytes;
        return result;
    }

    void AudioMemoryManagement::Reset() {
        auto& heap = s_heapState.Get();
        heap.current = heap.base;
    }

    void AudioMemoryManagement::Terminate() {
        RASTER_LOG("terminating all audio heaps");
        for (auto& heap : s_heaps) {
            delete[] heap;
        }
        s_heaps.clear();
        s_heapState = ThreadUniqueValue<AudioHeapState>();
    }
};