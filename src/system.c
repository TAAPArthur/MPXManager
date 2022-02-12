#include <assert.h>
#include <err.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#include "masters.h"
#include "monitors.h"
#include "mywm-structs.h"
#include "system.h"
#include "util/arraylist.h"
#include "util/debug.h"
#include "util/logger.h"
#include "windows.h"
#include "workspaces.h"

int statusPipeFD[4] = {0};
int numPassedArguments;
const char* const* passedArguments;
static char buffer[255] = {};

void (*onChildSpawn)(void) = NULL;
void safePipe(int* fds) {
    if(pipe(fds)) {
        perror(NULL);
        err(SYS_CALL_FAILED, "Ran out of file descriptors");
    }
    fcntl(fds[0], F_SETFD, FD_CLOEXEC);
    fcntl(fds[1], F_SETFD, FD_CLOEXEC);
    INFO("Created pipes %d\n", fds[1]);
}

void suppressOutput(void) {
    dup2(open("/dev/null", O_WRONLY | O_APPEND), STDERR_FILENO);
    dup2(open("/dev/null", O_WRONLY | O_APPEND), STDOUT_FILENO);
}

static int _spawn(const char* command, ChildRedirection spawnPipe, bool preserveSession, bool silent) {
    INFO("Spawning command %s", command);
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
        if(command == NULL) {
            if(spawnPipe == REDIRECT_CHILD_INPUT_ONLY || spawnPipe == REDIRECT_BOTH)
                close(STATUS_FD);
            if(spawnPipe == REDIRECT_CHILD_OUTPUT_ONLY || spawnPipe == REDIRECT_BOTH)
                close(STATUS_FD_READ);
            return 0;
        }
        const char* const args[] = {SHELL, "-c", command, NULL};
        execv(args[0], (char* const*)args);
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

void spawn(const char* command) {
    _spawn(command, 0, 0, 0);
}
int spawnChild(const char* command) {
    return _spawn(command, NO_REDIRECTION, 1, 0);
}
void spawnPipe(const char* command, ChildRedirection redirection) {
    _spawn(command, redirection, 0, 0);
}
int spawnPipeChild(const char* command, ChildRedirection redirection) {
    return _spawn(command, redirection, 1, 0);
}

int spawnSilent(const char* command) {
    return _spawn(command, 0, 0, 1);
}

int waitForChild(int pid) {
    DEBUG("Waiting for process: %d", pid);
    int status = 0;
    waitpid(pid, &status, 0);
    int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : WIFSIGNALED(status) ? WTERMSIG(status) : -1;
    DEBUG("Process exited with %d status %d", pid, status);
    return exitCode;
}

void restart() {
    DEBUG("calling execv");
    if(passedArguments)
        execv(passedArguments[0], (char**)passedArguments);
    err(SYS_CALL_FAILED, "exec failed; Aborting");
}
void quit(int exitCode) {
    DEBUG("Exiting");
    exit(exitCode);
}

static void handler(int sig) {
    ERROR("Error: signal %d:", sig);
    printSummary();
    if(sig == SIGSEGV || sig == SIGABRT)
        exit(sig);
    quit(sig);
}

int createSigAction(int sig, void(*callback)(int)) {
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
    sprintf(buffer, "%d", value);
    setenv(RESTART_COUNTER_STR, buffer, 1);
    return value;
}
int RESTART_COUNTER;
static void resetPipe() {
    STATUS_FD = 0;
    INFO("Received SIGPIPE");
}
void set_handlers() {
    RESTART_COUNTER = initRestartCounter();
    createSigAction(SIGSEGV, handler);
    createSigAction(SIGABRT, handler);
    createSigAction(SIGTERM, handler);
    createSigAction(SIGPIPE, resetPipe);
    createSigAction(SIGHUP, restart);
    createSigAction(SIGUSR1, restart);
}
