#include <assert.h>
#include <fcntl.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "communications.h"
#include "bindings.h"
#include "events.h"
#include "globals.h"
#include "logger.h"
#include "wmfunctions.h"
#include "mywm-util.h"
#include "spawn.h"
#include "windows.h"
#include "xsession.h"

static void listOptions(void);
#define _OPTION(VAR){#VAR,&VAR,\
   _Generic((VAR), \
        bool:BOOL_VAR, \
        int:INT_VAR, \
        unsigned int:INT_VAR, \
        long:LONG_VAR, \
        char*:STRING_VAR \
)}

ArrayList options;
static Option default_options[] = {
    {"dump", {.boundFunction = BIND(dumpAllWindowInfo, MAPPABLE_MASK)}, FUNC, .forkOnReceive = 1},
    {"dump-all", {.boundFunction = BIND(dumpAllWindowInfo, 0)}, FUNC, .forkOnReceive = 1},
    {"dump-filter", {.boundFunction = BIND(dumpMatch, NULL)}, FUNC_STR, .forkOnReceive = 1},
    {"dump-win", {.boundFunction = BIND(dumpWindowInfo)}, FUNC_WIN_INFO, .forkOnReceive = 1},
    {"list-options", {.boundFunction = BIND(listOptions)}, FUNC, .forkOnReceive = 1},
    {"log-level", {.boundFunction = BIND(setLogLevel)}, FUNC_INT, .forkOnReceive = 0},
    {"quit", {.boundFunction = BIND(quit, 0)}, FUNC, .forkOnReceive = 0},
    {"restart", {.boundFunction = BIND(restart)}, FUNC, .forkOnReceive = 0},
    {"sum", {.boundFunction = BIND(printSummary)}, FUNC, .forkOnReceive = 1},
    _OPTION(AUTO_FOCUS_NEW_WINDOW_TIMEOUT),
    _OPTION(CLONE_REFRESH_RATE),
    _OPTION(CRASH_ON_ERRORS),
    _OPTION(DEFAULT_BORDER_COLOR),
    _OPTION(DEFAULT_BORDER_WIDTH),
    _OPTION(DEFAULT_FOCUS_BORDER_COLOR),
    _OPTION(DEFAULT_MODE),
    _OPTION(DEFAULT_MOD_MASK),
    _OPTION(DEFAULT_NUMBER_OF_HIDDEN_WORKSPACES),
    _OPTION(DEFAULT_NUMBER_OF_WORKSPACES),
    _OPTION(DEFAULT_UNFOCUS_BORDER_COLOR),
    _OPTION(DEFAULT_WORKSPACE_INDEX),
    _OPTION(EVENT_PERIOD),
    _OPTION(IGNORE_KEY_REPEAT),
    _OPTION(IGNORE_MASK),
    _OPTION(IGNORE_SEND_EVENT),
    _OPTION(IGNORE_SUBWINDOWS),
    _OPTION(KILL_TIMEOUT),
    _OPTION(LD_PRELOAD_INJECTION),
    _OPTION(LOAD_SAVED_STATE),
    _OPTION(MASTER_INFO_PATH),
    _OPTION(POLL_COUNT),
    _OPTION(POLL_INTERVAL),
    _OPTION(RUN_AS_WM),
    _OPTION(SHELL),
    _OPTION(SPAWN_ENV),
    _OPTION(STEAL_WM_SELECTION),
    _OPTION(SYNC_FOCUS),
    _OPTION(SYNC_WINDOW_MASKS),
};
void printOption(Option* option){
    switch(option->type){
        case STRING_VAR:
            LOG(LOG_LEVEL_INFO, "%s='%s'\n", option->name, *(char**)option->var.var);
            break;
        case BOOL_VAR:
            LOG(LOG_LEVEL_INFO, "%s=%d\n", option->name, *(char*)option->var.var);
            break;
        case INT_VAR:
            LOG(LOG_LEVEL_INFO, "%s=%d\n", option->name, *(int*)option->var.var);
            break;
        case LONG_VAR:
            LOG(LOG_LEVEL_INFO, "%s=%ld\n", option->name, *(long*)option->var.var);
            break;
        default:
            LOG(LOG_LEVEL_INFO, "%s\n", option->name);
    }
}

void addOption(Option* option, int len){
    for(int i = 0; i < len; i++)
        addToList(&options, &option[i]);
}
void loadDefaultOptions(void){
    clearList(&options);
    addOption(default_options, LEN(default_options));
}
/**
 * Lists all options with their current value
 */
static void listOptions(void){
    FOR_EACH(Option*, option, &options)printOption(option);
}

