/**
 * @file mywm-structs.h
 *
 * @brief All the structs that are key to MyWM
 *
 * In general when creating an instance to a struct all values are set
 * to their defaults unless otherwise specified.
 *
 * The default for Node* is an empty node (multiple values in a struct may point to the same empty node)
 * The default for points is NULL
 * everything else 0
 */
#ifndef MYWM_STRUCTS_H_
#define MYWM_STRUCTS_H_

#include <xcb/xcb_ewmh.h>
#include <X11/Xlib.h>
#include <xcb/xcb.h>

#include "util.h"

#define UNMAPPED  0
#define MAPPED    1

#define STICK_WORKSPACE 0xFFFFFFFF
#define NO_WORKSPACE    -1
#define NO_LAYER    -1
#define DEFAULT_WORKSPACE_INDEX 0

#define DESKTOP_LAYER 0
#define LOWER_LAYER 1
#define NORMAL_LAYER 2
#define UPPER_LAYER 3   //DOCKS
#define FULL_LAYER 4
#define NUMBER_OF_LAYERS 5
#define DEFAULT_LAYER NORMAL_LAYER

#define NO_MASK                 0

#define URGENT_MASK             1<<0    //set when the window received an urgent hint
#define MINIMIZED_MASK          1<<1
#define X_MAXIMIZED_MASK        1<<2
#define Y_MAXIMIZED_MASK        1<<3
//rename
#define FULLSCREEN_MASK         1<<4
#define ROOT_FULLSCREEN_MASK    1<<5

#define NO_TILE_MASK            1<<6
#define EXTERNAL_RESIZE_MASK    1<<7
#define EXTERNAL_MOVE_MASK      1<<8

/**Window is in Above layer and is visible on all workspaces **/
#define STICKY_MASK             1<<12

#define PRIVILGED_MASK          (FULLSCREEN_MASK | ROOT_FULLSCREEN_MASK)



#define NUMBER_OF_MASKS 9
#define ALL_MASKS     1<<NUMBER_OF_MASKS -1


#define CONFIG_LEN  6
enum {DOCK_LEFT,DOCK_RIGHT,DOCK_TOP,DOCK_BOTTOM};

struct context_t;




typedef struct window_info{
    /**Window id */
    unsigned int id;
    /**Window mask */
    unsigned int mask;
    /**xcb_atom representing the window type*/
    int type;
    /**string xcb_atom representing the window type*/
    char*typeName;
    /**title of window*/
    char*title;
    /**class name of window*/
    char*className;
    /** instance name of window*/
    char*instanceName;

    struct window_info*clone;

    int implicitType;
    int workspaceIndex;
    int activeWorkspaceCount;
    int minimizedTimeStamp;

    int config[5];
    unsigned int transientFor;
    char state;
    char mapped;
    char overrideRedirect;
    char visibile;
    char input;
    unsigned int groupId;
    int properties[12];
} WindowInfo;
struct binding_struct;
typedef struct{
    /**master id; intended to corrospond to Master device id;*/
    int id;
    /**Time the focused window changed*/
    int focusedTimeStamp;
    /**Pointer to last binding triggered by this master device*/
    struct binding_struct* lastBindingTriggered;
    /**Pointer to last binding triggered by this master device*/
    struct binding_struct* activeChainBinding;
    /**Stack of windows in order of most recently focused*/
    Node* windowStack;
    /**Contains the window with current focus,
     * will be same as top of window stack if freezeFocusStack==0
     * */
    Node* focusedWindow;
    /**When true, the focus stack won't be updated on focus change*/
    int freezeFocusStack;


    /** pointer to head of node list where the values point to WindowInfo */
    Node* windowsToIgnore;

    /**Index of active workspace; this workspace will be used for
     * all workspace operations if a workspace is not specified
     * */
    int activeWorkspaceIndex;
    /**
     * id of the last window clicked
     */
    unsigned int lastWindowClicked;
    /**
     * stack of visited workspaces with most recent at top
     */
    Node*workspaceHistory;

} Master;

typedef struct{
    Atom name;
    int cloneId;
    char primary;
    short int x;
    short int y;
    short int width;
    short int height;
    short int viewX;
    short int viewY;
    short int viewWidth;
    short int viewHeight;

} Monitor;

typedef struct {
    int* args; //limit,border
    char* name;
    void (*layoutFunction)(Monitor*,Node*,int*);
    //TODO implement conditional layout
    int (*conditionFunction)(struct context_t*,int);
    int conditionArg;
} Layout;

typedef struct{
    int id;
    char*name;
    Monitor*monitor;

    Node*windows[NUMBER_OF_LAYERS];

    Layout* activeLayout;
    Node*layouts;
}Workspace;






#endif /* MYWM_STRUCTS_H_ */
