/**
 * @file mywm-util.c
 * @copybrief mywm-util.h
 */


#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>


#include "globals.h"
#include "logger.h"
#include "masters.h"
#include "monitors.h"
#include "mywm-util.h"
#include "spawn.h"
#include "windows.h"
#include "workspaces.h"
#include "xsession.h"

#ifndef NO_PTHREADS
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static int trylock(void){
    return pthread_mutex_trylock(&mutex);
}
void lock(void){
    pthread_mutex_lock(&mutex);
}
void unlock(void){
    pthread_mutex_unlock(&mutex);
}
#else
void lock(void){}
void unlock(void){}
#endif

int numPassedArguments;
char** passedArguments;

void resetContext(void){
    while(isNotEmpty(getAllMonitors()))
        removeMonitor(((Monitor*)getHead(getAllMonitors()))->id);
    while(isNotEmpty(getAllMasters()))
        removeMaster(((Master*)getLast(getAllMasters()))->id);
    while(isNotEmpty(getAllWindows()))
        removeWindow(((WindowInfo*)getLast(getAllWindows()))->id);
    for(int i = 0; i < getNumberOfWorkspaces(); i++){
        clearList(getWindowStack(getWorkspaceByIndex(i)));
        clearList(getLayouts(getWorkspaceByIndex(i)));
    }
    resetWorkspaces();
    addWorkspaces(DEFAULT_NUMBER_OF_WORKSPACES);
}
static volatile int shuttingDown = 0;
void requestShutdown(void){
    shuttingDown = 1;
}
int isShuttingDown(void){
    return shuttingDown;
}
static ArrayList threads __attribute__((unused)) ;
/// holds thread metadata
typedef struct {
#ifndef NO_PTHREADS
    /// pthread id
    pthread_t thread;
#endif
    /// user specified name of thread
    char* name;
} Thread;
void runInNewThread(void* (*method)(void*)__attribute__((unused)), void* arg __attribute__((unused)), char* name){
    if(isShuttingDown())return;
#ifndef NO_PTHREADS
    pthread_t thread;
    int result __attribute__((unused)) = pthread_create(&thread, NULL, method, arg) == 0;
    assert(result);
    assert(sizeof(thread) <= sizeof(void*));
    Thread* t = malloc(sizeof(Thread));
    t->thread = thread;
    t->name = name;
    addToList(&threads, t);
#endif
}
void waitForAllThreadsToExit(void){
#ifndef NO_PTHREADS
    pthread_t self = pthread_self();
    bool relock = 0;
    if(trylock() == 0){
        relock = 1;
        unlock();
    }
    unlock();
    while(getSize(&threads)){
        Thread* thread = pop(&threads);
        LOG(LOG_LEVEL_INFO, "Waiting for thread '%s' and %d more threads\n", thread->name, getSize(&threads));
        if(thread->thread != self)
            pthread_join(thread->thread, NULL);
        free(thread);
    }
    clearList(&threads);
    if(relock)
        lock();
#endif
}
static void stop(void){
    requestShutdown();
    waitForAllThreadsToExit();
    LOG(LOG_LEVEL_INFO, "Shutting down\n");
    resetPipe();
    closeConnection();
    LOG(LOG_LEVEL_INFO, "destroying context\n");
    resetContext();
}

void restart(void){
    LOG(LOG_LEVEL_INFO, "restarting\n");
    stop();
    LOG(LOG_LEVEL_INFO, "calling execv\n");
    execv(passedArguments[0], passedArguments);
}
void quit(int exitCode){
    stop();
    exit(exitCode);
}

static void handler(int sig){
    LOG(LOG_LEVEL_ERROR, "Error: signal %d:\n", sig);
    printStackTrace();
    quit(1);
}

__attribute__((constructor)) static void set_handlers(){
    signal(SIGSEGV, handler);
    signal(SIGABRT, handler);
    signal(SIGPIPE, resetPipe);
}
