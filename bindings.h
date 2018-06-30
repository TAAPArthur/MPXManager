
#ifndef BINDINGS_H_
#define BINDINGS_H_

#include <regex.h>
#include <X11/keysym.h>
#include <X11/X.h>
#include <X11/XF86keysym.h>
#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI.h>

#include "mywm-structs.h"

#define GENERIC_EVENT_OFFSET LASTEvent
#define KEY_PRESS_BINDING_INDEX XI_KeyPress


#define MATCH_NONE          0
#define LITERAL             1<<0
#define CASE_INSENSITIVE    1<<1
#define CLASS               1<<2
#define RESOURCE            1<<3
#define TITLE               1<<4
#define TYPE                1<<5
#define MATCH_ANY_LITERAL   ((1<<6) -1)
#define MATCH_ANY_REGEX     (MATCH_ANY_LITERAL - LITERAL)


#define ProcessingWindow        62  //if all rules are passed through, then the window is added as a normal window
#define WorkspaceChange         63
#define WindowWorkspaceChange   64
#define WindowLayerChange       65
#define LayoutChange            66
#define TilingWindows           67
#define onXConnection           68

#define NUMBER_OF_EVENT_RULES   71




#define ADD_DEVICE_EVENT_MASK(mask)    DEVICE_EVENT_MASKS|=mask;
#define ADD_ALL_DEVICE_EVENTS   ADD_DEVICE_EVENT_MASK(1<<(XI_LASTEVENT+1)-1);
#define FOCUS_FOLLOWS_MOUSE ADD_DEVICE_EVENT_MASK(XI_EnterMask);
#define FOCUS_ON_CLICK ADD_DEVICE_EVENT_MASK(XI_ButtonPressMask);
#define ADD_AVOID_STRUCTS_RULE \
    ADD_WINDOW_RULE(ProcessingWindow, \
            CREATE_RULE("_NET_WM_WINDOW_TYPE_DOCK",TYPE|LITERAL,BIND_TO_WIN_FUNC(avoidStruct)));

#define DEFAULT_REGEX_FLAG REG_EXTENDED

#define ADD_WINDOW_RULE(E,R) \
    insertHead(eventRules[E],memcpy(malloc(sizeof(Rules)),&((Rules)R), sizeof(Rules))); \
    assert((((Rules*)getValue(eventRules[E])))->onMatch.type!=UNSET); \
    COMPILE_RULE(((Rules*)getValue(eventRules[E])))

#define COMPILE_RULE(R) \
     if((R->ruleTarget & LITERAL) == 0){\
        R->regexExpression=malloc(sizeof(regex_t));\
        assert(0==regcomp(R->regexExpression,R->literal, \
                (DEFAULT_REGEX_FLAG)|((R->ruleTarget & CASE_INSENSITIVE)?REG_ICASE:0)));\
    }

/*
if(((&r3)->ruleTarget & 1<<0) == 0){\
        (&r3)->regexExpression=malloc(sizeof(regex_t));\
        regcomp((&r3)->regexExpression,(&r3)->literal, \
                1 | (1 << 1)|(((&r3)->ruleTarget & 1<<1)?(1 << 1):0));\
    }*/
//#define CREATE_TYPE_RULE(C,F,...) {NULL,NULL,C,0,F,__VA_ARGS__}
//#define CREATE_CLASS_RULE(C,F,...) {NULL,C,NULL,0,F,__VA_ARGS__}
//#define CREATE_TITLE_RULE(C,F,...) {C,NULL,NULL,0,F,__VA_ARGS__}

enum{UNSET,CHAIN,NO_ARGS,INT_ARG,CHAR_ARG,WIN_ARG,VOID_ARG,ARR_ARG};
#define BIND_TO_STR_FUNC(F,S) {.func={.funcStrArg=F},.arg={.voidArg=S},.type=VOID_ARG}
#define BIND_TO_INT_FUNC(F,I)    {.func={.funcInt=F},.arg={.intArg=I},.type=INT_ARG}
#define BIND_TO_WIN_FUNC(F)    {.func=F,.type=WIN_ARG}
#define BIND_TO_FUNC(F)    {.func=F,.type=NO_ARGS}
#define BIND_TO_ARFUNC(F,V)    {.func={.funcLayoutArg=F},{.voidArg=V},.type=VOID_ARG}
#define BIND_TO_AYOUT_FUNC(F,V)    {.func={.funcVoidArg=F},{.voidArg=V},.type=VOID_ARG}

