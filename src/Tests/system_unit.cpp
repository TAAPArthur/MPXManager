#include <errno.h>
#include <pthread.h>
#include <signal.h>

#include "../arraylist.h"
#include "../globals.h"
#include "../logger.h"
#include "../masters.h"
#include "../system.h"
#include "test-mpx-helper.h"
#include "tester.h"

static void breakFork(int i) {
    char strValue[8];
    sprintf(strValue, "%d", i);
    setenv("BREAK_FORK", strValue, 1);
}

static void breakPipe() {
    setenv("BREAK_PIPE", "-1", 1);
}

static void breakExec() {
    setenv("BREAK_EXEC", "-1", 1);
}

MPX_TEST("get_time", {
    unsigned int time = getTime();
    int delay = 12;
    for(int i = 0; i < 10; i++) {
        msleep(delay);
        assert(time + delay <= getTime());
        time = getTime();
    }
})

MPX_TEST("failed_exec_spawn", {
    suppressOutput();
    breakExec();
    assertEquals(SYS_CALL_FAILED, waitForChild(spawn("exit 10")));
});

MPX_TEST_ERR("failed_exec_restart", SYS_CALL_FAILED, {
    suppressOutput();
    breakExec();
    restart();
});

MPX_TEST_ITER_ERR("spawn_child", 2, 10, {
    breakFork(0);
    if(_i)
        spawn("exit 10");
    else spawnPipe("exit 10");
});
MPX_TEST_ITER("spawn_parent", 2, {
    breakFork(1);
    int pid = _i ? spawn("exit 10") : spawnPipe("exit 10");
    assert(pid == 1);
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

MPX_TEST_ITER("spawn", 2, {
    const char value[] = "string";
    char buffer[LEN(value)] = {0};

    if(!spawnPipe(NULL)) {
        if(!spawn(NULL, _i)) {
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

MPX_TEST("spawn_pipe_input_only", {
    const char values[2][256] = {"1", "2"};
    char buffer[2][256] = {0};
    if(!spawnPipe(NULL, REDIRECT_CHILD_INPUT_ONLY)) {
        read(STDIN_FILENO, buffer[0], LEN(buffer[0]));
        assert(strcmp(buffer[0], values[0]) == 0);
        exit(0);
    }
    assert(STATUS_FD);
    dprintf(STATUS_FD, values[0]);
    assertEquals(waitForChild(0), 0);
    assert(dprintf(STATUS_FD, values[0]) < 1);
    assert(STATUS_FD == 0);
});

MPX_TEST("spawn_pipe_output_only", {
    const char values[2][256] = {"1", "2"};
    char buffer[2][256] = {0};
    if(!spawnPipe(NULL, REDIRECT_CHILD_OUTPUT_ONLY)) {
        printf(values[1]);
        exit(0);
    }
    assert(STATUS_FD_READ);
    read(STATUS_FD_READ, buffer[1], LEN(buffer[1]));
    assert(strcmp(buffer[1], values[1]) == 0);
    assertEquals(waitForChild(0), 0);
});

MPX_TEST_ITER("spawn_pipe_death", 4, {
    int pid = spawnPipe(NULL, (ChildRedirection)_i);
    if(!pid) {
        while(true)
            msleep(100);
    }
    kill(pid, SIGKILL);
    assertEquals(waitForChild(0), 9);
});

MPX_TEST_ERR("spawn_pipe_out_fds", SYS_CALL_FAILED, {
    suppressOutput();
    breakPipe();
    spawnPipe(NULL);
});

MPX_TEST("spawn_pipe_close", {
    if(!spawnPipe(NULL)) {
        exit(0);
    }
    INFO(STATUS_FD_EXTERNAL_READ << " " << STATUS_FD << " " << STATUS_FD_READ << " " << STATUS_FD_EXTERNAL_WRITE);
    assert(close(STATUS_FD_EXTERNAL_READ));
    assert(close(STATUS_FD_EXTERNAL_WRITE));
    assert(!close(STATUS_FD));
    assert(!close(STATUS_FD_READ));
    assertEquals(waitForChild(0), 0);
});
MPX_TEST_ITER("spawn_pipe_child_close", 2, {
    if(!spawnPipe(NULL, REDIRECT_BOTH)) {
        exit(0);
    }
    assert(waitForChild(0) == 0);
    dprintf(STATUS_FD, "0\n");
});

MPX_TEST_ITER("spawn_env_on_child_spawn", 2, {
    onChildSpawn = []() {quit(0);};
    int pid = _i ? spawn("exit 10") : spawnPipe("exit 10");
    assert(pid);
});

SET_ENV(NULL, simpleCleanup);
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
        simpleCleanup();
        quit(0);
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
        raise(SIGUSR1);
    assert(0);
});
SET_ENV(suppressOutput, simpleCleanup);
MPX_TEST_ITER_ERR("spawn_fail", 2, SYS_CALL_FAILED, {
    breakFork(-1);
    if(_i)
        spawn("");
    else spawnPipe("");
});
MPX_TEST_ERR("seg_fault", SIGSEGV, {
    raise(SIGSEGV);
});
MPX_TEST_ERR("sig_term", SIGTERM, {
    raise(SIGTERM);
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
