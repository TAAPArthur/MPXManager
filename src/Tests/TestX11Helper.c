#include <err.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "UnitTests.h"
#include "TestX11Helper.h"
#include "../default-rules.h"
#include "../logger.h"
#include "../xsession.h"
#include "../devices.h"
#include "../events.h"


void destroyWindow(WindowID win){
    assert(NULL == xcb_request_check(dis, xcb_destroy_window_checked(dis, win)));
}
static int createWindow(int parent, int mapped, int ignored, int userIgnored, int input, int class){
    assert(ignored < 2);
    assert(dis);
    xcb_window_t window = xcb_generate_id(dis);
    xcb_void_cookie_t c = xcb_create_window_checked(dis,                   /* Connection          */
                          XCB_COPY_FROM_PARENT,          /* depth (same as root)*/
                          window,                        /* window Id           */
                          parent,                  /* parent window       */
                          0, 0,                          /* x, y                */
                          150, 150,                      /* width, height       */
                          0,                            /* border_width        */
                          class, /* class               */
                          screen->root_visual,           /* visual              */
                          XCB_CW_OVERRIDE_REDIRECT, &ignored);                     /* masks, not used yet */
    xcb_generic_error_t* e = xcb_request_check(dis, c);
    if(e){
        logError(e);
        err(1, "could not create window\n");
    }
    xcb_icccm_wm_hints_t hints = {.input = input, .initial_state = mapped};
    e = xcb_request_check(dis, xcb_icccm_set_wm_hints_checked(dis, window, &hints));
    if(e){
        err(1, "could not set hintsfor window on creation\n");
    }
    flush();
    if(!userIgnored && !ignored)
        catchError(xcb_ewmh_set_wm_window_type_checked(ewmh, window, 1, &ewmh->_NET_WM_WINDOW_TYPE_NORMAL));
    return window;
}
int  createInputOnlyWindow(void){
    return createWindow(root, 1, 0, 0, 1, XCB_WINDOW_CLASS_INPUT_ONLY);
}
int  createInputWindow(int input){
    return createWindow(root, 1, 0, 0, input, XCB_WINDOW_CLASS_INPUT_OUTPUT);
}
int createUserIgnoredWindow(void){
    return createWindow(root, 1, 0, 1, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT);
}
int createIgnoredWindow(void){
    return createWindow(root, 1, 1, 0, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT);
}
int createNormalWindow(void){
    return createWindow(root, 1, 0, 0, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT);
}
int createNormalWindowWithType(xcb_atom_t type){
    Window win = createWindow(root, 1, 0, 0, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT);
    assert(!catchError(xcb_ewmh_set_wm_window_type_checked(ewmh, win, 1, &type)));
    return win;
}
int createNormalSubWindow(int parent){
    return createWindow(parent, 1, 0, 0, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT);
}
int  createUnmappedWindow(void){
    return createWindow(root, 0, 0, 0, 1, XCB_WINDOW_CLASS_INPUT_OUTPUT);
}

int mapWindow(int win){
    assert(xcb_request_check(dis, xcb_map_window_checked(dis, win)) == NULL);
    flush();
    return win;
}
int  mapArbitraryWindow(void){
    return mapWindow(createUnmappedWindow());
}

void createSimpleContext(){
    resetContext();
}

void createContextAndSimpleConnection(){
    createSimpleContext();
    openXDisplay();
    initCurrentMasters();
    detectMonitors();
}
void destroyContextAndConnection(){
    closeConnection();
    resetContext();
}

void* getNextDeviceEvent(){
    xcb_flush(dis);
    void* event = xcb_wait_for_event(dis);
    LOG(LOG_LEVEL_DEBUG, "Event received%d\n\n", ((xcb_generic_event_t*)event)->response_type);
    if(((xcb_generic_event_t*)event)->response_type == XCB_GE_GENERIC){
        loadGenericEvent(event);
        xcb_input_key_press_event_t* dEvent = event;
        LOG(LOG_LEVEL_DEBUG, "Detail %d type %s\n", dEvent->detail,
            eventTypeToString(GENERIC_EVENT_OFFSET + dEvent->event_type));
    }
    return event;
}
void triggerBinding(Binding* b){
    int id = isKeyboardMask(b->mask) ?
             getActiveMasterKeyboardID() : getActiveMasterPointerID();
    sendDeviceAction(id, b->detail, (int)log2(b->mask));
    if(b->mask == XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS)
        sendDeviceAction(id, b->detail, XCB_INPUT_BUTTON_RELEASE);
    if(b->mask == XCB_INPUT_XI_EVENT_MASK_KEY_PRESS)
        sendDeviceAction(id, b->detail, XCB_INPUT_KEY_RELEASE);
}
void triggerAllBindings(int mask){
    flush();
    FOR_EACH(Binding*, b, getDeviceBindings()){
        if(mask & b->mask)
            triggerBinding(b);
    }
    xcb_flush(dis);
}

void waitToReceiveInput(int mask, int detailMask){
    flush();
    LOG(LOG_LEVEL_ALL, "waiting for input %d\n\n", mask);
    while(mask || detailMask){
        xcb_input_key_press_event_t* e = getNextDeviceEvent();
        LOG(LOG_LEVEL_ALL, "type %d\n", e->response_type);
        if(getActiveMaster() && (mask & (1 << XCB_INPUT_MOTION)))
            setLastKnowMasterPosition(e->root_x >> 16, e->root_y >> 16);
        LOG(LOG_LEVEL_ALL, "type %d (%d); detail %d remaining mask:%d %d\n", e->response_type, (1 << e->event_type), e->detail,
            mask, detailMask);
        mask &= ~(1 << e->event_type);
        detailMask &= ~(1 << e->detail);
        free(e);
        msleep(10);
    }
}
int consumeEvents(){
    lock();
    xcb_generic_event_t* e;
    int numEvents = 0;
    while(1){
        e = xcb_poll_for_event(dis);
        if(e){
            numEvents++;
            LOG(LOG_LEVEL_ALL, "Event ignored %d %s\n",
                e->response_type, eventTypeToString(e->response_type & 127));
            free(e);
        }
        else break;
    }
    unlock();
    return numEvents;
}

