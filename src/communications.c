#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "bindings.h"
#include "communications.h"
#include "devices.h"
#include "functions.h"
#include "layouts.h"
#include "system.h"
#include "util/debug.h"
#include "util/logger.h"
#include "windows.h"
#include "wmfunctions.h"
#include "xevent.h"
#include "xutil/window-properties.h"
#include "xutil/xsession.h"


/**
 *
 * @param str the str to check
 * @param isNumeric if str can be converted to an int
 *
 * @return the interger if isNumeric is set to 1
 */
static int isInt(const char* str, bool* isNumeric) {
    char* end;
    int num = strtol(str, &end, 10);
    if(*end) {
        num = strtol(str, &end, 16);
    }
    if(isNumeric)
        *isNumeric = (*end == 0) && str[0];
    return num;
}

bool matchesOption(Option* o, const char* str, bool empty) {
    if(strcasecmp(o->name, str) == 0) {
        INFO("%s %d %d %d", str, o->flags, empty, (o->flags & (REQUEST_STR | REQUEST_INT) ? 1 : 0) ^ empty);
        INFO("%d %d", o->flags & (REQUEST_STR | REQUEST_INT) ? 1 : 0,  empty);
        return (o->flags & (REQUEST_STR | REQUEST_INT) ? 1 : 0) ^ empty;
    }
    return 0;
}
void callOption(const Option* o, const char* p) {
    TRACE("Calling %s %s", o->name, p);
    uint32_t i = isInt(p, NULL);
    if(o->flags & REQUEST_INT) {
        o->func(i, o->arg);
    }
    else if(o->flags & REQUEST_STR) {
        o->func(p, o->arg);
    }
    else {
        o->func(o->arg);
    }
}

static Option baseOptions[] = {
    {"dump", dumpWindowFilter, MAPPABLE_MASK, .flags = FORK_ON_RECEIVE},
    {"dump", dumpWindowByClass, .flags = FORK_ON_RECEIVE | REQUEST_STR},
    {"dump-master", dumpMaster, 0, .flags = FORK_ON_RECEIVE},
    {"dump-rules", dumpRules,  .flags = FORK_ON_RECEIVE},
    {"dump-win", dumpWindow,  .flags = FORK_ON_RECEIVE | REQUEST_INT},
    //{"find", [](const char * * str) {return findAndRaise(*str, MATCHES_CLASS, ACTION_NONE);}},
    //{"find-and-raise", [](const char * * str) {return findAndRaise(*str, MATCHES_CLASS, ACTION_ACTIVATE);}},
    {"focus-win", (void(*)())focusWindow, .flags = REQUEST_INT},
    {"log-level", setLogLevel, .flags = VAR_SETTER | REQUEST_INT},
    {"lower", lowerWindow, .flags = REQUEST_INT},
    {"next-layout", cycleLayouts, UP},
    {"next-win", shiftFocus, UP},
    {"next-win-of-class", shiftFocusToNextClass, UP},
    {"prev-layout", cycleLayouts, DOWN},
    {"prev-win", shiftFocus, DOWN},
    {"prev-win-of-class", shiftFocusToNextClass, DOWN},
    {"raise", raiseWindow, .flags = REQUEST_INT},
    {"restart", restart, .flags = CONFIRM_EARLY},
    {"sum", printSummary, .flags = FORK_ON_RECEIVE},
    {"switch-workspace", switchToWorkspace},
};
static ArrayList options;

ArrayList* getOptions() {
    return &options;
};

void initOptions() {
    for(int i = 0; i < LEN(baseOptions); i++)
        addElement(getOptions(), baseOptions + i);
}
void addInterClientCommunicationRule() {
    addEvent(XCB_CLIENT_MESSAGE, DEFAULT_EVENT(receiveClientMessage));
}


