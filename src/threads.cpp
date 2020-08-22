#include <assert.h>

#include <functional>
#include <pthread.h>
#include "time.h"

#include "threads.h"

static pthread_mutex_t globalMutex = PTHREAD_MUTEX_INITIALIZER;

void lock(void) {
    pthread_mutex_lock(&globalMutex);
}

void unlock(void) {
    pthread_mutex_unlock(&globalMutex);
}

struct _CondMutex {
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t  condVar = PTHREAD_COND_INITIALIZER;
};
ThreadSignaler::ThreadSignaler() {
    condMutex = new _CondMutex();
}
void ThreadSignaler::signal() {
    pthread_mutex_lock(&condMutex->mutex);
    ready = 1;
    pthread_mutex_unlock(&condMutex->mutex);
    pthread_cond_signal(&condMutex->condVar);
}
void ThreadSignaler::justWait() {
    pthread_mutex_lock(&condMutex->mutex);
    while(!ready) {
        pthread_cond_wait(&condMutex->condVar, &condMutex->mutex);
    }
    pthread_mutex_unlock(&condMutex->mutex);
    ready = 0;
}

struct PthreadFuncWrapper {
    std::function<void()>func;
};
void* _funcRunner(void* wrapper) {
    PthreadFuncWrapper* func = (PthreadFuncWrapper*)wrapper;
    func->func();
    delete func;
    return NULL;
}
pthread_t _spawnThread(std::function<void()>func) {
    PthreadFuncWrapper* wrapper = new PthreadFuncWrapper{func};
    pthread_t thread;
    pthread_create(&thread, NULL, _funcRunner, wrapper);
    return thread;
}
#ifndef NDEBUG
static std::vector<pthread_t> threads;
void spawnThread(std::function<void()>func) {
    threads.push_back(_spawnThread(func));
}
void waitForAllThreadsToExit() {
    for(auto& thread : threads)
        pthread_join(thread, NULL);
    threads.clear();
}

#else
void spawnThread(std::function<void()>func) {
    pthread_detach(_spawnThread(func));
}
#endif
