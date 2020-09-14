#include <unistd.h>

#include "tester.h"
#include "test-event-helper.h"
#include "test-x-helper.h"
#include "../communications.h"

int _main(int argc, char* const* argv) ;

int mainAlias(int argc, char* const argv[]);

static void setupRules() {
    addInterClientCommunicationRule();
    initOptions();
    setLogLevel(LOG_LEVEL_TRACE);
}
SCUTEST_SET_ENV(setupRules, cleanupXServer);
#define MAIN(ARGS...) _main(LEN( ((char* const []){ARGS}))+1, (char* const []){ "" __FILE__, ARGS, NULL})
int fakeMain() {
    return _main(1, NULL);
}

SCUTEST(startup) {
    MAIN("--no-event-loop");
}
SCUTEST_ERR(startup_bad_option, INVALID_OPTION) {
    MAIN("--bad-option");
}

SCUTEST(startup_no_args) {
    addShutdownOnIdleRule();
    addEvent(X_CONNECTION, DEFAULT_EVENT(incrementCount));
    assertEquals(0, fakeMain());
    assertEquals(getCount(), 1);
}
static char* autoCompleteHelpers [] = {"--list-start-options", "--list-options"};
SCUTEST_ITER(lists, LEN(autoCompleteHelpers)) {
    assertEquals(0, MAIN(autoCompleteHelpers[_i]));
}
SCUTEST_ERR(wm_no_response, WM_NOT_RESPONDING) {
    assert(MAIN("log-level", "0"));
}
static void setup() {
    int pid = spawnPipe(NULL, REDIRECT_CHILD_INPUT_ONLY);
    if(pid) {
        setLogLevel(LOG_LEVEL_WARN);
        addInterClientCommunicationRule();
        MAIN("--no-event-loop");
        createMasterDevice("test");
        initCurrentMasters();
        write(STATUS_FD, &pid, sizeof(pid));
        runEventLoop();
        assert(isShuttingDown());
        assertEquals(0, waitForChild(pid));
        cleanupXServer();
        quit(0);
    }
    else {
        int result = read(STATUS_FD_EXTERNAL_READ, &pid, sizeof(pid));
        assertEquals(result, sizeof(pid));
        clearAllRules();
    }
}
SCUTEST_SET_ENV(setup, cleanupXServer);
SCUTEST(test_send_function, .timeout = 5) {
    assertEquals(0, MAIN("dump"));
    assertEquals(0, MAIN("request-shutdown"));
    INFO("down");
    exit(0);
}
SCUTEST(replace) {
    assertEquals(0, MAIN("--no-event-loop", "--replace"));
}
