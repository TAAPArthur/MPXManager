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

#include <regex.h>
#include "util.h"

#define UNMAPPED  0
#define MAPPED    1

#define ALL_WORKSPACES 0xFFFFFFFF
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



#define WINDOW_STRUCT_ARRAY_SIZE    12
typedef struct window_info{
    /**Window id */
    unsigned int id;
    /**Window mask */
    unsigned int mask;
    /**xcb_atom representing the window type*/
    int type;
    /**string xcb_atom representing the window type*/
    char*typeName;
    /**class name of window*/
    char*className;
    /** instance name of window*/
    char*instanceName;
    /**title of window*/
    char*title;


    int implicitType;
    /**
     * Used to indicate the "last seen in" workspace.
     * A window should only be tiled if the this field matches
     * the tiling workspace's index
     *
     * Note that his field may not contain all the workspaces an window belongs to
     */
    int workspaceIndex;
    int activeWorkspaceCount;
    int minimizedTimeStamp;

    int config[5];
    unsigned int transientFor;
    char onlyOnPrimary;
    char state;
    char mapped;
    char overrideRedirect;
    /**Keeps track on the visibility state of the window: either
     * obsucred, partialy obsured or visible. This is not used for anything by default
     *
     */
    char visibile;
    char input;
    unsigned int groupId;
    int properties[WINDOW_STRUCT_ARRAY_SIZE];
} WindowInfo;

struct binding_struct;
typedef struct{
    /**master id; intended to corrospond to Master device id;*/
    int id;
    int pointerId;
    /**Stack of windows in order of most recently focused*/
    Node* windowStack;
    /**Contains the window with current focus,
     * will be same as top of window stack if freezeFocusStack==0
     * */
    Node* focusedWindow;

    /**Time the focused window changed*/
    int focusedTimeStamp;
    /**Pointer to last binding triggered by this master device*/
    struct binding_struct* lastBindingTriggered;
    /**List of the active chains with the newest first*/
    Node*activeChains;
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
    /**TODO Needs to be an unsigned long*/
    int name;
    int cloneId;
    /**1 iff the monitor is the primary*/
    char primary;
    /**The unmodified size of the monitor*/
    short int x;
    short int y;
    short int width;
    short int height;
    /**The modified size of the monitor*/
    short int viewX;
    short int viewY;
    short int viewWidth;
    short int viewHeight;

} Monitor;

typedef struct {
    /**
     * Arguments to pass into layout functions.
     * These vary with the function but the first arguments are
     * limit and border
     */
    int* args; //limit,border
    /**
     * The name of the layout. Used solefly for user
     */
    char* name;
    /**
     * The function to call to tile the windows
     * @param
     * @param
     * @param
     */
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

#define DEFAULT_REGEX_FLAG REG_EXTENDED

/**Rule will match nothing */
#define MATCH_NONE          0
/**Rule will be interpreted literally as opposed to Regex */
#define LITERAL             1<<0
/**Rule will be will ignore case */
#define CASE_INSENSITIVE    1<<1
/**Rule will match on window type name */
#define TYPE                1<<2
/**Rule will match on window class name */
#define CLASS               1<<3
/**Rule will match on window instance name */
#define RESOURCE            1<<4
/**Rule will match on window title/name */
#define TITLE               1<<5
/**Rule will match on above property of window literally */
#define MATCH_ANY_LITERAL   ((1<<6) -1)
/**Rule will match on above property of window not literally */
#define MATCH_ANY_REGEX     (MATCH_ANY_LITERAL - LITERAL)


typedef enum{
    GRAB_AUTO=0,
    /**Automatically which grab should be used*/
    NO_GRAB=1,
    /**Grabs the specified keycode*/
    GRAB_KEY,
    /**Grabs the specified button*/
    GRAB_BUTTON,

}GrabType;
typedef enum{
    /**Don't call a function*/
    UNSET=0,
    /**Start a chain binding*/
    CHAIN,
    /**Call a function that takes no arguments*/
    NO_ARGS,
    /**Call a function that takes a single int argument*/
    INT_ARG,
    /**Call a function that takes a WindowInfo as an object
     * The WindowInfo provided to the function will be set to the active window
     */
    WIN_ARG,
    /**
     * Call a function that takes a non-function pointer
     * as an argument
     */
    VOID_ARG,
}BindingType;
/**
 * Used to bind a function with arguments to a Rule or Binding
 * such that when the latter is triggered the the bound function
 * is called with arguments
 */
typedef struct{
    /**Union of supported functions*/
    union{
        /**Function that takes no arguments*/
        void (*funcNoArg)();
        /**Function that takes a signle int as an*/
        void (*funcInt)(int);
        /**Function that takes one pointer as an argument*/
        void (*funcVoidArg)(void*);
        /**An array of Bindings terminated with a by the last member having .endChain==1*/
        struct binding_struct *chainBindings;
    } func;
    /**Arguments to pass into the bound function*/
    union{
        /**an int*/
        int intArg;
        /**a pointer*/
        void* voidArg;
    } arg;
    /**The type of Binding which is used to call the right function*/
    BindingType type;
} BoundFunction;

typedef struct binding_struct{
    /**Modifer to match on*/
    unsigned int mod;
    /**Either Button or KeySym to match**/
    int buttonOrKey;
    /**Function to call on match**/
    BoundFunction boundFunction;
    /**Wheter more bindings should be considered*/
    int passThrough;
    /**Whether a device should be grabbed or not*/
    GrabType noGrab;
    /**If in an array, this property being set to 1 signifies the end*/
    char endChain;
    /**If a binding is pressed that doesn't match the chain, don't end it*/
    char noEndOnPassThrough;
    /**Converted detail of key bindings. Should not be set manually*/
    int detail;
} Binding;

typedef struct{
    /**Literal expression*/
    char* literal;
    /**What the rule should match*/
    int ruleTarget;
    /**Function to be called when rule is matched*/
    BoundFunction onMatch;
    /**If false then subsequent rules won't be checked if this rule matches*/
    int passThrough;
    /**Compiled regex used for matching with non-literal rules*/
    regex_t* regexExpression;
} Rules;


#endif /* MYWM_STRUCTS_H_ */
