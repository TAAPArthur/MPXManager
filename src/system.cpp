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
void safePipe(int* fds) {
    if(pipe2(fds, O_CLOEXEC)) {
        err(SYS_CALL_FAILED, "Ran out of file descriptors");
    }
}

static inline void setEnvRect(const char* name, const Rect& rect) {
    const char var[4][32] = {"_%s_X", "_%s_Y", "_%s_WIDTH", "_%s_HEIGHT"};
    char strName[32];
    char strValue[16];
    for(int n = 0; n < 4; n++) {
        sprintf(strName, var[n], name);
        sprintf(strValue, "%ud", rect[n]);
        setenv(strName, strValue, 1);
    }
}
void setClientMasterEnvVar(void) {
    static char strValue[32];
    if(getActiveMaster()) {
        sprintf(strValue, "%ud", getActiveMasterKeyboardID());
        setenv(DEFAULT_KEYBOARD_ENV_VAR_NAME, strValue, 1);
        sprintf(strValue, "%ud", getActiveMasterPointerID());
        setenv(DEFAULT_POINTER_ENV_VAR_NAME, strValue, 1);
        if(getFocusedWindow()) {
            sprintf(strValue, "%ud", getFocusedWindow()->getID());
            setenv("_WIN_ID", strValue, 1);
            setenv("_WIN_TITLE", getFocusedWindow()->getTitle().c_str(), 1);
            setenv("_WIN_CLASS", getFocusedWindow()->getClassName().c_str(), 1);
            setenv("_WIN_INSTANCE", getFocusedWindow()->getInstanceName().c_str(), 1);
            setEnvRect("WIN", getFocusedWindow()->getGeometry());
        }
        Monitor* m = getActiveWorkspace() ? getActiveWorkspace()->getMonitor() : NULL;
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

int notify(std::string summary, std::string body) {
    return spawn((NOTIFY_CMD + " '" + summary + "' '" + body + "'").c_str());
}


static int _spawn(const char* command, ChildRedirection spawnPipe, bool preserveSession = 0, bool silent = 0) {
    INFO("Spawning command " << (command ? command : "(nil)"));
    if(spawnPipe) {
        if(spawnPipe == REDIRECT_CHILD_INPUT_ONLY || spawnPipe == REDIRECT_BOTH) {
            DEBUG("Creating input pipes");
            safePipe(statusPipeFD);
        }
        if(spawnPipe == REDIRECT_CHILD_OUTPUT_ONLY || spawnPipe == REDIRECT_BOTH) {
            DEBUG("Creating output pipes");
            safePipe(statusPipeFD + 2);
        }
    }
    int pid = fork();
    if(pid == 0) {
        if(!preserveSession)
            if(fork())
                exit(0);
        if(spawnPipe) {
            if(spawnPipe == REDIRECT_CHILD_INPUT_ONLY || spawnPipe == REDIRECT_BOTH) {
                dup2(STATUS_FD_EXTERNAL_READ, STDIN_FILENO);
            }
            if(spawnPipe == REDIRECT_CHILD_OUTPUT_ONLY || spawnPipe == REDIRECT_BOTH)
                dup2(STATUS_FD_EXTERNAL_WRITE, STDOUT_FILENO);
        }
        if(onChildSpawn)
            onChildSpawn();
        if(silent)
            suppressOutput();
        if(command == NULL)
            return 0;
        const char* const args[] = {SHELL.c_str(), "-c", command, NULL};
        execv(SHELL.c_str(), (char* const*)args);
        err(SYS_CALL_FAILED, "exec failed; Aborting");
    }
    else if(pid < 0)
        err(SYS_CALL_FAILED, "error forking\n");
    if(spawnPipe) {
        if(spawnPipe == REDIRECT_CHILD_OUTPUT_ONLY || spawnPipe == REDIRECT_BOTH) {
            close(STATUS_FD_EXTERNAL_WRITE);
        }
        if(spawnPipe == REDIRECT_CHILD_INPUT_ONLY || spawnPipe == REDIRECT_BOTH) {
            close(STATUS_FD_EXTERNAL_READ);
        }
    }
    if(!preserveSession)
        waitForChild(pid);
    return pid;
}
int spawn(const char* command) {
    return spawn(command, 0, 0);
}
int spawn(const char* command, bool preserveSession, bool silent) {
    return _spawn(command, NO_REDIRECTION, preserveSession, silent);
}
int spawnPipe(const char* command, ChildRedirection redirection, bool preserveSession) {
    return _spawn(command, redirection, preserveSession);
}

int waitForChild(int pid) {
    LOG(LOG_LEVEL_DEBUG, "Waiting for process: %d", pid);
    int status = 0;
    waitpid(pid, &status, 0);
    int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : WIFSIGNALED(status) ? WTERMSIG(status) : -1;
    LOG(LOG_LEVEL_DEBUG, "Process exited with %d status %d", pid, status);
    return exitCode;
}

void restart() {
    DEBUG("calling execv");
    if(passedArguments)
        execv(passedArguments[0], passedArguments);
    err(SYS_CALL_FAILED, "exec failed; Aborting");
}
void quit(int exitCode) {
    DEBUG("Exiting");
    exit(exitCode);
}

static void handler(int sig) {
    LOG(LOG_LEVEL_ERROR, "Error: signal %d:", sig);
    LOG_RUN(LOG_LEVEL_WARN, printStackTrace());
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
    createSigAction(SIGSEGV, handler);
    createSigAction(SIGABRT, handler);
    createSigAction(SIGTERM, handler);
    createSigAction(SIGPIPE, [](int) {STATUS_FD = 0; INFO("Received SIGPIPE");});
    createSigAction(SIGHUP, [](int) {restart();});
    createSigAction(SIGUSR1, [](int) {restart();});
    createSigAction(SIGUSR2, [](int) {printStackTrace();});
}
