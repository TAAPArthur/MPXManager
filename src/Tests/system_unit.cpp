#include <errno.h>
#include <signal.h>
#include <pthread.h>

#include "../arraylist.h"
#include "../globals.h"
#include "../logger.h"
#include "../masters.h"
#include "../system.h"
#include "test-mpx-helper.h"
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
MPX_TEST("notify", {
    suppressOutput();
    NOTIFY_CMD = "echo";
    notify("110", "");
    assertEquals(0, waitForChild(0));
    notify("", "110");
    assertEquals(0, waitForChild(0));
});

MPX_TEST("spawn_wait", {
    assert(waitForChild(spawn("exit 0")) == 0);
    assert(waitForChild(spawn("exit 1")) == 1);
    assert(waitForChild(spawn("exit 122")) == 122);
});
static void verifyResetPipe() {
    for(int i : statusPipeFD)
        assert(i == 0);
}
MPX_TEST("reset_pipe", {
    spawnPipe("yes");
    for(int i = 0; i < LEN(statusPipeFD); i++)
        for(int j = i + 1; j < LEN(statusPipeFD); j++)
            assert(i != statusPipeFD[j]);
    for(int i : statusPipeFD)
        assert(i);
    resetPipe();
    verifyResetPipe();
    waitForChild(0);
});
MPX_TEST_ITER("spawn", 2, {
    const char value[] = "string";
    char buffer[LEN(value)] = {0};

    if(!spawnPipe(NULL)) {
        if(!spawn(NULL, _i)) {
            verifyResetPipe();
            printf(value);
            exit(0);
        }
        assertEquals(0, waitForChild(0));
        exit(0);
    }
    int result = read(STATUS_FD_READ, buffer, LEN(buffer));
    assert(result != -1);
    if(_i)
        assertEquals(result, 0);
    else
        assert(strcmp(buffer, value) == 0);
    assertEquals(waitForChild(0), 0);
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
    assertEquals(waitForChild(0), 0);
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

MPX_TEST("spawn_env", {
    createSimpleEnv();
    LD_PRELOAD_INJECTION = 1;
    getAllWindows().add(new WindowInfo(1));
    getAllWindows()[0]->addMask(FOCUSABLE_MASK);
    getActiveMaster()->onWindowFocus(getAllWindows()[0]->getID());
    getAllMonitors().add(new Monitor(1, {0, 0, 0, 0}, ""));
    assignUnusedMonitorsToWorkspaces();
    if(!spawn(NULL)) {
        assert(getenv(DEFAULT_KEYBOARD_ENV_VAR_NAME));
        assert(getenv(DEFAULT_POINTER_ENV_VAR_NAME));
        std::string(getenv("LD_PRELOAD")).rfind(LD_PRELOAD_PATH, 0);
    }
    assertEquals(waitForChild(0), 0);
});

MPX_TEST("quit", {
    quit(0);
    assert(0);
});
MPX_TEST_ERR("quit_err", 1, {
    quit(1);
});

MPX_TEST_ITER_ERR("restart", 2, 10, {
    static const char* const args[] = {SHELL.c_str(), "-c", "exit 10", NULL};
    passedArguments = (char* const*)args;
    numPassedArguments = LEN(args) - 1;
    if(_i)
        restart();
    else
        kill(getpid(), SIGUSR1);
    assert(0);
});
SET_ENV(suppressOutput, NULL);
MPX_TEST_ITER("restart_recursive", 3, {
    const char* testKey = "MPX_TEST_KEY";
    if(!getenv(testKey))
        clearenv();
    setenv(testKey, "1", 1);
    setenv("TEST_FUNC", ("restart_recursive." + std::to_string(_i)).c_str(), 1);
    const int COUNTER = 5;
    while(RESTART_COUNTER < COUNTER) {
        if(_i == 0)
            restart();
        else if(_i == 1)
            raise(SIGUSR1);
        else if(_i == 2)
            raise(SIGHUP);
        pause();
    }
    assertEquals(RESTART_COUNTER, COUNTER);
});
MPX_TEST_ITER_ERR("spawn_fail", 2, 1, {
    breakFork(-1);
    if(_i)
        spawn("");
    else spawnPipe("");
});
MPX_TEST_ERR("seg_fault", SIGSEGV, {
    kill(getpid(), SIGSEGV);
});
MPX_TEST_ERR("sig_term", SIGTERM, {
    kill(getpid(), SIGTERM);
});
MPX_TEST_ERR("assert_fail", SIGABRT, {
    assert(0);
});
MPX_TEST_ERR("seg_fault_recursive", SIGSEGV, {
    getAllMasters().add((Master*)0xDEADBEEF);
    getAllWindows().add((WindowInfo*)0xDEADBEEF);
    printSummary();
});
MPX_TEST_ERR("abort_recursive", SIGABRT, {
    WindowInfo* winInfo = new WindowInfo(1);
    WindowInfo* winInfo2 = new WindowInfo(1);
    getAllWindows().add(winInfo);
    getAllWindows().add(winInfo2);
    assert(!validate());
});
