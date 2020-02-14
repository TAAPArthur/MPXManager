#include <assert.h>
#include <cstdlib>
#include <fcntl.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

#include "bindings.h"
#include "communications.h"
#include "globals.h"
#include "debug.h"
#include "logger.h"
#include "system.h"
#include "window-properties.h"
#include "windows.h"
#include "wmfunctions.h"
#include "xsession.h"

std::ostream& operator<<(std::ostream& strm, const Option& option) {
    return strm << option.name << " (" << (option.boundFunction.isVoid() ? "void" : option.boundFunction.isNumeric() ?
            "numeric" : "string") <<
        " Flags: " << option.flags << ")";
}

int isInt(std::string str, bool* isNumeric) {
    char* end;
    int num = strtol(str.c_str(), &end, 10);
    if(*end) {
        num = strtol(str.c_str(), &end, 16);
    }
    if(isNumeric)
        *isNumeric = (*end == 0) && !str.empty();
    return num;
}

Option::Option(std::string n, BoundFunction boundFunction, int flags): boundFunction(boundFunction), flags(flags) {
    std::transform(n.begin(), n.end(), n.begin(), ::tolower);
    std::replace(n.begin(), n.end(), '_', '-');
    name = n;
}

bool Option::matches(std::string str, bool empty, bool isNumeric) const {
    if(strcasecmp(name.c_str(), str.c_str()) == 0) {
        return (boundFunction.isVoid() == empty && boundFunction.isNumeric() == isNumeric);
    }
    return 0;
}
int Option::call(std::string p)const {
    DEBUG("Calling " << *this << " with " << p);
    uint32_t i = isInt(p, NULL);
    BoundFunctionArg arg = {
        .winInfo = getWindowInfo(i),
        .master = getMasterByID(i),
        .integer = i, .string = &p
    };
    return boundFunction(arg);
}
#define _OPTION_GETTER(VAR) Option("get-"#VAR,[]{std::cout<<VAR<<std::endl;})
#define _OPTION(VAR) Option(#VAR,[](uint32_t value){VAR=value;}, VAR_SETTER), _OPTION_GETTER(VAR)
#define _OPTION_STR(VAR) Option(#VAR,[](std::string* value){VAR=*value;}, VAR_SETTER), _OPTION_GETTER(VAR)

static UniqueArrayList<Option*> options = {
    {"activate", activateWindow},
    {"dump", +[]() {dumpWindow(MAPPABLE_MASK);}, FORK_ON_RECEIVE},
    {"dump", +[](WindowMask i) {dumpWindow(i);}, FORK_ON_RECEIVE},
    {"dump", +[](std::string * s) {dumpWindow(*s);}, FORK_ON_RECEIVE},
    {"dump-master", dumpMaster, FORK_ON_RECEIVE},
    {"dump-master", +[]() {dumpMaster(getActiveMaster());}, FORK_ON_RECEIVE},
    {"dump-options", +[](){std::cout << options << "\n";}, FORK_ON_RECEIVE},
    {"dump-rules", dumpRules, FORK_ON_RECEIVE},
    {"dump-stack", dumpWindowStack, FORK_ON_RECEIVE},
    {"dump-win", dumpSingleWindow, FORK_ON_RECEIVE},
    {"focus", +[](WindowID id) {focusWindow(id);}},
    {"focus-root", +[]() {focusWindow(root);}},
    {"list-options", +[](){std::cout >> options << "\n";}, FORK_ON_RECEIVE},
    {"load", loadWindowProperties},
    {"log-level", +[](int i){setLogLevel((LogLevel)i);}, VAR_SETTER},
    {"lower", +[](WindowID id) {lowerWindow(id);}},
    {"quit", +[]() {quit(0);}, CONFIRM_EARLY},
    {"raise", +[](WindowID id) {raiseWindow(id);}},
    {"restart", restart, CONFIRM_EARLY},
    {"request-shutdown", requestShutdown},
    {"sum", printSummary, FORK_ON_RECEIVE},
    {"switch-workspace", switchToWorkspace},
    _OPTION(AUTO_FOCUS_NEW_WINDOW_TIMEOUT),
    _OPTION(CLONE_REFRESH_RATE),
    _OPTION(CRASH_ON_ERRORS),
    _OPTION(DEFAULT_BORDER_COLOR),
    _OPTION(DEFAULT_BORDER_WIDTH),
    _OPTION(DEFAULT_MOD_MASK),
    _OPTION(DEFAULT_NUMBER_OF_WORKSPACES),
    _OPTION(DEFAULT_UNFOCUS_BORDER_COLOR),
    _OPTION(EVENT_PERIOD),
    _OPTION(IGNORE_MASK),
    _OPTION(KILL_TIMEOUT),
    _OPTION(LD_PRELOAD_INJECTION),
    _OPTION(POLL_COUNT),
    _OPTION(POLL_INTERVAL),
    _OPTION(RUN_AS_WM),
    _OPTION(STEAL_WM_SELECTION),
    _OPTION_STR(MASTER_INFO_PATH),
    _OPTION_STR(SHELL),
};

