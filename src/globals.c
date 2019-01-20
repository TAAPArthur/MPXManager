#include "globals.h"
#include <unistd.h>


char LOAD_SAVED_STATE=1;

int SOURCE_INDICATION_MASK=7;

long KILL_TIMEOUT=100;

int ROOT_EVENT_MASKS=XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT|XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY|XCB_EVENT_MASK_STRUCTURE_NOTIFY;

int NON_ROOT_EVENT_MASKS=XCB_EVENT_MASK_SUBSTRUCTURE_REDIRECT|XCB_EVENT_MASK_SUBSTRUCTURE_NOTIFY|XCB_EVENT_MASK_PROPERTY_CHANGE|XCB_EVENT_MASK_VISIBILITY_CHANGE;

int KEYBOARD_MASKS=XCB_INPUT_XI_EVENT_MASK_KEY_PRESS|XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE;
int POINTER_MASKS=XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS|XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE|XCB_INPUT_XI_EVENT_MASK_MOTION;

int ROOT_DEVICE_EVENT_MASKS=XCB_INPUT_XI_EVENT_MASK_HIERARCHY|XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS;
int NON_ROOT_DEVICE_EVENT_MASKS=XCB_INPUT_XI_EVENT_MASK_FOCUS_IN;

int NUMBER_OF_WORKSPACES=10;

int IGNORE_MASK=Mod2Mask;

int DEFAULT_BORDER_WIDTH=1;
int DEFAULT_BORDER_COLOR=0;

int DEFAULT_FOCUS_BORDER_COLOR=0x00FF00;
int DEFAULT_UNFOCUS_BORDER_COLOR=0xDDDDDD;

char IGNORE_SEND_EVENT=0;
char IGNORE_KEY_REPEAT=0;


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


int POLL_COUNT=10;
int POLL_INTERVAL=10;

char * MASTER_INFO_PATH="master-info.txt";

int DEFAULT_POINTER=2;
int DEFAULT_KEYBOARD=3;

void (*preStartUpMethod)(void);
void (*startUpMethod)(void);
void (*printStatusMethod)(void);
int statusPipeFD[2];
void init(void){
    pipe(statusPipeFD);
    SHELL=getenv("SHELL");
    deviceBindings=createCircularHead(NULL);
}

///Last registered event
static void*lastEvent;
void setLastEvent(void* event){
    lastEvent=event;
}
void* getLastEvent(void){
    return lastEvent;
}
