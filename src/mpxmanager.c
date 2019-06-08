/**
 * @file mpxmanager.c
 * Entry point for MPXManager
 */
#include <assert.h>
#include <getopt.h>
#include <string.h>


#include "wmfunctions.h"
#include "communications.h"
#include "default-rules.h"
#include "events.h"
#include "globals.h"
#include "logger.h"
#include "mywm-util.h"
#include "spawn.h"
#include "settings.h"
#include "Extensions/xmousecontrol.h"
#include "Extensions/mpx.h"
#include "xsession.h"

#ifdef DOXYGEN
    #undef assert
    #define assert(x)
#endif

static bool RUN_EVENT_LOOP = 1;

static void clearWMSettings(void){
    ROOT_EVENT_MASKS = ROOT_DEVICE_EVENT_MASKS = NON_ROOT_EVENT_MASKS = NON_ROOT_DEVICE_EVENT_MASKS = 0;
    startUpMethod = NULL;
    RUN_AS_WM = 0;
}
static void setMode(char* c){
    clearWMSettings();
    if(!c || c[0] == 0 || strcasecmp(c, " "))
        return;
#if XMOUSE_CONTROL_EXT_ENABLED
    else if(strcasecmp(c, "xmousecontrol") == 0){
        startUpMethod = enableXMouseControl;
    }
#endif
#if MPX_EXT_ENABLED
    else if(strcasecmp(c, "mpx") == 0){
        addAutoMPXRules();
        addSomeBasicRules((int[]){XCB_INPUT_HIERARCHY + GENERIC_EVENT_OFFSET}, 1);
        postStartUpMethod = startMPX;
    }
    else if(strcasecmp(c, "mpx-start") == 0){
        postStartUpMethod = startMPX;
        RUN_EVENT_LOOP = 0;
    }
    else if(strcasecmp(c, "mpx-stop") == 0){
        postStartUpMethod = stopMPX;
        RUN_EVENT_LOOP = 0;
    }
    else if(strcasecmp(c, "mpx-restart") == 0){
        postStartUpMethod = restartMPX;
        RUN_EVENT_LOOP = 0;
    }
    else if(strcasecmp(c, "mpx-split") == 0){
        addAutoMPXRules();
        postStartUpMethod = (void(*)())splitMaster;
        addDieOnIdleRule();
    }
    else if(strcasecmp(c, "mpx-restore") == 0){
        postStartUpMethod = restoreMPX;
        RUN_EVENT_LOOP = 0;
    }
#endif
    else {
        LOG(LOG_LEVEL_ERROR, "could not find mode %s\n", c);
        exit(1);
    }
}
static int setOption(char* c){
    char* value = strchr(c, '=');
    int len = value ? value - c : strlen(c);
    Option* option = findOption(c, len);
    if(option)
        callOption(option, value ? value + 1 : NULL);
    else {
        LOG(LOG_LEVEL_ERROR, "cannot find option %.*s\n", len, c);
        exit(1);
    }
    return option ? 1 : 0;
}
static int sendOptionToWaitFor = 0;
static int sendOption(char* c){
    if(!dpy){
        openXDisplay();
        clearWMSettings();
        RUN_EVENT_LOOP = 0;
    }
    char* value = strchr(c, '=');
    int len = value ? value - c : strlen(c);
    Option* option = findOption(c, len);
    if(option){
        sendOptionToWaitFor += option->forkOnReceive;
        send(option->name, value ? value + 1 : NULL);
    }
    else {
        LOG(LOG_LEVEL_ERROR, "cannot find option to send %.*s\n", len, value ? value : c);
        exit(1);
    }
    return option ? 1 : 0;
}
static void clearStartupMethod(void){
    startUpMethod = NULL;
}
static Option options[] = {
    {"mode", {.boundFunction = BIND(setMode)}, FUNC_STR, .shortName = 'm'},
    {"clear-startup-method", {.boundFunction = BIND(clearStartupMethod)}, FUNC},
    {"enable-inter-client-communication", {.boundFunction = BIND(enableInterClientCommunication)}, FUNC},
    {"replace", {.boundFunction = BIND(setOption, "STEAL_WM_SELECTION=1")}, FUNC},
    {"set", {.boundFunction = BIND(setOption)}, FUNC_STR},
    {"send", {.boundFunction = BIND(sendOption)}, FUNC_STR, .shortName = 's'},
    {"list-options", {.boundFunction = AND(BIND(setOption, "list-options"), BIND(exit, 0))}, FUNC},
};
static void listOptions(void){
    for(int i = 0; i < LEN(options); i++)
        printOption(&options[i]);
}
static const int NUM_OPTIONS = LEN(options);
static char short_opt[LEN(options) + 1] = {0};
static struct option long_opt[LEN(options) + 1];
static int init = 0;

static int getOptionIndexByShortName(char name){
    for(int i = 0; i < NUM_OPTIONS; i++)
        if(options[i].shortName == name)
            return i;
    return -1;
}

static inline void initParsing(){
    optind = 1;
    if(init)return;
    init = 1;
    for(int i = 0, counter = 0; i < NUM_OPTIONS; i++){
        long_opt[i] = (struct option){
            options[i].name, options[i].type == FUNC ? no_argument : required_argument, NULL, options[i].shortName
        };
        if(options[i].shortName){
            short_opt[counter++] = options[i].shortName;
            if(options[i].type != FUNC)
                short_opt[counter++] = ':';
        }
    }
}
/**
 * Parse command line arguments starting the 1st index
 * @param argc the number of args to parse
 * @param argv the list of args. An element by be like "-s", "--long", --long=V", etc
 * @param exitOnUnknownOptions if true will exit(1) if encounter an unknown option
 */
static void parseArgs(int argc, char* argv[], int exitOnUnknownOptions){
    initParsing();
    int c;
    int index;
    while((c = getopt_long(argc, argv, short_opt, long_opt, &index)) != -1){
        switch(c){
            case -1:       /* no more arguments */
                break;
            case '?':
                LOG(LOG_LEVEL_WARN, "unknown option %c\n", c);
                if(exitOnUnknownOptions){
                    listOptions();
                    exit(1);
                }
                break;
            default:
                index = getOptionIndexByShortName(c);
                if(index == -1){
                    LOG(LOG_LEVEL_WARN, "unknown option %c\n", c);
                    listOptions();
                    break;
                }
                __attribute__((fallthrough));
            case 0:        /* long options toggles */
                LOG(LOG_LEVEL_VERBOSE, "found option %d value %s\n", index, optarg);
                callOption(&options[index], optarg);
                break;
        }
    }
}
/**
 * @param argc
 * @param argv[]
 *
 * @return
 */
int main(int argc, char* argv[]){
    numPassedArguments = argc;
    passedArguments = argv;
    loadDefaultOptions();
    startUpMethod = loadSettings;
    parseArgs(argc, argv, 1);
    if(!dpy)
        onStartup();
    if(RUN_EVENT_LOOP)
        runEventLoop(NULL);
    else if(ENABLE_FORK_ON_RECEIVE){
        if(hasOutStandingMessages()){
            while(isMPXManagerRunning() && hasOutStandingMessages())
                msleep(100);
        }
    }
    quit(0);
    return 0;
}
