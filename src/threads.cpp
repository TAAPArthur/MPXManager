#include <assert.h>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "arraylist.h"
#include "logger.h"
#include "threads.h"

static std::mutex globalMutex;

void lock(void) {
    globalMutex.lock();
}

void unlock(void) {
#ifndef NDEBUG
    std::cout << std::flush;
#endif
    globalMutex.unlock();
}

struct _CondMutex {
    std::mutex m;
    std::condition_variable cv;
};
ThreadSignaler::ThreadSignaler() {
    condMutex = new _CondMutex();
}
void ThreadSignaler::signal() {
    {
        std::lock_guard<std::mutex> lk(condMutex->m);
        ready = 1;
    }
    condMutex->cv.notify_one();
}
void ThreadSignaler::justWait() {
    std::unique_lock<std::mutex> lk(condMutex->m);
    condMutex->cv.wait(lk, [&] {return ready;});
    ready = 0;
}
void spawnThread(std::function<void()>func) {
    std::thread thread(func);
    thread.detach();
}
