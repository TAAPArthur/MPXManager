#include <errno.h>
#include <signal.h>
#include <pthread.h>

#include "../system.h"
#include "../masters.h"
#include "../arraylist.h"
#include "../globals.h"
#include "../logger.h"

#include "tester.h"

MPX_TEST("get_time", {
    unsigned int time = getTime();
    int delay = 12;
    for(int i = 0; i < 10; i++) {
        msleep(delay);
        assert(time + delay <= getTime());
        time = getTime();
    }
})

static int size = 100000;
static int counter = 0;
void* incrementCounter(void* p) {
    int size = *(int*)p;
    for(int i = 0; i < size; i++) {
        lock();
        counter++;
        unlock();
    }
    return nullptr;
}
MPX_TEST_ITER("run_in_new_thread", 2, {
    runInNewThread(incrementCounter, &size, "increment");
    if(_i)
        waitForAllThreadsToExit();
    else ATOMIC(waitForAllThreadsToExit(););
    assert(counter == size);
});
// make volatile
int getSize(ArrayList<long>* list) {return list->size();}
MPX_TEST("thread_lock_unlock", {
    runInNewThread(incrementCounter, &size, "increment");
    runInNewThread(incrementCounter, &size, "increment2");
    assertEquals(getNumberOfThreads(), 2);
    waitForAllThreadsToExit();
    assertEquals(counter, size * 2);
});
MPX_TEST("safe_shutdown", {
    runInNewThread([](void*)->void* {
        while(!isShuttingDown())msleep(100);
        return NULL;
    }, NULL, "Spin Forever");
    requestShutdown();
    waitForAllThreadsToExit();
});

MPX_TEST_ITER_ERR("spawn_child", 2, 10, {
    breakFork(0);
    if(_i)
        spawn("exit 10");
    else spawnPipe("exit 10");
});
MPX_TEST_ITER_ERR("spawn_parent", 2, 0, {
    breakFork(1);
    int pid = _i ? spawn("exit 10") : spawnPipe("exit 10");
    assert(pid == 1);
});
MPX_TEST_ITER_ERR("spawn_env", 2, 0, {

    onChildSpawn = []() {exit(0);};
    int pid = _i ? spawn("exit 10") : spawnPipe("exit 10");
    assert(pid);
});

MPX_TEST("spawn_wait", {
    assert(waitForChild(spawn("exit 0")) == 0);
    assert(waitForChild(spawn("exit 1")) == 1);
    assert(waitForChild(spawn("exit 122")) == 122);
});
MPX_TEST("reset_pipe", {
    spawnPipe("yes");
    for(int i = 0; i < LEN(statusPipeFD); i++)
        for(int j = i + 1; j < LEN(statusPipeFD); j++)
            assert(i != statusPipeFD[j]);
    for(int i : statusPipeFD)
        assert(i);
    resetPipe();
    for(int i : statusPipeFD)
        assert(i == 0);
    waitForChild(0);
});
MPX_TEST("spawn_pipe", {
    const char values[2][256] = {"1\n", "2\n"};
    char buffer[2][256] = {0};
    if(!spawnPipe(NULL)) {
        printf(values[1]);
        read(STDIN_FILENO, buffer[0], LEN(buffer[0]));
        assert(strcmp(buffer[0], values[0]) == 0);
        exit(0);
    }
    dprintf(STATUS_FD, values[0]);
    read(STATUS_FD_READ, buffer[1], LEN(buffer[1]));
    assert(strcmp(buffer[1], values[1]) == 0);
    assert(waitForChild(0) == 0);
});
MPX_TEST("spawn_pipe_close", {
    if(!spawnPipe(NULL)) {
        assert(!close(STATUS_FD_EXTERNAL_READ));
        assert(!close(STATUS_FD_EXTERNAL_WRITE));
        assert(close(STATUS_FD));
        assert(close(STATUS_FD_READ));
        exit(0);
    }
    assert(close(STATUS_FD_EXTERNAL_READ));
    assert(close(STATUS_FD_EXTERNAL_WRITE));
    assert(!close(STATUS_FD));
    assert(!close(STATUS_FD_READ));
    assert(waitForChild(0) == 0);
});
MPX_TEST_ITER("spawn_pipe_child_close", 2, {
    if(!spawnPipe(NULL)) {
        assert(!close(STATUS_FD_EXTERNAL_READ));
        assert(!close(STATUS_FD_EXTERNAL_WRITE));
        exit(0);
    }
    assert(waitForChild(0) == 0);
    char buffer[8] = {0};
    if(_i)
        dprintf(STATUS_FD, "0\n");
    else
        read(STATUS_FD_READ, buffer, LEN(buffer));
});
MPX_TEST("quit", {
    quit(0);
    assert(0);
});
MPX_TEST_ERR("quit_err", 1, {
    quit(1);
});
MPX_TEST("safe_quit", {
    char buffer[8] = {0};
    static const char* value = "safe";
    if(!spawnPipe(NULL)) {
        runInNewThread([](void*)->void* {
            while(!isShuttingDown())msleep(100);
            printf(value);
            return NULL;
        }, NULL, "Spin Forever");
        quit(0);
    }
    assert(waitForChild(0) == 0);
    read(STATUS_FD_READ, buffer, LEN(buffer));
    assert(strcmp(buffer, value) == 0);
});

MPX_TEST_ITER_ERR("restart", 3, 2, {
    static const char* const args[] = {SHELL.c_str(), "-c", "exit 2", NULL};
    if(_i == 2) {
        passedArguments = (char* const*)args;
        numPassedArguments = LEN(args) - 1;
    }
    if(_i)
        restart();
    else
        kill(getpid(), SIGUSR1);
    assert(0);
});
SET_ENV(suppressOutput, NULL);
MPX_TEST_ITER_ERR("spawn_fail", 2, 1, {
    breakFork(-1);
    if(_i)
        spawn("");
    else spawnPipe("");
});
MPX_TEST_ERR("seg_fault", SIGSEGV, {
    kill(getpid(), SIGSEGV);
});
MPX_TEST_ERR("seg_fault_recursive", SIGSEGV, {
    Master* p = (Master*)0xDEADBEEF;
    getAllMasters().add(p);
    kill(getpid(), SIGSEGV);
});
MPX_TEST_ERR("sig_term", SIGTERM, {
    kill(getpid(), SIGTERM);
});
MPX_TEST_ERR("assert_fail", SIGABRT, {
    assert(0);
});