#define BIND_TO_RULE_FUNC(F,...) {.func={.funcRuleArg=F},.arg={.voidArg=(Rules*)(Rules[]){__VA_ARGS__}},.type=VOID_ARG}
#define SPAWN(COMMAND_STR)  BIND_TO_STR_FUNC(spawn,COMMAND_STR)

#define CHAIN(...) CHAIN_GRAB(1,__VA_ARGS__)
#define CHAIN_GRAB(GRAB,...) {.func={.chainBindings=(Binding*)(Binding[]){__VA_ARGS__,{0}}},.arg={.intArg=GRAB},.type=CHAIN}

#define CREATE_DEFAULT_EVENT_RULE(func) {NULL,0,BIND_TO_FUNC(func),.passThrough=1}
#define CREATE_WILDCARD(...) {NULL,0,__VA_ARGS__}
#define CREATE_RULE(expression,target,func) {expression,target,func}
#define CREATE_LITERAL_RULE(E,T,F) {E,(T|LITERAL),F}

#define RUN_RAISE_ANY(STR_TO_MATCH)    RUN_OR_RAISE_TYPE(STR_TO_MATCH,ANY,STR_TO_MATCH)
#define RUN_OR_RAISE_CLASS(STR_TO_MATCH)  RUN_OR_RAISE_TYPE(STR_TO_MATCH,CLASS|CASE_INSENSITIVE|LITERAL,STR_TO_MATCH)
#define RUN_OR_RAISE_TITLE(STR_TO_MATCH,COMMAND)  RUN_OR_RAISE_TYPE(STR_TO_MATCH,TITLE,COMMAND)

#define RUN_OR_RAISE_TYPE(STR_TO_MATCH,TYPE,COMMAND_STR) \
    BIND_TO_RULE_FUNC(runOrRaise,CREATE_RULE(STR_TO_MATCH,TYPE,NULL),{0},CREATE_WILDCARD(SPAWN(COMMAND_STR)),{0})

#define WORKSPACE_OPERATION(K,N) \
    {  Mod4Mask,             K, BIND_TO_INT_FUNC(moveToWorkspace,N)}, \
    {  Mod4Mask|ShiftMask,   K, BIND_TO_INT_FUNC(sendToWorkspace,N)}/*, \
    {  Mod4Mask|ControlMask,   K, BIND_TO_INT_FUNC(cloneToWorkspace,N)}, \
    {  Mod4Mask|Mod1Mask,   K, BIND_TO_INT_FUNC(swapWithWorkspace,N)}, \
    {  Mod4Mask|ControlMask|Mod1Mask,   K, BIND_TO_INT_FUNC(stealFromWorkspace,N)}, \
    {  Mod4Mask|Mod1Mask|ShiftMask,   K, BIND_TO_INT_FUNC(giveToWorkspace,N)},\
    {  Mod4Mask|ControlMask|Mod1Mask|ShiftMask,   K, BIND_TO_INT_FUNC(tradeWithWorkspace,N)}*/

