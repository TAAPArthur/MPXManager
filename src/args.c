/**
 * @file args.c
 * \copybrief args.h
 */
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "args.h"
#include "bindings.h"
#include "events.h"
#include "globals.h"
#include "logger.h"
#include "mywm-util.h"
#include "windows.h"

/// determines how verbose logging is
extern int LOG_LEVEL;
#ifdef MPX_TESTING
    static const char* fifoPipe = "/tmp/mpxmanager-fifo-test";
#else
    static const char* fifoPipe = "/tmp/mpxmanager-fifo";
#endif

/// Specifes type of option
typedef enum {
    BOOL, INT, LONG, STRING, FUNC, FUNC_STR, FUNC_INT
} OptionTypes;
/// Holds info to map string to a given operation
typedef struct {
    /// option name; --$name will trigger option
    char* name;
    /// detail how we should respond to the option
    union {
        /// set the var
        void* var;
        /// call a function
        BoundFunction boundFunction;
    } var;
    /// the type of var.var
    OptionTypes type;
    /// character that may also be used to trigger binding
    char shortName;
} Option;

#define _OPTION(VAR,shortName...){#VAR,&VAR,\
   _Generic((VAR), \
        char:BOOL, \
        int:INT, \
        unsigned int:INT, \
        long:LONG, \
        char*:STRING \
),shortName}

static Option options[] = {
    {"list", {.boundFunction = BIND(listOptions)}, FUNC},
    {"restart", {.boundFunction = BIND(restart)}, FUNC},
    {"dump", {.boundFunction = BIND(dumpAllWindowInfo, MAPPABLE_MASK)}, FUNC},
    {"dump-filter", {.boundFunction = BIND(dumpMatch, NULL)}, FUNC_STR},
    {"dump-all", {.boundFunction = BIND(dumpAllWindowInfo, 0)}, FUNC},
    {"list", {.boundFunction = BIND(listOptions)}, FUNC},
    {"quit", {.boundFunction = BIND(exit, 0)}, FUNC},
    {"restart", {.boundFunction = BIND(restart)}, FUNC},
    {"sum", {.boundFunction = BIND(printSummary)}, FUNC},
    _OPTION(AUTO_FOCUS_NEW_WINDOW_TIMEOUT),
    _OPTION(CLONE_REFRESH_RATE),
    _OPTION(CRASH_ON_ERRORS, 'c'),
    _OPTION(DEFAULT_BORDER_COLOR),
    _OPTION(DEFAULT_BORDER_WIDTH),
    _OPTION(DEFAULT_FOCUS_BORDER_COLOR),
    _OPTION(DEFAULT_UNFOCUS_BORDER_COLOR),
    _OPTION(DEFAULT_WORKSPACE_INDEX),
    _OPTION(IGNORE_KEY_REPEAT),
    _OPTION(IGNORE_MASK),
    _OPTION(IGNORE_SEND_EVENT),
    _OPTION(IGNORE_SUBWINDOWS),
    _OPTION(KILL_TIMEOUT),
    _OPTION(LOAD_SAVED_STATE),
    _OPTION(LOG_LEVEL),
    _OPTION(MASTER_INFO_PATH),
    _OPTION(NUMBER_OF_WORKSPACES),
    _OPTION(POLL_COUNT),
    _OPTION(POLL_INTERVAL),
    _OPTION(SHELL),
};
static const int NUM_OPTIONS = LEN(options);
static char short_opt[LEN(options) + 1] = {0};
static struct option long_opt[LEN(options) + 1];
static int init = 0;

static void printValue(int optionIndex){
    switch(options[optionIndex].type){
        case STRING:
            LOG(LOG_LEVEL_INFO, "%s='%s'\n", options[optionIndex].name, *(char**)options[optionIndex].var.var);
            break;
        case BOOL:
            LOG(LOG_LEVEL_INFO, "%s=%d\n", options[optionIndex].name, *(char*)options[optionIndex].var.var);
            break;
        case INT:
            LOG(LOG_LEVEL_INFO, "%s=%d\n", options[optionIndex].name, *(int*)options[optionIndex].var.var);
            break;
        case LONG:
            LOG(LOG_LEVEL_INFO, "%s=%ld\n", options[optionIndex].name, *(long*)options[optionIndex].var.var);
            break;
        default:
            LOG(LOG_LEVEL_INFO, "%s\n", options[optionIndex].name);
    }
}
static void setValue(int optionIndex, char* value){
    if(!value)
        value = "1";
    if(options[optionIndex].type < FUNC)
        LOG(LOG_LEVEL_DEBUG, "setting %s to %s\n", options[optionIndex].name, value);
    switch(options[optionIndex].type){
        case STRING:
            *(char**)options[optionIndex].var.var = value;
            break;
        case BOOL:
            *(char*)options[optionIndex].var.var = atoi(value);
            break;
        case INT:
            *(int*)options[optionIndex].var.var = atoi(value);
            break;
        case LONG:
            *(long*)options[optionIndex].var.var = atoi(value);
            break;
        case FUNC_INT:
            options[optionIndex].var.boundFunction.arg.intArg = atoi(value);
            callBoundedFunction(&options[optionIndex].var.boundFunction, NULL);
            break;
        case FUNC_STR:
            options[optionIndex].var.boundFunction.arg.voidArg = value;
            callBoundedFunction(&options[optionIndex].var.boundFunction, NULL);
            break;
        case FUNC:
            callBoundedFunction(&options[optionIndex].var.boundFunction, NULL);
            break;
    }
}
static int getOptionIndexByShortName(char name){
    for(int i = 0; i < NUM_OPTIONS; i++)
        if(options[i].shortName == name)
            return i;
    return -1;
}
void listOptions(void){
    for(int i = 0; i < NUM_OPTIONS; i++)
        printValue(i);
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
void parseArgs(int argc, char* argv[], int exitOnUnknownOptions){
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
                setValue(index, optarg);
                break;
        }
    }
}

static int reading;
static void* readUserInput(void* arg __attribute__((unused))){
    if(reading)return NULL;
    reading = 1;
    unlink(fifoPipe);
    if(mkfifo(fifoPipe, 0666) == -1){
        LOG(LOG_LEVEL_ERROR, "could not create fifo pipe\n");
        exit(1);
    }
    FILE* fp = fopen(fifoPipe, "r");
    char buffer[255];
    while(!isShuttingDown()){
        if(fgets(buffer, LEN(buffer), fp)){
            lock();
            if(buffer[strlen(buffer) - 1] == '\n')
                buffer[strlen(buffer) - 1] = 0;
            parseArgs(2, (char* []){0, buffer, 0}, 0);
            unlock();
        }
        else {
            fclose(fp);
            fp = fopen(fifoPipe, "r");
        }
        msleep(10);
    }
    reading = 0;
    return NULL;
}
void startReadingUserInput(void){
    runInNewThread(readUserInput, NULL, 1);
}
