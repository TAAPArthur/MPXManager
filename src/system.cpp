
/**
 * @file system.cpp
 * @copybrief system.h
 */



#include <assert.h>
#include <err.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "globals.h"
#include "arraylist.h"
#include "logger.h"
#include "system.h"
#include "mywm-structs.h"
#include "monitors.h"
#include "windows.h"
#include "workspaces.h"
#include "masters.h"
#include "workspaces.h"
#include "debug.h"

int statusPipeFD[4] = {0};
int numPassedArguments;
char* const* passedArguments;

static volatile int shuttingDown = 0;
void requestShutdown(void) {
    shuttingDown = 1;
}
int isShuttingDown(void) {
    return shuttingDown;
}


void (*onChildSpawn)(void) = NULL;

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static int trylock(void) {
    return pthread_mutex_trylock(&mutex);
}
void lock(void) {
    pthread_mutex_lock(&mutex);
}
void unlock(void) {
    pthread_mutex_unlock(&mutex);
}

/// holds thread metadata
typedef struct {
    /// pthread id
    pthread_t thread;
    /// user specified name of thread
    const char* name;
} Thread;
static ArrayList<Thread*> threads __attribute__((unused)) ;
void runInNewThread(void* (*method)(void*)__attribute__((unused)), void* arg __attribute__((unused)),
                    const char* name) {
    if(isShuttingDown())return;
    pthread_t thread;
    int result __attribute__((unused)) = pthread_create(&thread, NULL, method, arg) == 0;
    assert(result);
    assert(sizeof(thread) <= sizeof(void*));
    Thread* t = new Thread{thread, name};
    threads.add(t);
}
int getNumberOfThreads(void) {return threads.size();}
void waitForAllThreadsToExit(void) {
    pthread_t self = pthread_self();
    bool relock = 0;
    // if already locked, set relock, else lock
    if(trylock()) {
        relock = 1;
    }
    unlock();
    LOG(LOG_LEVEL_INFO, "Thread %ld is waiting on %d threads\n", self, threads.size());
    while(threads.size()) {
        Thread* thread = threads.pop();
        LOG(LOG_LEVEL_INFO, "Waiting for thread '%s' and %d more threads\n", thread->name, threads.size());
        if(thread->thread != self) {
            int result __attribute__((unused)) = pthread_join(thread->thread, NULL);
            assert(result == 0);
        }
        delete thread;
    }
    LOG(LOG_LEVEL_INFO, "Finished waiting on threads\n");
    if(relock)
        lock();
}
void resetPipe() {
    if(statusPipeFD[0]) {
        // other fields were closed right after a call to spawnPipe;
        LOG(LOG_LEVEL_TRACE, "reseting pipe\n");
        close(STATUS_FD);
        close(STATUS_FD_READ);
        for(int i = 0; i < LEN(statusPipeFD); i++)
            statusPipeFD[i] = 0;
    }
}

static int _spawn(const char* command, bool spawnPipe, bool dup = 1) {
    LOG(LOG_LEVEL_INFO, "running command %s\n", command);
    if(spawnPipe) {
        resetPipe();
        pipe(statusPipeFD);
        pipe(statusPipeFD + 2);
    }
    int pid = fork();
    if(pid == 0) {
        if(spawnPipe) {
            close(STATUS_FD);
            close(STATUS_FD_READ);
            if(dup) {
                dup2(STATUS_FD_EXTERNAL_READ, STDIN_FILENO);
                dup2(STATUS_FD_EXTERNAL_WRITE, STDOUT_FILENO);
            }
        }
        else
            resetPipe();
        if(onChildSpawn)
            onChildSpawn();
        if(command == NULL)
            return 0;
        execl(SHELL.c_str(), SHELL.c_str(), "-c", command, (char*)0);
        err(1, "exec failed; Aborting");
    }
    else if(pid < 0)
        err(1, "error forking\n");
    if(spawnPipe) {
        close(STATUS_FD_EXTERNAL_READ);
        close(STATUS_FD_EXTERNAL_WRITE);
    }
    return pid;
}
int spawn(const char* command) {
    return _spawn(command, 0);
}
int spawnPipe(const char* command, bool dup) {
    return _spawn(command, 1, dup);
}

int waitForChild(int pid) {
    int status = 0;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : WIFSIGNALED(status) ? WTERMSIG(status) : -1;
}
void clearAllLists() {
    setActiveMaster(NULL);
    getAllSlaves().deleteElements();
    getAllMasters().deleteElements();
    removeAllWorkspaces();
    getAllWindows().deleteElements();
    getAllMonitors().deleteElements();
}
static void stop(void) {
    requestShutdown();
    waitForAllThreadsToExit();
    LOG(LOG_LEVEL_INFO, "Shutting down\n");
    resetPipe();
    clearAllLists();
}

void restart(void) {
    if(passedArguments) {
        LOG(LOG_LEVEL_INFO, "restarting\n");
        stop();
        LOG(LOG_LEVEL_INFO, "calling execv\n");
        execv(passedArguments[0], passedArguments);
        err(1, "exec failed; Aborting");
    }
    else
        quit(2);
}
void quit(int exitCode) {
    stop();
    LOG(LOG_LEVEL_INFO, "Exiting\n");
    exit(exitCode);
}

static void handler(int sig) {
    LOG(LOG_LEVEL_ERROR, "Error: signal %d:\n", sig);
    printStackTrace();
    printSummary();
    validate();
    quit(sig);
}

__attribute__((constructor)) static void set_handlers() {
    signal(SIGSEGV, handler);
    signal(SIGABRT, handler);
    signal(SIGTERM, handler);
    signal(SIGPIPE, [](int) {resetPipe();});
    signal(SIGUSR1, [](int) {restart();});
    signal(SIGUSR2, [](int) {printStackTrace();});
}