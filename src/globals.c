#include "globals.h"
#include <unistd.h>


char LOAD_SAVED_STATE=1;

int SOURCE_INDICATION_MASK=7;

long KILL_TIMEOUT=100;

int ROOT_EVENT_MASKS=XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT|XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY|XCB_EVENT_MASK_STRUCTURE_NOTIFY;

int NON_ROOT_EVENT_MASKS=XCB_EVENT_MASK_PROPERTY_CHANGE|XCB_EVENT_MASK_VISIBILITY_CHANGE;


int ROOT_DEVICE_EVENT_MASKS=XCB_INPUT_XI_EVENT_MASK_HIERARCHY|XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS;
int NON_ROOT_DEVICE_EVENT_MASKS=XCB_INPUT_XI_EVENT_MASK_FOCUS_IN|XCB_INPUT_XI_EVENT_MASK_FOCUS_OUT;

int NUMBER_OF_WORKSPACES=10;

int IGNORE_MASK=Mod2Mask;

int DEFAULT_BORDER_WIDTH=1;

char IGNORE_SEND_EVENT=0;
char IGNORE_KEY_REPEAT=0;

int WILDCARD_MODIFIER=AnyModifier;

char* SHELL;

int CLONE_REFRESH_RATE=15;

int DEFAULT_WINDOW_MASKS=0;

int DEFAULT_BINDING_MASKS=XCB_INPUT_XI_EVENT_MASK_KEY_PRESS;

int DEFAULT_WORKSPACE_INDEX=0;

Node*deviceBindings;

Display *dpy;
xcb_connection_t *dis;
int root;
xcb_ewmh_connection_t*ewmh;
xcb_screen_t* screen;
int defaultScreenNumber;

int CRASH_ON_ERRORS=0;

Node* eventRules[NUMBER_OF_EVENT_RULES];

void (*preStartUpMethod)();
void (*startUpMethod)();
void (*printStatusMethod)();
int statusPipeFD[2];
void init(){
    pipe(statusPipeFD);
    SHELL=getenv("SHELL");
    deviceBindings=createCircularHead(NULL);
    for(int i=0;i<NUMBER_OF_EVENT_RULES;i++)
        eventRules[i]=createCircularHead(NULL);
}

///Last registered event
void*lastEvent;
void setLastEvent(void* event){
    lastEvent=event;
}
void* getLastEvent(){
    return lastEvent;
}
