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

/// list options that set global variables
static void listVarOptions() {
    for(const Option* option : getOptions())
        if(option->getFlags() & VAR_SETTER)
            std::cout << "--" << option->getName() << std::endl;
}
/// list of startup options
static UniqueArrayList<Option*> options = {
    {"enable-inter-client-communication", addInterClientCommunicationRule},
    {"list-start-options", +[]() {std::cout >> options << "\n"; exit(0);}},
    {"list-options", +[]() {std::cout >> getOptions() << "\n"; exit(0);}},
    {"list-vars", +[]() {listVarOptions(); exit(0);}},
    {"no-event-loop", +[]() {RUN_EVENT_LOOP = 0;}},
    {"no-run-as-window-manager", clearWMSettings},
    {"replace", +[]() {STEAL_WM_SELECTION = 1;}},
};

void addStartupMode(std::string name, void(*func)()) {
    options.add(new Option(name, func));
}

/**
 * @param list list of options
 * @param argv
 * @param i index of argv; may be modified
 * @param varsOnly if true will only consider options with the VAR_SETTER flag
 *
 * @return true, if an option was found and called
 */
static inline bool callStartupOption(const ArrayList<Option*>& list, const char* const* argv, int& i,
    bool varsOnly = 0) {
    std::string str = argv[i];
    for(const Option* option : list) {
        if(varsOnly && !(option->getFlags() & VAR_SETTER)) {
            continue;
        }
        std::string value = "";
        bool incrementI = 0;
        if(str == ("--" + option->name))
            if(option->getBoundFunction().isVoid())
                value = "";
            else if(argv[i] && argv[i + 1]) {
                value = argv[i + 1];
                incrementI = 1;
            }
            else
                continue;
        else if(str.find("--" + option->name + "=") == 0) {
            int index = str.find('=');
            value = str.substr(index + 1);
        }
        else continue;
        if(option->matches(value)) {
            option->call(value);
            if(incrementI)
                i++;
            return 1;
        }
    }
    return 0;
}
/**
 * Parse command line arguments starting the 1st index
 * @param argc the number of args to parse
 * @param argv the list of args. An element by be like "-s", "--long", --long=V", etc
 */
static void parseArgs(int argc, char* const* argv) {
    assert(argc > 1);
    if(std::string(argv[1]).find("--") == 0)
        for(int i = 1; i < argc; i++) {
            LOG(LOG_LEVEL_TRACE, "processing %s", argv[i]);
            if(!(callStartupOption(options, argv, i) || callStartupOption(getOptions(), argv, i, 1))) {
                LOG(LOG_LEVEL_ERROR, "Could not find matching options for %s.", argv[i]);
                exit(1);
            }
        }
    else {
        int i = 1;
        LOG(LOG_LEVEL_DEBUG, "Trying to send %s.", argv[i]);
        std::string value(argv[i + 1] ? argv[i + 1] : "");
        if(!findOption(argv[i], value)) {
            LOG(LOG_LEVEL_ERROR, "Could not find matching options for %s.", argv[i]);
            exit(1);
        }
        if(!hasXConnectionBeenOpened()) {
            openXDisplay();
            clearWMSettings();
            RUN_EVENT_LOOP = 0;
        }
        send(argv[i], value);
    }
}
int _main(int argc, char* const* argv) {
    LOG(LOG_LEVEL_DEBUG, "MPXManager started with %d args", argc);
    numPassedArguments = argc;
    passedArguments = argv;
    if(!startupMethod)
        startupMethod = loadSettings;
    if(argc > 1)
        parseArgs(argc, argv);
    if(!hasXConnectionBeenOpened())
        onStartup();
    if(RUN_EVENT_LOOP)
        runEventLoop();
    else if(getNumberOfMessageSent()) {
        if(hasOutStandingMessages()) {
            TRACE("waiting for send receipts");
            bool wasMPXManagerRunning = isMPXManagerRunning();
            if(!wasMPXManagerRunning)
                for(int i = 0; i < 10 && hasOutStandingMessages(); i++)
                    msleep(100);
            while(isMPXManagerRunning() && hasOutStandingMessages())
                msleep(100);
            if(hasOutStandingMessages()) {
                ERROR("did not receive confirmation");
                quit(2);
            }
            LOG(LOG_LEVEL_DEBUG, "WM Running: %d; Outstanding messages: %d", isMPXManagerRunning(), hasOutStandingMessages());
        }
        if(hasXConnectionBeenOpened())
            return !getLastMessageExitCode();
    }
    return 0;
}
/**
 * @param argc
 * @param argv[]
 *
 * @return
 */
int __attribute__((weak)) main(int argc, char* const argv[]) {
    setLogLevel(LOG_LEVEL_WARN);
    quit(_main(argc, argv));
}
#ifndef NDEBUG
int mainAlias(int argc, char* const argv[]) __attribute__((weak, alias("main")));
#endif
