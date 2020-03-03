#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "arraylist.h"
#include "debug.h"
#include "globals.h"
#include "logger.h"
#include "masters.h"
#include "monitors.h"
#include "mywm-structs.h"
#include "system.h"
#include "threads.h"
#include "time.h"
#include "windows.h"
#include "workspaces.h"

int statusPipeFD[4] = {0};
int numPassedArguments;
char* const* passedArguments;

void (*onChildSpawn)(void) = setClientMasterEnvVar;
void createStatusPipe() {
    pipe(statusPipeFD);
    pipe(statusPipeFD + 2);
}
void resetPipe() {
    if(statusPipeFD[0]) {
        // other fields were closed right after a call to spawnPipe;
        TRACE("reseting pipe");
        close(STATUS_FD);
        close(STATUS_FD_READ);
        for(int i = LEN(statusPipeFD) - 1; i >= 0; i--)
            statusPipeFD[i] = 0;
    }
}
static inline void setEnvRect(const char* name, const Rect& rect) {
    const char var[4][32] = {"_%s_X", "_%s_Y", "_%s_WIDTH", "_%s_HEIGHT"};
    char strName[32];
    char strValue[8];
    for(int n = 0; n < 4; n++) {
        sprintf(strName, var[n], name);
        sprintf(strValue, "%d", rect[n]);
        setenv(strName, strValue, 1);
    }
}
void setClientMasterEnvVar(void) {
    char strValue[8];
    if(getActiveMaster()) {
        sprintf(strValue, "%d", getActiveMasterKeyboardID());
        setenv(DEFAULT_KEYBOARD_ENV_VAR_NAME, strValue, 1);
        sprintf(strValue, "%d", getActiveMasterPointerID());
        setenv(DEFAULT_POINTER_ENV_VAR_NAME, strValue, 1);
        if(getFocusedWindow()) {
            sprintf(strValue, "%d", getFocusedWindow()->getID());
            setenv("_WIN_ID", strValue, 1);
            setenv("_WIN_TITLE", getFocusedWindow()->getTitle().c_str(), 1);
            setenv("_WIN_CLASS", getFocusedWindow()->getClassName().c_str(), 1);
            setenv("_WIN_INSTANCE", getFocusedWindow()->getInstanceName().c_str(), 1);
            setEnvRect("WIN", getFocusedWindow()->getGeometry());
        }
        Monitor* m = getActiveMonitor();
        if(m) {
            setEnvRect("VIEW", m->getViewport());
            setEnvRect("MON", m->getBase());
            setenv("MONITOR_NAME", m->getName().c_str(), 1);
        }
        const Rect rootBounds = {0, 0, getRootWidth(), getRootHeight()};
        setEnvRect("ROOT", rootBounds);
        if(LD_PRELOAD_INJECTION) {
            std::string newPreload = LD_PRELOAD_PATH.c_str();
            const char* previousPreload = getenv("LD_PRELOAD");
            if(previousPreload)
                newPreload += std::string(" ") + previousPreload ;
            setenv("LD_PRELOAD", newPreload.c_str(), 1);
        }
    }
}

void suppressOutput(void) {
    assert(dup2(open("/dev/null", O_WRONLY | O_APPEND), STDERR_FILENO) == STDERR_FILENO);
    assert(dup2(open("/dev/null", O_WRONLY | O_APPEND), STDOUT_FILENO) == STDOUT_FILENO);
}

void notify(std::string summary, std::string body) {
    spawn((NOTIFY_CMD + " '" + summary + "' '" + body + "'").c_str());
}

static int _spawn(const char* command, bool spawnPipe, bool silent = 0, bool noDup = 0) {
    INFO("Spawning command " << command);
    if(spawnPipe) {
        resetPipe();
        createStatusPipe();
    }
    int pid = fork();
    if(pid == 0) {
        if(spawnPipe) {
            close(STATUS_FD);
            close(STATUS_FD_READ);
            if(!noDup) {
                dup2(STATUS_FD_EXTERNAL_READ, STDIN_FILENO);
                dup2(STATUS_FD_EXTERNAL_WRITE, STDOUT_FILENO);
            }
        }
        else
            resetPipe();
        if(onChildSpawn)
            onChildSpawn();
        if(silent)
            suppressOutput();
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
    return spawn(command, 0);
}
int spawn(const char* command, bool silent) {
    return _spawn(command, 0, silent);
}
int spawnPipe(const char* command, bool noDup) {
    return _spawn(command, 1, 0, noDup);
}

int waitForChild(int pid) {
    LOG(LOG_LEVEL_DEBUG, "Waiting for process: %d", pid);
    int status = 0;
    waitpid(pid, &status, 0);
    int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : WIFSIGNALED(status) ? WTERMSIG(status) : -1;
    LOG(LOG_LEVEL_DEBUG, "Process exited with %d status %d", pid, status);
    return exitCode;
}
void destroyAllLists() {
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
    DEBUG("Shutting down");
    resetPipe();
    destroyAllLists();
}

void restart(bool force) {
    INFO("restarting");
    if(!force)
        stop();
    else
        resetPipe();
    DEBUG("calling execv");
    if(passedArguments)
        execv(passedArguments[0], passedArguments);
    err(1, "exec failed; Aborting");
}
void quit(int exitCode) {
    stop();
    DEBUG("Exiting");
    exit(exitCode);
}

static bool caughtError = 0;
static void handler(int sig) {
    LOG(LOG_LEVEL_ERROR, "Error: signal %d:", sig);
#ifdef NDEBUG
    printStackTrace();
#endif
    if(caughtError) {
        exit(sig);
    }
    caughtError = sig;
    printSummary();
    validate();
    if(sig == SIGSEGV || sig == SIGABRT)
        exit(sig);
    quit(sig);
}

/**
 * Used to create a signal handler for functions that don't return
 *
 * @param sig the signal to catch
 * @param handler the callback function
 *
 * @return 0 on success or the result from sigaction
 */
static int createSigAction(int sig, void(*callback)(int)) {
    struct sigaction sa;
    sa.sa_handler = callback;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESETHAND | SA_NODEFER;
    return sigaction(sig, &sa, NULL);
}

static const char* RESTART_COUNTER_STR = "MPX_RESTART_COUNTER";
static int initRestartCounter() {
    char* counter = getenv(RESTART_COUNTER_STR);
    int value = counter ? strtol(counter, NULL, 10) + 1 : 0;
    setenv(RESTART_COUNTER_STR, std::to_string(value).c_str(), 1);
    return value;
}
const int RESTART_COUNTER = initRestartCounter();
__attribute__((constructor)) static void set_handlers() {
    signal(SIGSEGV, handler);
    signal(SIGABRT, handler);
    signal(SIGTERM, handler);
    signal(SIGPIPE, [](int) {resetPipe();});
    createSigAction(SIGHUP, [](int) {restart(1);});
    createSigAction(SIGUSR1, [](int) {restart(0);});
    signal(SIGUSR2, [](int) {printStackTrace();});
}