#define STACK_OPERATION(UP,DOWN,LEFT,RIGHT) \
    {  Mod4Mask,             UP, BIND_TO_INT_FUNC(shiftPosition,1)}, \
    {  Mod4Mask,             DOWN, BIND_TO_INT_FUNC(shiftPosition,-1)}, \
    {  Mod4Mask,             LEFT, BIND_TO_INT_FUNC(shiftFocus,1)}, \
    {  Mod4Mask,             RIGHT, BIND_TO_INT_FUNC(shiftFocus,-1)}/*, \
    {  Mod4Mask|ShiftMask,             UP, BIND_TO_INT_FUNC(popMin,1)}, \
    {  Mod4Mask|ShiftMask,             DOWN, BIND_TO_INT_FUNC(pushMin,1)}, \
    {  Mod4Mask|ShiftMask,             LEFT, BIND_TO_INT_FUNC(sendToNextWorkNonEmptySpace,1)}, \
    {  Mod4Mask|ShiftMask,             RIGHT, BIND_TO_INT_FUNC(sendToNextWorkNonEmptySpace,-1)}, \
    {  Mod4Mask|ControlMask,             UP, BIND_TO_INT_FUNC(swapWithNextMonitor,1)}, \
    {  Mod4Mask|ControlMask,             DOWN, BIND_TO_INT_FUNC(swapWithNextMonitor,-1)}, \
    {  Mod4Mask|ControlMask,             LEFT, BIND_TO_INT_FUNC(sendToNextMonitor,1)}, \
    {  Mod4Mask|ControlMask,             RIGHT, BIND_TO_INT_FUNC(sendToNextMonitor,-1)}, \
    {  Mod4Mask|Mod1Mask,             UP, BIND_TO_FUNC(focusTop)}, \
    {  Mod4Mask|Mod1Mask,             DOWN, BIND_TO_FUNC(focusBottom)}, \
    {  Mod4Mask|Mod1Mask,             LEFT, BIND_TO_INT_FUNC(sendtoBottom,1)}, \
    {  Mod4Mask|Mod1Mask,             RIGHT, BIND_TO_FUNC(sendFromBottomToTop)}*/




struct rules_t;
struct bound_function_t;

typedef struct{
    union{
        void (*funcNoArg)();
        void (*funcVoidArg)(void*);
        void (*funcInt)(int);
        void (*funcArrArg)(void*,int);
        void (*funcStrArg)(char*);
        void (*funcLayoutArg)(Layout*);
        int (*funcRuleArg)(struct rules_t*);
        struct binding_struct *chainBindings;
    } func;
    union{
        char* charArg;
        int intArg;
        void* voidArg;
        struct{
            void*voidArg;
            int len;
        } arrArg;
    } arg;
    int type;
} BoundFunction;

typedef struct binding_struct{
    /**Modifer to match on*/
    unsigned int mod;
    /**Either Button or KeySym to match**/
    int detail;
    /**Function to call on match**/
    BoundFunction boundFunction;
    /**Wheter more bindings should be considered*/
    int passThrough;
    /**Whether a device should be grabbed or not*/
    int noGrab;
    /**Converted detail of key bindings*/
    KeyCode keyCode;
} Binding;

typedef struct rules_t{
    char* literal;
    int ruleTarget;
    BoundFunction onMatch;
    int passThrough;
    regex_t* regexExpression;
} Rules;


/**
 * Grab all the buttons modifer combo specified by binding
 * @param binding the list of binding
 * @param numKeys   the number of bindings
 * @param mask  XI_BUTTON_PRESS or RELEASE
 */
void grabButtons(Binding*binding,int num,int mask);
/**
 * Grab all the keys/modifer combo specified by binding
 * @param binding the list of binding
 * @param numKeys   the number of bindings
 * @param mask  XI_KEY_PRESS or RELEASE
 */
void grabKeys(Binding*Binding,int numKeys,int mask);


int doesStringMatchRule(Rules*rule,char*str);
int doesWindowMatchRule(Rules *rules,WindowInfo*info);
int applyRule(Rules*rule,WindowInfo*info);

int applyGenericEventRules(int type,WindowInfo*info);
int applyEventRules(int type,WindowInfo*info);
void callBoundedFunction(BoundFunction*boundFunction,WindowInfo*info);

void loadWindowProperties(WindowInfo *winInfo);


extern Node* eventRules[NUMBER_OF_EVENT_RULES];

extern Binding*bindings[4];
extern int bindingLengths[4];
extern const int bindingMasks[4];

void checkBindings(int keyCode,int mods,int bindingType,WindowInfo*info);
#endif
