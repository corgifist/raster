#pragma once

#include <atomic>
#include <mutex>

namespace Raster {
    struct FastMutex {
        //Status of the fast mutex
        std::atomic<bool> locked;
        //helper mutex and vc on which threads can wait in case of collision
        std::mutex mux;
        std::condition_variable cv;
        //the maximum number of threads that might be waiting on the cv (conservative estimation)
        std::atomic<int> cntr; 
    
        FastMutex():locked(false), cntr(0){}
    
        void lock() {
            if (locked.exchange(true)) {
                cntr++;
                {
                    std::unique_lock<std::mutex> ul(mux);
                    cv.wait(ul, [&]{return !locked.exchange(true); });
                }
                cntr--;
            }
        }
        void unlock() {
            locked = false;
            if (cntr > 0){
                std::lock_guard<std::mutex> ul(mux);
                cv.notify_one();
            }
        }
    };
};