ArrayList<Option*>& getOptions() {return options;}

static int outstandingSendCount;
uint32_t getNumberOfMessageSent() {
    return outstandingSendCount;
}
int getConfirmedSentMessage(WindowID win) {
    auto result = getWindowPropertyValue(win, MPX_WM_INTERPROCESS_COM, XCB_ATOM_CARDINAL);
    LOG(LOG_LEVEL_TRACE, "Found %d out of %d confirmations", result, getNumberOfMessageSent());
    return result;
}
bool hasOutStandingMessages(void) {
    return getNumberOfMessageSent() > getConfirmedSentMessage(getPrivateWindow());
}
int getLastMessageExitCode(void) {
    return getWindowPropertyValue(getPrivateWindow(), MPX_WM_INTERPROCESS_COM, XCB_ATOM_CARDINAL);
}
static void sendConfirmation(WindowID target, int exitCode) {
    LOG(LOG_LEVEL_DEBUG, "sending receive confirmation to %d", target);
    // not atomic, but there should be only one thread/process modifying this value
    int count = getConfirmedSentMessage(target) + 1;
    setWindowProperty(target, MPX_WM_INTERPROCESS_COM_STATUS, XCB_ATOM_CARDINAL, exitCode);
    setWindowProperty(target, MPX_WM_INTERPROCESS_COM, XCB_ATOM_CARDINAL, count);
    flush();
}
void send(std::string name, std::string value) {
    unsigned int data[5] = {getAtom(name), getAtom(value), (uint32_t)getpid(), getPrivateWindow()};
    LOG(LOG_LEVEL_DEBUG, "sending %d (%s) %d (%s)", data[0], name.c_str(), data[1], value.c_str());
    catchError(xcb_ewmh_send_client_message(dis, root, root, MPX_WM_INTERPROCESS_COM, sizeof(data), data));
    outstandingSendCount++;
}
const Option* findOption(std::string name, std::string value) {
    bool isNumeric;
    bool empty = value.empty();
    isInt(value, &isNumeric);
    for(const Option* option : getOptions()) {
        if(option->matches(name, empty, isNumeric))
            return option;
    }
    LOG(LOG_LEVEL_WARN, "could not find option matching '%s' '%s' Empty %d numeric %d", name.c_str(), value.c_str(),
        empty, isNumeric);
    return NULL;
}

void receiveClientMessage(void) {
    xcb_client_message_event_t* event = (xcb_client_message_event_t*) getLastEvent();
    xcb_client_message_data_t data = event->data;
    xcb_atom_t message = event->type;
    xcb_atom_t optionAtom = data.data32[0];
    xcb_atom_t valueAtom = data.data32[1];
    pid_t callerPID = data.data32[2];
    WindowID sender = data.data32[3];
    if(message == MPX_WM_INTERPROCESS_COM && event->window == root && optionAtom) {
        DEBUG(getAtomsAsString(data.data32, 5));
        std::string name = getAtomName(optionAtom);
        std::string value = "";
        if(valueAtom)
            value = getAtomName(valueAtom);
        const Option* option = findOption(name, value);
        if(option) {
            DEBUG(*option);
            int childPID;
            int returnValue = 0;
            if(option->flags & CONFIRM_EARLY) {
                TRACE("sending early confirmation");
                sendConfirmation(sender, 0);
                flush();
            }
            if(!(option->flags & FORK_ON_RECEIVE)) {
                returnValue = option->call(value);
            }
            // TODO stop forking
            else if(!(childPID = fork())) {
                std::cout << std::flush;
                char outputFile[64] = {0};
                if(callerPID) {
                    const char* formatter = "/proc/%d/fd/1";
                    sprintf(outputFile, formatter, callerPID);
                    int fd = open(outputFile, O_WRONLY | O_APPEND);
                    if(fd == -1 || dup2(fd, STDOUT_FILENO) == -1) {
                        LOG(LOG_LEVEL_WARN, "could not open %s for writing; could not call/set %s aborting", outputFile,
                            option->name.c_str());
                        exit(0);
                    }
                    close(fd);
                }
                openXDisplay();
                returnValue = option->call(value);
                std::cout << std::flush;
                close(STDOUT_FILENO);
                exit(returnValue);
            }
            else {
                returnValue = waitForChild(childPID);
            }
            if(!(option->flags & CONFIRM_EARLY))
                sendConfirmation(sender, returnValue);
        }
        else
            LOG(LOG_LEVEL_WARN, "could not find option matching '%s' '%s'", name.c_str(), value.c_str());
    }
}
void enableInterClientCommunication(void) {
    getEventRules(XCB_CLIENT_MESSAGE).add(DEFAULT_EVENT(receiveClientMessage));
}