static int outstandingSendCount;
uint32_t getNumberOfMessageSent() {
    return outstandingSendCount;
}
int getConfirmedSentMessage(WindowID win) {
    int result = getWindowPropertyValueInt(win, MPX_WM_INTERPROCESS_COM, XCB_ATOM_CARDINAL);
    TRACE("Found %d out of %d confirmations", result, getNumberOfMessageSent());
    return result;
}
bool hasOutStandingMessages(void) {
    return getNumberOfMessageSent() > getConfirmedSentMessage(getPrivateWindow());
}
int getLastMessageExitCode(void) {
    return getWindowPropertyValueInt(getPrivateWindow(), MPX_WM_INTERPROCESS_COM_STATUS, XCB_ATOM_CARDINAL);
}
static void sendConfirmation(WindowID target, int exitCode) {
    DEBUG("sending receive confirmation to %d", target);
    // not atomic, but there should be only one thread/process modifying this value
    int count = getConfirmedSentMessage(target) + 1;
    setWindowPropertyInt(target, MPX_WM_INTERPROCESS_COM_STATUS, XCB_ATOM_CARDINAL, exitCode);
    setWindowPropertyInt(target, MPX_WM_INTERPROCESS_COM, XCB_ATOM_CARDINAL, count);
    flush();
}
void send(const char* name, const char* value) {
    sendAs(name, value, 0);
}
void sendAs(const char* name, const char* value, WindowID active) {
    unsigned int data[5] = {getAtom(name), getAtom(value), active};
    DEBUG("sending %d (%s) %d (%s) active: %d", data[0], name, data[1], value, active);
    catchError(xcb_ewmh_send_client_message(dis, getPrivateWindow(), root, MPX_WM_INTERPROCESS_COM, sizeof(data), data));
    outstandingSendCount++;
}
const Option* findOption(const char* name, const char* value) {
    bool empty = !value[0];
    FOR_EACH(Option*, option, getOptions()) {
        if(matchesOption(option, name, empty))
            return option;
    }
    WARN("could not find option matching '%s' '%s' Empty %d", name, value, empty);
    return NULL;
}

void receiveClientMessage(xcb_client_message_event_t* event) {
    xcb_client_message_data_t data = event->data;
    WindowID win = event->window;
    xcb_atom_t message = event->type;
    xcb_atom_t optionAtom = data.data32[0];
    xcb_atom_t valueAtom = data.data32[1];
    WindowID active = data.data32[2];
    if(message == MPX_WM_INTERPROCESS_COM && optionAtom) {
        LOG_RUN(LOG_LEVEL_DEBUG, dumpAtoms(data.data32, 2));
        pid_t callerPID = getWindowPropertyValueInt(win, ewmh->_NET_WM_PID, XCB_ATOM_CARDINAL);
        char name[MAX_NAME_LEN];
        char value[MAX_NAME_LEN] = "";
        getAtomName(optionAtom, name);
        if(valueAtom)
            getAtomName(valueAtom, value);
        const Option* option = findOption(name, value);
        if(active) {
            WindowInfo* winInfo = getWindowInfo(active);
            if(winInfo) {
                INFO("Find client master for %d", winInfo->id);
                setActiveMasterByDeviceID(getClientPointerForWindow(winInfo->id));
            }
            else {
                INFO("Setting active master to %d", active);
                setActiveMasterByDeviceID(active);
            }
        }
        if(option && (ALLOW_UNSAFE_OPTIONS || !(option->flags & UNSAFE))) {
            INFO("Executing %s", option->name);
            int returnValue = 0;
            if(option->flags & CONFIRM_EARLY) {
                TRACE("sending early confirmation");
                destroyWindow(getPrivateWindow());
                sendConfirmation(win, NORMAL_TERMINATION);
                flush();
            }
            fflush(NULL);
            int stdout = -1;
            if((option->flags & FORK_ON_RECEIVE)) {
                fflush(NULL);
                stdout = dup(STDOUT_FILENO);
                char outputFile[64] = {"/dev/null"};
                if(callerPID) {
                    const char* formatter = "/proc/%d/fd/1";
                    sprintf(outputFile, formatter, callerPID);
                }
                int fd = open(outputFile, O_WRONLY | O_APPEND);
                if(fd == -1) {
                    WARN("could not open %s for writing;", outputFile);
                }
                else if(dup2(fd, STDOUT_FILENO) == -1) {
                    WARN("could not open %s for writing; could not call/set '%s' aborting", outputFile,
                        option->name);
                    exit(SYS_CALL_FAILED);
                }
                else
                    close(fd);
            }
            callOption(option, value);
            if((option->flags & FORK_ON_RECEIVE)) {
                fflush(NULL);
                if(dup2(stdout, STDOUT_FILENO) == -1) {
                    perror("Could not revert back stdout");
                }
                close(stdout);
            }
            if(!(option->flags & CONFIRM_EARLY))
                sendConfirmation(win, returnValue);
        }
        else {
            if(option)
                INFO("option '%s' is unsafe and unsafe options are not allowed", name);
            else
                WARN("could not find option matching '%s' '%s'", name, value);
            sendConfirmation(win, INVALID_OPTION);
        }
    }
}