static int outstandingSendCount;
int getConfirmedSentMessage(WindowID win){
    xcb_get_property_cookie_t cookie;
    xcb_get_property_reply_t* reply;
    cookie = xcb_get_property(dis, 0, win, WM_INTERPROCESS_COM, XCB_ATOM_CARDINAL, 0, sizeof(int));
    int result = 0;
    if((reply = xcb_get_property_reply(dis, cookie, NULL)) && xcb_get_property_value_length(reply)){
        result = *(int*)xcb_get_property_value(reply);
    }
    free(reply);
    return result;
}
bool hasOutStandingMessages(void){
    LOG(0, "CCCCCC %d %d %d\n", outstandingSendCount, getConfirmedSentMessage(getPrivateWindow()), getPrivateWindow());
    return outstandingSendCount > getConfirmedSentMessage(getPrivateWindow());
}
static void sendConfirmation(WindowID target){
    LOG(LOG_LEVEL_DEBUG, "sending receive confirmation to %d\n", target);
    int count = getConfirmedSentMessage(target) + 1;
    catchError(xcb_change_property_checked(dis, XCB_PROP_MODE_REPLACE, target, WM_INTERPROCESS_COM, XCB_ATOM_CARDINAL, 32,
                                           1, &count));
    flush();
}
void send(char* name, char* value){
    unsigned int data[5] = {getAtom(name), getAtom(value), getpid(), getLogLevel(), getPrivateWindow()};
    LOG(LOG_LEVEL_DEBUG, "sending %d (%s) %d (%s)\n", data[0], name, data[1], value);
    catchError(xcb_ewmh_send_client_message(dis, root, root, WM_INTERPROCESS_COM, sizeof(data), data));
    outstandingSendCount++;
}
Option* findOption(char* name, int len){
    Option* option = NULL;
    UNTIL_FIRST(option, &options, option->name == name || name && strncasecmp(name, option->name, len) == 0)
    return option;
}
void callOption(Option* option, char* value){
    if(!value)
        value = "1";
    if(option->type < FUNC)
        LOG(LOG_LEVEL_DEBUG, "setting %s to %s\n", option->name, value);
    switch(option->type){
        case STRING_VAR:
            *(char**)option->var.var = strdup(value);
            break;
        case BOOL_VAR:
            *(char*)option->var.var = atoi(value);
            break;
        case INT_VAR:
            *(int*)option->var.var = atoi(value);
            break;
        case LONG_VAR:
            *(long*)option->var.var = atol(value);
            break;
        case FUNC_INT:
            option->var.boundFunction.arg.intArg = atoi(value);
            callBoundedFunction(&option->var.boundFunction, NULL);
            break;
        case FUNC_WIN_INFO:
            callBoundedFunction(&option->var.boundFunction,  getWindowInfo(atoi(value)));
            break;
        case FUNC_STR:
            option->var.boundFunction.arg.voidArg = value;
            callBoundedFunction(&option->var.boundFunction, (void*)value);
            break;
        case FUNC:
            callBoundedFunction(&option->var.boundFunction, NULL);
            break;
    }
}
void enableInterClientCommunication(void){
    static Rule receiveRules = CREATE_DEFAULT_EVENT_RULE(receiveClientMessage);
    addToList(getEventRules(XCB_CLIENT_MESSAGE), &receiveRules);
}

void receiveClientMessage(void){
    xcb_client_message_event_t* event = getLastEvent();
    xcb_client_message_data_t data = event->data;
    xcb_atom_t message = event->type;
    xcb_atom_t optionAtom = data.data32[0];
    xcb_atom_t valueAtom = data.data32[1];
    pid_t callerPID = data.data32[2];
    xcb_atom_t logLevel = data.data32[3];
    WindowID sender = data.data32[4];
    if(message == WM_INTERPROCESS_COM && event->window == root && optionAtom){
        dumpAtoms(data.data32, 5);
        xcb_get_atom_name_reply_t* nameReply;
        char* value = NULL;
        Option* option = NULL;
        nameReply = xcb_get_atom_name_reply(dis, xcb_get_atom_name(dis, optionAtom), NULL);
        assert(nameReply);
        if(nameReply){
            option = findOption(xcb_get_atom_name_name(nameReply), nameReply->name_len);
            free(nameReply);
        }
        if(option){
            if(valueAtom)
                getAtomValue(valueAtom, &value);
            int childPID;
            if(!ENABLE_FORK_ON_RECEIVE || !option->forkOnReceive){
                callOption(option, value);
            }
            else if(!(childPID = fork())){
                openXDisplay();
                char outputFile[64] = {0};
                if(callerPID){
                    char* formatter = "/proc/%d/fd/1";
                    sprintf(outputFile, formatter, callerPID);
                    if(dup2(open(outputFile, O_WRONLY | O_APPEND), LOG_FD) == -1){
                        LOG(LOG_LEVEL_WARN, "could not open %s for writing; could not call/set %s aborting\n", outputFile, option->name);
                        exit(0);
                    }
                }
                setLogLevel(logLevel);
                callOption(option, value);
                if(callerPID)
                    LOG(LOG_LEVEL_INFO, "\n");
                exit(0);
            }
            else {
                int  returnVal __attribute__((unused));
                returnVal = waitForChild(childPID);
                assert(returnVal == 0);
            }
            if(value)
                free(value);
        }
        sendConfirmation(sender);
    }
}