int waitForNormalEvent(int type){
    flush();
    while(type){
        xcb_generic_event_t* e = xcb_wait_for_event(dis);
        LOG(LOG_LEVEL_ALL, "Found event\n");
        if(!e)
            return 0;
        LOG(LOG_LEVEL_ALL, "type %d %s\n", e->response_type, eventTypeToString(e->response_type));
        int t = e->response_type & 127;
        free(e);
        if(type == t)
            break;
    }
    return 1;
}

int isWindowMapped(WindowID win){
    xcb_get_window_attributes_reply_t* reply;
    reply = xcb_get_window_attributes_reply(dis, xcb_get_window_attributes(dis, win), NULL);
    int result = reply->map_state != XCB_MAP_STATE_UNMAPPED;
    free(reply);
    return result;
}

Window createDock(int i, int size, int full){
    return setDock(createNormalWindow(), i, size, full);
}
Window setDock(WindowID win, int i, int size, int full){
    assert(i >= 0);
    assert(i < 4);
    int strut[12] = {0};
    strut[i] = size;
    //strut[i*2+4+1]=0;
    xcb_void_cookie_t cookie = !full ?
                               xcb_ewmh_set_wm_strut_partial_checked(ewmh, win, *((xcb_ewmh_wm_strut_partial_t*)strut)) :
                               xcb_ewmh_set_wm_strut_checked(ewmh, win, strut[0], strut[1], strut[2], strut[3]);
    assert(xcb_request_check(dis, cookie) == NULL);
    xcb_atom_t atom[] = {ewmh->_NET_WM_WINDOW_TYPE_DOCK};
    assert(!xcb_request_check(
               dis, xcb_ewmh_set_wm_window_type_checked(ewmh, win, 1, atom)));
    return win;
}


extern void requestShutdown();
void fullCleanup(){
    setLogLevel(LOG_LEVEL_NONE + 1);
    requestShutdown();
    registerForWindowEvents(root, ROOT_EVENT_MASKS);
    //wake up other thread
    createNormalWindow();
    flush();
    waitForAllThreadsToExit();
    destroyAllNonDefaultMasters();
    quit(0);
}
int getExitStatusOfFork(){
    int status = 0;
    wait(&status);
    if(WIFEXITED(status))
        return WEXITSTATUS(status);
    else return -1;
}

void waitForExit(int signal){
    int status = getExitStatusOfFork();
    LOG(LOG_LEVEL_DEBUG, "exit status %d\n", status);
    assert(status == signal);
}
void waitForCleanExit(){
    waitForExit(EXIT_SUCCESS);
}


static char* classInstance = "Instance\0Class";
static char* clazz = "Class";
static char* instace = "Instance";
static char* title = "Title";
static char* typeName;
static xcb_atom_t* atomTypes;
static xcb_atom_t* atom;

void loadSampleProperties(WindowInfo* winInfo){
    const char* dummyString = "dummy";
    char** fields = &winInfo->typeName;
    for(int i = 0; i < 4; i++)
        fields[i] = memcpy(calloc(strlen(dummyString) + 1, sizeof(char)), dummyString, strlen(dummyString) * sizeof(char));
}
void setProperties(WindowID win){
    if(!atom){
        atom = &ewmh->_NET_WM_WINDOW_TYPE_NORMAL;
        typeName = "_NET_WM_WINDOW_TYPE_NORMAL";
    }
    atomTypes = atom;
    int size = strlen(classInstance) + 1;
    size += strlen(&classInstance[size]);
    xcb_void_cookie_t cookies[] = {
        xcb_ewmh_set_wm_name_checked(ewmh, win, strlen(title), title),
        xcb_icccm_set_wm_class_checked(dis, win, size, classInstance),
        xcb_ewmh_set_wm_window_type_checked(ewmh, win, 1, atom)
    };
    for(int i = 0; i < 3; i++)
        assert(xcb_request_check(dis, cookies[i]) == NULL);
}
void checkProperties(WindowInfo* winInfo){
    assert(winInfo->className);
    assert(winInfo->instanceName);
    assert(winInfo->title);
    assert(winInfo->typeName);
    assert(winInfo->type);
    assert(strcmp(title, winInfo->title) == 0);
    assert(strcmp(typeName, winInfo->typeName) == 0);
    assert(*atomTypes == winInfo->type);
    assert(strcmp(instace, winInfo->instanceName) == 0);
    assert(strcmp(clazz, winInfo->className) == 0);
}
int checkStackingOrder(WindowID* stackingOrder, int num){
    xcb_query_tree_reply_t* reply;
    reply = xcb_query_tree_reply(dis, xcb_query_tree(dis, root), 0);
    if(!reply){
        LOG(LOG_LEVEL_WARN, "could not query tree\n");
        return 0;
    }
    int numberOfChildren = xcb_query_tree_children_length(reply);
    xcb_window_t* children = xcb_query_tree_children(reply);
    int counter = 0;
    PRINT_ARR("stacking order", children, numberOfChildren, "\n");
    PRINT_ARR("Target order: ", stackingOrder, num, "\n");
    for(int i = 0; i < numberOfChildren; i++){
        if(children[i] == stackingOrder[counter]){
            counter++;
            if(counter == num)
                break;
        }
    }
    free(reply);
    return counter == num;
}

