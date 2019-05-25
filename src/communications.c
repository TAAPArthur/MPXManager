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
#include "mywm-util.h"
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
    {"dump", {.boundFunction = BIND(dumpAllWindowInfo, MAPPABLE_MASK)}, FUNC},
    {"dump-all", {.boundFunction = BIND(dumpAllWindowInfo, 0)}, FUNC},
    {"dump-filter", {.boundFunction = BIND(dumpMatch, NULL)}, FUNC_STR},
    {"dump-win", {.boundFunction = BIND(dumpWindowInfo)}, FUNC_WIN_INFO},
    {"list-options", {.boundFunction = BIND(listOptions)}, FUNC},
    {"log-level", {.boundFunction = BIND(setLogLevel)}, FUNC_INT},
    {"quit", {.boundFunction = BIND(quit, 0)}, FUNC},
    {"restart", {.boundFunction = BIND(restart)}, FUNC},
    {"sum", {.boundFunction = BIND(printSummary)}, FUNC},
    _OPTION(AUTO_FOCUS_NEW_WINDOW_TIMEOUT),
    _OPTION(RUN_AS_WM),
    _OPTION(SYNC_FOCUS),
    _OPTION(CLONE_REFRESH_RATE),
    _OPTION(CRASH_ON_ERRORS),
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
    _OPTION(MASTER_INFO_PATH),
    _OPTION(DEFAULT_NUMBER_OF_WORKSPACES),
    _OPTION(POLL_COUNT),
    _OPTION(POLL_INTERVAL),
    _OPTION(SHELL),
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


xcb_atom_t getAtom(char* name){
    if(!name)return XCB_ATOM_NONE;
    xcb_intern_atom_reply_t* reply;
    reply = xcb_intern_atom_reply(dis, xcb_intern_atom(dis, 0, 32, name), NULL);
    xcb_atom_t atom = reply->atom;
    free(reply);
    return atom;
}
void send(char* name, char* value){
    LOG(LOG_LEVEL_DEBUG, "sending %s %s\n", name, value);
    unsigned int data[2] = {getAtom(name), getAtom(value)};
    catchError(xcb_ewmh_send_client_message(dis, root, root, WM_INTERPROCESS_COM, sizeof(data), data));
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
        case FUNC_WIN_INFO: {
            WindowInfo* winInfo = getWindowInfo(atoi(value));
            callBoundedFunction(&option->var.boundFunction, winInfo);
        }
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
    if(message == WM_INTERPROCESS_COM){
        if(data.data32[0]){
            xcb_get_atom_name_reply_t* nameReply;
            char* value = NULL;
            nameReply = xcb_get_atom_name_reply(dis, xcb_get_atom_name(dis, data.data32[0]), NULL);
            Option* option = findOption(xcb_get_atom_name_name(nameReply), nameReply->name_len);
            free(nameReply);
            if(option){
                if(data.data32[1]){
                    xcb_get_atom_name_reply_t* valueReply;
                    valueReply = xcb_get_atom_name_reply(dis, xcb_get_atom_name(dis, data.data32[1]), NULL);
                    value = malloc((valueReply->name_len + 1) * sizeof(char));
                    memcpy(value, xcb_get_atom_name_name(valueReply), (valueReply->name_len)*sizeof(char));
                    value[valueReply->name_len] = 0;
                    free(valueReply);
                }
                callOption(option, value);
                if(value)
                    free(value);
            }
        }
    }
}
