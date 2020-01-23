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
#include "logger.h"
#include "system.h"
#include "window-properties.h"
#include "windows.h"
#include "wmfunctions.h"
#include "xsession.h"

std::ostream& operator<<(std::ostream& strm, const Option& option) {
    strm << option.name;
    if(!option.isVoid())
        strm << " = ";
    return option.func->toString(strm) << " (Flags: " << option.flags << ")";
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

bool Option::matches(std::string str, bool empty, bool isNumeric) const {
    if(func->isVoid() == empty && func->isNumeric() == isNumeric)
        return strcasecmp(name.c_str(), str.c_str()) == 0;
    return 0;
}
void Option::operator()(std::string value)const {
    LOG(LOG_LEVEL_DEBUG, "calling option %s(%s)\n", name.c_str(), value.c_str());
    func->call(value);
    if(flags & QUIT_AFTER)
        quit(0);
}
#define _OPTION(VAR) Option(std::string(#VAR),&VAR)

static UniqueArrayList<Option*> options = {
    {"activate", +[](WindowID win) {if(getWindowInfo(win))activateWindow(getWindowInfo(win));}},
    {"dump", +[]() {dumpWindow(MAPPABLE_MASK);}, FORK_ON_RECEIVE},
    {"dump", +[](WindowMask i) {dumpWindow(i);}, FORK_ON_RECEIVE},
    {"dump", +[](std::string s) {assert(s != ""); std::cout << s << "\n"; dumpWindow(s);}, FORK_ON_RECEIVE},
    {"dump-master", +[]() {dumpMaster(getActiveMaster());}, FORK_ON_RECEIVE},
    {"dump-master", +[](MasterID id) {dumpMaster(getMasterByID(id));}, FORK_ON_RECEIVE},
    {"dump-options", +[](){std::cout << options << "\n";}, FORK_ON_RECEIVE},
    {"dump-rules", dumpRules, FORK_ON_RECEIVE},
    {"dump-stack", dumpWindowStack, FORK_ON_RECEIVE},
    {"dump-win", +[](WindowID win) {dumpSingleWindow(win);}, FORK_ON_RECEIVE},
    {"focus", +[](WindowID id) {focusWindow(id);}},
    {"focus-root", +[]() {focusWindow(root);}},
    {"list-options", +[](){std::cout >> options << "\n";}, FORK_ON_RECEIVE},
    {"load", +[](WindowID win) {WindowInfo* winInfo = getWindowInfo(win); if(winInfo)loadWindowProperties(winInfo);}},
    {"log-level", setLogLevel},
    {"lower", +[](WindowID id) {lowerWindow(id);}},
    {"quit", +[]() {quit(0);}, CONFIRM_EARLY},
    {"raise", +[](WindowID id) {raiseWindow(id);}},
    {"restart", restart, CONFIRM_EARLY},
    {"sum", printSummary, FORK_ON_RECEIVE},
    {"switch-workspace", +[](WorkspaceID id) {switchToWorkspace(id);}},
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
    _OPTION(MASTER_INFO_PATH),
    _OPTION(POLL_COUNT),
    _OPTION(POLL_INTERVAL),
    _OPTION(RUN_AS_WM),
    _OPTION(SHELL),
    _OPTION(STEAL_WM_SELECTION),
};

ArrayList<Option*>& getOptions() {return options;}



static int outstandingSendCount;
int getConfirmedSentMessage(WindowID win) {
    xcb_get_property_cookie_t cookie;
    xcb_get_property_reply_t* reply;
    cookie = xcb_get_property(dis, 0, win, WM_INTERPROCESS_COM, XCB_ATOM_CARDINAL, 0, sizeof(int));
    int result = 0;
    if((reply = xcb_get_property_reply(dis, cookie, NULL)) && xcb_get_property_value_length(reply)) {
        result = *(int*)xcb_get_property_value(reply);
    }
    free(reply);
    LOG(LOG_LEVEL_TRACE, "Found %d out of %d confirmations\n", result, outstandingSendCount);
    return result;
}
bool hasOutStandingMessages(void) {
    return outstandingSendCount > getConfirmedSentMessage(getPrivateWindow());
}
static void sendConfirmation(WindowID target) {
    LOG(LOG_LEVEL_DEBUG, "sending receive confirmation to %d\n", target);
    int count = getConfirmedSentMessage(target) + 1;
    catchErrorSilent(xcb_change_property_checked(dis, XCB_PROP_MODE_REPLACE, target, WM_INTERPROCESS_COM, XCB_ATOM_CARDINAL,
            32,
            1, &count));
    flush();
}
void send(std::string name, std::string value) {
    unsigned int data[5] = {getAtom(name), getAtom(value), (uint32_t)getpid(), getPrivateWindow()};
    LOG(LOG_LEVEL_DEBUG, "sending %d (%s) %d (%s)\n", data[0], name.c_str(), data[1], value.c_str());
    catchError(xcb_ewmh_send_client_message(dis, root, root, WM_INTERPROCESS_COM, sizeof(data), data));
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
    LOG(LOG_LEVEL_WARN, "could not find option matching '%s' '%s' Empty %d numeric %d\n", name.c_str(), value.c_str(),
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
    if(message == WM_INTERPROCESS_COM && event->window == root && optionAtom) {
        LOG_RUN(LOG_LEVEL_DEBUG, dumpAtoms(data.data32, 5));
        std::string name = getAtomName(optionAtom);
        std::string value = "";
        if(valueAtom)
            value = getAtomName(valueAtom);
        const Option* option = findOption(name, value);
        if(option) {
            logger.debug() << *option << std::endl;
            int childPID;
            if(option->flags & CONFIRM_EARLY) {
                LOG(LOG_LEVEL_TRACE, "sending early confirmation\n");
                sendConfirmation(sender);
                flush();
            }
            if(!(option->flags & FORK_ON_RECEIVE)) {
                option->call(value);
            }
            // TODO stop forking
            else if(!(childPID = fork())) {
                std::cout << std::flush;
                char outputFile[64] = {0};
                if(callerPID) {
                    const char* formatter = "/proc/%d/fd/1";
                    sprintf(outputFile, formatter, callerPID);
                    int fd = open(outputFile, O_WRONLY | O_APPEND);
                    if(fd == -1 || dup2(fd, LOG_FD) == -1) {
                        LOG(LOG_LEVEL_WARN, "could not open %s for writing; could not call/set %s aborting\n", outputFile,
                            option->name.c_str());
                        exit(0);
                    }
                    close(fd);
                }
                openXDisplay();
                option->call(value);
                std::cout << std::flush;
                close(LOG_FD);
                exit(0);
            }
            else {
                waitForChild(childPID);
            }
            if(!(option->flags & CONFIRM_EARLY))
                sendConfirmation(sender);
        }
        else
            LOG(LOG_LEVEL_WARN, "could not find option matching '%s' '%s'\n", name.c_str(), value.c_str());
    }
}
void enableInterClientCommunication(void) {
    getEventRules(XCB_CLIENT_MESSAGE).add(DEFAULT_EVENT(receiveClientMessage));
}
