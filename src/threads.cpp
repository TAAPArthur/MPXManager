#include <assert.h>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "arraylist.h"
#include "logger.h"
#include "threads.h"

static ArrayList<std::function<void()>>wakeupFunctions;
void registerWakeupFunction(void(*func)()) {
    wakeupFunctions.add(func);
}
void registerWakeupFunction(std::function<void()>func) {
    wakeupFunctions.add(func);
}

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
    registerWakeupFunction([&] {this->signal();});
}
void ThreadSignaler::signal(std::function<void()>func, bool all) {
    {
        std::lock_guard<std::mutex> lk(condMutex->m);
        func();
    }
    if(all)
        condMutex->cv.notify_all();
    else
        condMutex->cv.notify_one();
}
void ThreadSignaler::justWait(std::function<bool()>func, bool noResetSignal) {
    std::unique_lock<std::mutex> lk(condMutex->m);
    condMutex->cv.wait(lk, func);
    if(!noResetSignal && !isShuttingDown())
        ready = 0;
}

/// holds thread metadata
struct Thread {
    /// thread id
    std::thread thread;
    /// user specified name of thread
    const char* name;
} ;
static ArrayList<Thread*> threads __attribute__((unused)) ;
void spawnThread(std::function<void()>func, const char* name) {
    threads.add(new Thread{std::thread(func), name});
}
int getNumberOfThreads(void) {return threads.size();}
void waitForAllThreadsToExit(void) {
    auto currentThreadID = std::this_thread::get_id();
    LOG(LOG_LEVEL_DEBUG, "Running %d wakeup functions for %d threads", wakeupFunctions.size(), threads.size());
    for(auto func : wakeupFunctions)
        func();
    while(threads.size()) {
        Thread* thread = threads.pop();
        LOG(LOG_LEVEL_DEBUG, "Waiting for thread '%s' and %d more threads", thread->name, threads.size());
        if(thread->thread.get_id() != currentThreadID) {
            thread->thread.join();
        }
        delete thread;
    }
    DEBUG("Finished waiting on threads");
}
