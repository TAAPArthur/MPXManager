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
void signalThread(ThreadSignaler* condMutex) {
    pthread_mutex_lock(&condMutex->mutex);
    condMutex->ready = 1;
    pthread_mutex_unlock(&condMutex->mutex);
    pthread_cond_signal(&condMutex->condVar);
}
void justWait(ThreadSignaler* condMutex) {
    pthread_mutex_lock(&condMutex->mutex);
    while(!condMutex->ready) {
        pthread_cond_wait(&condMutex->condVar, &condMutex->mutex);
    }
    condMutex->ready = 0;
    pthread_mutex_unlock(&condMutex->mutex);
}
void sleepOrWait(ThreadSignaler* signaler, uint32_t condition, long delay) {
    if(condition)
        msleep(delay);
    else
        justWait(signaler);
}

pthread_t _spawnThread(void(*func)()) {
    pthread_t thread;
    pthread_create(&thread, NULL, (void* (*)())func, NULL);
    return thread;
}
#ifndef NDEBUG
#include "arraylist.h"
#include <stdlib.h>
static ArrayList threads;
void spawnThread(void(*func)()) {
    pthread_t* p = malloc(sizeof(pthread_t));
    *p = _spawnThread(func);
    addElement(&threads, p);
}
void waitForAllThreadsToExit() {
    FOR_EACH(pthread_t*, thread, &threads) {
        pthread_join(*thread, NULL);
        free(thread);
    }
    clearArray(&threads);
}

#else
void spawnThread(void(*func)()) {
    pthread_detach(_spawnThread(func));
}
#endif
