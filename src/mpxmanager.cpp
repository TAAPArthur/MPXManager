/**
 * @file mpxmanager.cpp
 * Entry point for MPXManager
 */
#include <assert.h>
#include <getopt.h>
#include <string.h>


#include "Extensions/mpx.h"
#include "Extensions/xmousecontrol.h"
#include "communications.h"
#include "wm-rules.h"
#include "ewmh.h"
#include "extra-rules.h"
#include "globals.h"
#include "logger.h"
#include "settings.h"
#include "system.h"
#include "time.h"
#include "wmfunctions.h"
#include "xsession.h"

#ifdef DOXYGEN
#undef assert
#define assert(x)
#endif

/// if true, the event loop will be started else we wait for send confirmations and exit
static bool RUN_EVENT_LOOP = 1;

/**
 * Clear properties so we won't conflict with running WM
 */
static void clearWMSettings(void) {
    ROOT_EVENT_MASKS = ROOT_DEVICE_EVENT_MASKS = NON_ROOT_EVENT_MASKS = NON_ROOT_DEVICE_EVENT_MASKS = 0;
    startupMethod = NULL;
    RUN_AS_WM = 0;
}
struct Mode {
    std::string name;
    void(*func)();
    bool runEventLoop = 1;
};
/// all enabled modes
ArrayList<Mode>modes = {
#if XMOUSE_CONTROL_EXT_ENABLED
    {"xmousecontrol", addStartXMouseControlRule},
#endif
#if MPX_EXT_ENABLED
    {"mpx", +[]() {addAutoMPXRules(); getEventRules(X_CONNECTION).add(startMPX);}},
    {"mpx-start", +[](){getEventRules(X_CONNECTION).add(startMPX);}, 0},
    {"mpx-stop", +[](){getEventRules(X_CONNECTION).add(stopMPX);}, 0},
    {"mpx-restart", +[](){getEventRules(X_CONNECTION).add(restartMPX);}, 0},
    {"mpx-restore", +[](){getEventRules(X_CONNECTION).add(restoreMPX);}, 0},

#endif
};
/// list all defined modes
static void listModes() {
    for(const Mode& mode : modes)
        std::cout << mode.name << " ";
    std::cout << "\n";
}
/**
 * Calls the mode with name str.
 * It is an error if str is not found
 *
 * @param str the name of a mode
 */
static void setMode(std::string str) {
    const char* c = str.c_str();
    clearWMSettings();
    for(const Mode& mode : modes) {
        if(strcasecmp(c, mode.name.c_str()) == 0) {
            mode.func();
            RUN_EVENT_LOOP = mode.runEventLoop;
            return;
            break;
        }
    }
    LOG(LOG_LEVEL_ERROR, "could not find mode '%s'\n", c);
    quit(1);
}
/// whether the send (0) or set(1) was last called
static bool lastLocal = 0;
/**
 * Finds the option matching str and either calls in locally or sends it to a running instance
 *
 * @param str
 * @param local
 */
static void _handleOption(std::string str, bool local) {
    int index = str.find('=');
    std::string name = str.substr(0, index);
    std::string value = index == -1 ? "" : str.substr(index + 1);
    const Option* option = findOption(name, value);
    if(option)
        if(local)
            option->call(value);
        else {
            send(option->name, value);
        }
    else {
        LOG(LOG_LEVEL_ERROR, "cannot find option %s with value '%s'\n", name.c_str(), value.c_str());
        quit(1);
    }
    lastLocal = local;
}
/**
 * Calls the option specified by str
 *
 * @param str of the form: name[=value]
 */
static void setOption(std::string str) {
    _handleOption(str, 1);
}
/**
 * sends str to a running instance
 *
 * @param str of the form: name[=value]
 */
static void sendOption(std::string str) {
    if(!dpy) {
        setLogLevel(LOG_LEVEL_WARN);
        openXDisplay();
        clearWMSettings();
        RUN_EVENT_LOOP = 0;
    }
    _handleOption(str, 0);
}
/// sets the startup method to NULL
/// used generally when this instance is short lived
static void clearStartupMethod(void) {
    startupMethod = NULL;
}

/// list of startup options
static UniqueArrayList<Option*> options = {
    {"mode", setMode},
    {"clear-startup-method", clearStartupMethod},
    {"replace", +[]() {STEAL_WM_SELECTION = 1;}},
    {"no-event-loop", +[]() {RUN_EVENT_LOOP = 0;}},
    {"set", setOption},
    {"send", sendOption},
    {"enable-inter-client-communication", enableInterClientCommunication},
    {"list-modes", +[]() {listModes(); exit(0);}},
    {"list-start-options", +[]() {std::cout >> options; exit(0);}},
};

/**
 * Parse command line arguments starting the 1st index
 * @param argc the number of args to parse
 * @param argv the list of args. An element by be like "-s", "--long", --long=V", etc
 * @param exitOnUnknownOptions if true will exit(1) if encounter an unknown option
 */
static void parseArgs(int argc, char* const* argv, int exitOnUnknownOptions) {
    for(int i = 1; i < argc; i++) {
        bool foundMatch = 0;
        LOG(LOG_LEVEL_TRACE, "processing %s\n", argv[i]);
        std::string str = argv[i];
        for(const Option* option : options) {
            if(str == ("--" + option->name))
                if(option->isVoid())
                    option->call("");
                else
                    option->call(argv[++i]);
            else if(str.find("--" + option->name + "=") == 0) {
                int index = str.find('=');
                std::string value = str.substr(index + 1);
                option->call(value);
            }
            else continue;
            foundMatch = 1;
            break;
        }
        if(!foundMatch && exitOnUnknownOptions) {
            LOG(LOG_LEVEL_DEBUG, "Trying to send/set %s.\n", argv[i]);
            const char* c = argv[i];
            if(c[0] == '-' && c[1] == '-')
                c += 2;
            if(lastLocal)
                setOption(c);
            else
                sendOption(c);
        }
    }
}
int _main(int argc, char* const* argv) {
    LOG(LOG_LEVEL_DEBUG, "MPXManager started with %d args\n", argc);
    numPassedArguments = argc;
    passedArguments = argv;
    startupMethod = loadSettings;
    parseArgs(argc, argv, 1);
    if(!dpy)
        onStartup();
    if(RUN_EVENT_LOOP)
        runEventLoop();
    else {
        if(hasOutStandingMessages()) {
            LOG(LOG_LEVEL_TRACE, "waiting for send receipts\n");
            bool wasMPXManagerRunning = isMPXManagerRunning();
            if(!wasMPXManagerRunning)
                for(int i = 0; i < 10 && hasOutStandingMessages(); i++)
                    msleep(100);
            while(isMPXManagerRunning() && hasOutStandingMessages())
                msleep(100);
            if(hasOutStandingMessages()) {
                LOG(LOG_LEVEL_ERROR, "did not receive confirmation\n");
                quit(2);
            }
            LOG(LOG_LEVEL_DEBUG, "WM Running: %d; Outstanding messages: %d\n", isMPXManagerRunning(), hasOutStandingMessages());
        }
    }
    return 0;
}
#ifndef MPX_TEST_NO_MAIN
/**
 * @param argc
 * @param argv[]
 *
 * @return
 */
int __attribute__((weak)) main(int argc, char* const argv[]) {
    _main(argc, argv);
    quit(0);
}
#endif
