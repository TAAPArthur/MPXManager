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
#include "default-rules.h"
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

static bool RUN_EVENT_LOOP = 1;

static void clearWMSettings(void) {
    ROOT_EVENT_MASKS = ROOT_DEVICE_EVENT_MASKS = NON_ROOT_EVENT_MASKS = NON_ROOT_DEVICE_EVENT_MASKS = 0;
    startupMethod = NULL;
    RUN_AS_WM = 0;
}
static void setMode(std::string str) {
    const char* c = str.c_str();
    clearWMSettings();
    if(!c || c[0] == 0 || strcasecmp(c, " "))
        return;
#if XMOUSE_CONTROL_EXT_ENABLED
    else if(strcasecmp(c, "xmousecontrol") == 0) {
        startupMethod = addStartXMouseControlRule;
    }
#endif
#if MPX_EXT_ENABLED
    else if(strcasecmp(c, "mpx") == 0) {
        addAutoMPXRules();
        getEventRules(onXConnection).add(startMPX);
    }
    else if(strcasecmp(c, "mpx-start") == 0) {
        getEventRules(onXConnection).add(startMPX);
        RUN_EVENT_LOOP = 0;
    }
    else if(strcasecmp(c, "mpx-stop") == 0) {
        getEventRules(onXConnection).add(stopMPX);
        RUN_EVENT_LOOP = 0;
    }
    else if(strcasecmp(c, "mpx-restart") == 0) {
        getEventRules(onXConnection).add(restartMPX);
        RUN_EVENT_LOOP = 0;
    }
    else if(strcasecmp(c, "mpx-split") == 0) {
        addAutoMPXRules();
        getEventRules(onXConnection).add(splitMaster);
        addDieOnIdleRule();
    }
    else if(strcasecmp(c, "mpx-restore") == 0) {
        getEventRules(onXConnection).add(restoreMPX);
        RUN_EVENT_LOOP = 0;
    }
#endif
    else {
        LOG(LOG_LEVEL_ERROR, "could not find mode %s\n", c);
        exit(1);
    }
}
static int sendOptionToWaitFor = 0;
static void _handleOption(std::string str, bool local) {
    int index = str.find('=');
    std::string name = str.substr(0, index);
    std::string value = index == str.npos ? "" : str.substr(index + 1);
    Option* option = findOption(name, value);
    if(option)
        if(local)
            option->call(value);
        else {
            sendOptionToWaitFor += option->flags & FORK_ON_RECEIVE ? 1 : 0;
            send(option->name, value);
        }
    else {
        LOG(LOG_LEVEL_ERROR, "cannot find option %s with value '%s'\n", name.c_str(), value.c_str());
        quit(1);
    }
}
static void setOption(std::string str) {
    _handleOption(str, 1);
}
static void sendOption(std::string str) {
    if(!dpy) {
        openXDisplay();
        clearWMSettings();
        RUN_EVENT_LOOP = 0;
    }
    _handleOption(str, 0);
}
static void clearStartupMethod(void) {
    startupMethod = NULL;
}

static Option options[] = {
    {"mode", setMode},
    {"clear-startup-method", clearStartupMethod},
    {"replace", +[]() {setOption("STEAL_WM_SELECTION=1");}},
    {"set", setOption},
    {"send", sendOption},
    {"enable-inter-client-communication", enableInterClientCommunication},
    {"list-options", +[]() {std::cout << options << "\n"; quit(0);}},
};

/**
 * Parse command line arguments starting the 1st index
 * @param argc the number of args to parse
 * @param argv the list of args. An element by be like "-s", "--long", --long=V", etc
 * @param exitOnUnknownOptions if true will exit(1) if encounter an unknown option
 */
static void parseArgs(int argc, char* argv[], int exitOnUnknownOptions) {
    for(int i = 1; i < argc; i++) {
        bool foundMatch = 0;
        LOG(LOG_LEVEL_TRACE, "processing %s\n", argv[i]);
        std::string str = argv[i];
        for(Option& option : options) {
            if(str == ("--" + option.name))
                if(option.getType() == FUNC_VOID)
                    option.call("");
                else
                    option.call(argv[++i]);
            else if(str.find("--" + option.name + "=") == 0) {
                int index = str.find('=');
                std::string value = str.substr(index + 1);
                option.call(value);
            }
            else continue;
            foundMatch = 1;
            break;
        }
        if(!foundMatch && exitOnUnknownOptions) {
            LOG(LOG_LEVEL_ERROR, "could not process %s\n", argv[i]);
            quit(1);
        }
    }
}
/**
 * @param argc
 * @param argv[]
 *
 * @return
 */
int main(int argc, char* argv[]) {
    LOG(LOG_LEVEL_TRACE, "MPXManager started with %d args\n", argc);
    numPassedArguments = argc;
    passedArguments = argv;
    startupMethod = loadSettings;
    parseArgs(argc, argv, 1);
    if(!dpy)
        onStartup();
    if(RUN_EVENT_LOOP)
        runEventLoop(NULL);
    else  {
        if(hasOutStandingMessages()) {
            LOG(LOG_LEVEL_TRACE, "waiting for send receipts\n");
            bool wasMPXManagerRunning = isMPXManagerRunning();
            if(!wasMPXManagerRunning)
                for(int i = 0; i < 10 && hasOutStandingMessages(); i++)
                    msleep(100);
            while(isMPXManagerRunning() && hasOutStandingMessages())
                msleep(100);
            if(hasOutStandingMessages())
                quit(2);
            LOG(LOG_LEVEL_TRACE, "waiting for send receipts %d %d\n", isMPXManagerRunning(), hasOutStandingMessages());
        }
    }
    quit(0);
    return 0;
}
