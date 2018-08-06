/**
 * @file functions.h
 *
 * @brief User functions that are that
 * users may add to their config that are not used elsewhere
 */


#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_


#include "bindings.h"

#define RUN_RAISE_ANY(STR_TO_MATCH)    RUN_OR_RAISE_TYPE(STR_TO_MATCH,ANY,STR_TO_MATCH)
#define RUN_OR_RAISE_CLASS(STR_TO_MATCH)  RUN_OR_RAISE_TYPE(STR_TO_MATCH,CLASS|CASE_INSENSITIVE|LITERAL,STR_TO_MATCH)
#define RUN_OR_RAISE_TITLE(STR_TO_MATCH,COMMAND)  RUN_OR_RAISE_TYPE(STR_TO_MATCH,TITLE,COMMAND)


/**
 * Used to create a binding to a function that returns void and takes a
 * list of Rules as an argument.
 *
 * @param F the function to bind to
 */
#define BIND_TO_RULE_FUNC(F,...) BIND(F,(Rules*)(Rules[]){__VA_ARGS__})

#define RUN_OR_RAISE_TYPE(STR_TO_MATCH,TYPE,COMMAND_STR) \
    AND(BIND(runOrRaise,CREATE_RULE(STR_TO_MATCH,TYPE,NULL)), SPAWN(COMMAND_STR))

#define SPAWN(COMMAND_STR)  BIND_FUNC(spawn,COMMAND_STR)

#define WORKSPACE_OPERATION(K,N) \
    {  Mod4Mask,             K, BIND_FUNC(moveToWorkspace,N)}, \
    {  Mod4Mask|ShiftMask,   K, BIND_FUNC(sendToWorkspace,N)}/*, \
    {  Mod4Mask|ControlMask,   K, BIND_TO_INT_FUNC(cloneToWorkspace,N)}, \
    {  Mod4Mask|Mod1Mask,   K, BIND_TO_INT_FUNC(swapWithWorkspace,N)}, \
    {  Mod4Mask|ControlMask|Mod1Mask,   K, BIND_TO_INT_FUNC(stealFromWorkspace,N)}, \
    {  Mod4Mask|Mod1Mask|ShiftMask,   K, BIND_TO_INT_FUNC(giveToWorkspace,N)},\
    {  Mod4Mask|ControlMask|Mod1Mask|ShiftMask,   K, BIND_TO_INT_FUNC(tradeWithWorkspace,N)}*/

#define STACK_OPERATION(UP,DOWN,LEFT,RIGHT) \
    {  Mod4Mask,             UP, BIND_FUNC(shiftPosition,1)}, \
    {  Mod4Mask,             DOWN, BIND_FUNC(shiftPosition,-1)}, \
    {  Mod4Mask,             LEFT, BIND_FUNC(shiftFocus,1)}, \
    {  Mod4Mask,             RIGHT, BIND_FUNC(shiftFocus,-1)}/*, \
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






#define CYCLE(D,M1,K1,M2,K2)    {M1,K1,AUTO_CHAIN({M1,K1,BIND(cycleWindows,D)},END_CHAIN(M2,K2,BIND(endCycleWindows)))}
/**
 * Call to stop cycling windows
 *
 * Unfreezes the master window stack
 */
void endCycleWindows(void);
/**
 * Freezes and cycle through windows in the active master's stack
 * @param delta
 */
void cycleWindows( int delta);



//////Run or Raise code

/**
 *Attempts to find a that rule matches
 * @param rule
 * @param numberOfRules
 * @return 1 if a matching window was found
 */
int findAndRaise(Rule*rule);
int findAndRaiseLazy(Rule*rule);
Node* findWindow(Rule*rule,Node*searchList,Node*ignoreList);
/**
 * Sends a kill signal to the focused window
 */
void killFocusedWindow();

/**
 * Forks and runs command in SHELL
 * @param command
 */
void spawn(char* command);



/**
 * Switch to workspace
 * @param context
 * @param index
 */
void moveToWorkspace(int index);
/**
 * Send active window to workspace
 * @param context
 * @param index
 */
void sendToWorkspace(int index);
/**
 * Adds the window to new workspace without removing it
 */
void cloneToWorkspace(int index);


void swapWithWorkspace(int index);
void stealFromWorkspace(int index);
void giveToWorkspace(int index);
void tradeWithWorkspace(int index);

void shiftPosition(int index);
void shiftFocus(int index);
void popMin();
void pushMin();
void sendToNextWorkNonEmptySpace(int index);

/**
 * Swap monitors with the active workspace
 * @param index the index of the workspace to swap with
 */
void swapWithNextMonitor(int index);
void sendToNextMonitor(int index);

/**
 * Shifts the focused window to the top of the window stack
 */
void shiftTop();
/**
 * Swaps the focused window with the top of the window stack
 */
void swapWithTop();

/**
 * Cycle through the layouts of the active workspace
 * @param index either UP or DOWN
 */
void cycleLayouts(int dir);

/**
 * Apply this external layout to the workspace if it is not already the active layout
 * @param layout
 */
void toggleLayout(Layout* layout);

/**
 * Manually retile the active workspace
 */
void retile();

void focusBottom();
void focusTop();
void focusBottom();
void sendtoBottom(int index);
void sendFromBottomToTop();


int floatActiveWindow();
int sinkFocusedWindow();

int moveToLayer(int index);
int sendWindowToWorkspaceByName(char*,int window);





#endif
