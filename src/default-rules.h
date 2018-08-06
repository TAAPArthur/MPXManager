
/**
 * @file defaults.h
 * @brief Sane defaults
 *
 */
#ifndef DEFAULT_RULES_H_
#define DEFAULT_RULES_H_


#define ADD_DEVICE_EVENT_MASK(mask)    DEVICE_EVENT_MASKS|=mask;
#define ADD_ALL_DEVICE_EVENTS   ADD_DEVICE_EVENT_MASK(1<<(XI_LASTEVENT+1)-1);
#define FOCUS_FOLLOWS_MOUSE ADD_DEVICE_EVENT_MASK(XI_EnterMask);
#define FOCUS_ON_CLICK ADD_DEVICE_EVENT_MASK(XI_ButtonPressMask);
#define ADD_AVOID_STRUCTS_RULE \
    ADD_WINDOW_RULE(ProcessingWindow, \
            CREATE_RULE("_NET_WM_WINDOW_TYPE_DOCK",TYPE|LITERAL,BIND_TO_WIN_FUNC(avoidStruct)));

#define RUN_RAISE_ANY(STR_TO_MATCH)    RUN_OR_RAISE_TYPE(STR_TO_MATCH,ANY,STR_TO_MATCH)
#define RUN_OR_RAISE_CLASS(STR_TO_MATCH)  RUN_OR_RAISE_TYPE(STR_TO_MATCH,CLASS|CASE_INSENSITIVE|LITERAL,STR_TO_MATCH)
#define RUN_OR_RAISE_TITLE(STR_TO_MATCH,COMMAND)  RUN_OR_RAISE_TYPE(STR_TO_MATCH,TITLE,COMMAND)

#define RUN_OR_RAISE_TYPE(STR_TO_MATCH,TYPE,COMMAND_STR) \
    BIND_TO_RULE_FUNC(runOrRaise,CREATE_RULE(STR_TO_MATCH,TYPE,NULL),{0},CREATE_WILDCARD(SPAWN(COMMAND_STR)),{0})

#define SPAWN(COMMAND_STR)  BIND_TO_STR_FUNC(spawn,COMMAND_STR)

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





#define ADD_WINDOW_RULE(E,R) \
    insertHead(eventRules[E],memcpy(malloc(sizeof(Rules)),&((Rules)R), sizeof(Rules))); \
    assert((((Rules*)getValue(eventRules[E])))->onMatch.type!=UNSET); \
    COMPILE_RULE(((Rules*)getValue(eventRules[E])))


void onHiearchyChangeEvent();
void onDeviceEvent();
/**
 * Set initial window config
 * @param event
 */
void onConfigureRequestEvent();
void onVisibilityEvent();
void onCreateEvent();
void onDestroyEvent();
void onError();
void onExpose();

/**
 * Called when the a client application changes the window's state from unmapped to mapped.
 * @param context
 */
void onMapRequestEvent();
void onUnmapEvent();
void onEnterEvent();
void onFocusInEvent();
void onFocusOutEvent();
void onPropertyEvent();
void onClientMessage();

void detectMonitors();

void onStartup();
void clearAllRules();

/**
 * Called when an connection to the Xserver has been established
 */
void onXConnect();


#endif /* DEFAULT_RULES_H_ */
