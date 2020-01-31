#include "../communications.h"
#include "../extra-rules.h"
#include "../globals.h"
#include "../logger.h"
#include "../wmfunctions.h"

#include "tester.h"
#include "test-event-helper.h"
#include "test-x-helper.h"
int _main(int argc, char* const* argv) ;

int mainAlias(int argc, char* const argv[]);
int fakeMain(ArrayList<const char*>args) {
    args.add("" __FILE__, PREPEND_ALWAYS);
    args.add(NULL);
    int result = _main(args.size() - 1, (char* const*)args.data());
    return result;
}
MPX_TEST_ERR("startup", 0, {
    ArrayList<const char*>args = { "" __FILE__, "--no-event-loop", NULL };
    mainAlias(args.size() - 1, (char* const*)args.data());
    exit(10);
});
MPX_TEST_ERR("startup_bad_option", 1, {
    setLogLevel(LOG_LEVEL_NONE);
    ArrayList<const char*>args = { "" __FILE__, "--bad-option", NULL };
    mainAlias(args.size() - 1, (char* const*)args.data());
    exit(10);
});

MPX_TEST("startup_no_args", {
    requestShutdown();
    getEventRules(X_CONNECTION).add(incrementCount);
    assertEquals(0, fakeMain({}));
    assertEquals(getCount(), 1);
});
MPX_TEST_ITER("lists", 3, {
    suppressOutput();
    const char* arr[] = {"--list-start-options", "--list-options", "--list-vars"};
    assertEquals(0, fakeMain({arr[_i]}));
});
MPX_TEST("startup_set_vars", {
    requestShutdown();
    suppressOutput();
    getEventRules(X_CONNECTION).add(incrementCount);
    assertEquals(0, fakeMain({"--default-mod-mask=0", "--default-number-of-workspaces", "1", "--master-info-path", "/dev/null", "--list-start-options"}));
    assertEquals(getCount(), 1);
    assertEquals(DEFAULT_MOD_MASK, 0);
    assertEquals(DEFAULT_NUMBER_OF_WORKSPACES, 1);
    assertEquals(MASTER_INFO_PATH, "/dev/null");
});
SET_ENV(+[] {setLogLevel(LOG_LEVEL_NONE);}, NULL);
MPX_TEST_ITER_ERR("startup_set_vars_wrong_arg", 2, 1, {
    fakeMain({(_i) ? "default-mod-mask" : "--default-mod-mask", "hi"});
});
MPX_TEST_ITER_ERR("startup_set_vars_missing_arg", 2, 1, {
    fakeMain({(_i) ? "default-mod-mask" : "--default-mod-mask"});
});
MPX_TEST_ITER_ERR("set_var_unknown", 2, 1, {
    assert(fakeMain({(_i) ? "__bad__value" : "--__bad__value"}));
});
MPX_TEST_ITER_ERR("set_var_unknown_with_arg", 2, 1, {
    assert(fakeMain({(_i) ? "__bad__value" : "--__bad__value", "0"}));
});
MPX_TEST_ERR("timeout", 2, {
    assert(fakeMain({"log-level", "0"}));
});
static void setup() {
    addDieOnIntegrityCheckFailRule();
    int number = 1;
    int readNumber = 0;
    auto pid = spawnPipe(NULL, 1);
    if(pid) {
        fakeMain({"--clear-startup-method", "--no-event-loop"});
        enableInterClientCommunication();
        startWM();
        waitUntilIdle(1);
        write(STATUS_FD, &number, sizeof(number));
        int result = read(STATUS_FD_READ, &readNumber, sizeof(readNumber));
        if(result) {
            assertEquals(result, sizeof(readNumber));
            assertEquals(readNumber, POLL_COUNT);
        }
        else
            assert(isShuttingDown());
        assertEquals(0, waitForChild(pid));
        fullCleanup();
        exit(0);
    }
    int result = read(STATUS_FD_EXTERNAL_READ, &readNumber, sizeof(readNumber));
    assertEquals(result, sizeof(readNumber));
    assertEquals(readNumber, number);
    clearAllRules();
}
SET_ENV(setup, fullCleanup);
MPX_TEST("dump", {
    suppressOutput();
    assertEquals(0, fakeMain({"dump"}));
    assertEquals(0, fakeMain({"request-shutdown"}));
});
MPX_TEST("replace", {
    assertEquals(0, fakeMain({"--no-event-loop", "--replace"}));
});
static void cleanup() {
    int n = 0;
    write(STATUS_FD_EXTERNAL_WRITE, &n, sizeof(POLL_COUNT));
    fullCleanup();
}
SET_ENV(setup, cleanup);
MPX_TEST("set-var", {
    assertEquals(0, fakeMain({"poll-count", "0"}));
});
