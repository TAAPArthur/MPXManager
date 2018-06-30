/**
 * @file layouts.h
 *
 * @brief User functions that are that
 * users may add to their config that are not used elsewhere
 */

#include <xcb/xcb.h>


#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_
#include "mywm-structs.h"
#include "bindings.h"

void endCycleWindows();
void cycleWindowsForward();
void cycleWindowsReverse();
void cycleWindows( int delta);


//////Run or Raise code

/**
 *
 * @param rule
 * @param numberOfRules
 * @return 1 if a matching window was found or 0 if the command was run
 */
int runOrRaise(Rules*rule);
Node* findWindow(Rules*rule,Node*searchList,Node*ignoreList);
void killFocusedWindow();

void spawn(char* target);
int spawnAsMaster(char* target);
int spawnWithArgs(char* target);
int spawnWithArgsAsMaster(char* target);


void toggleStateForActiveWindow(int mask);


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
void popMin(int index);
void pushMin(int index);
void sendToNextWorkNonEmptySpace(int index);
void swapWithNextMonitor(int index);
void sendToNextMonitor(int index);

void shiftTop();
void swapWithTop();
void cycleLayouts(int index);

void activateLastClickedWindow();
void focusBottom();
void focusTop();
void focusBottom();
void sendtoBottom(int index);
void sendFromBottomToTop();


int floatActiveWindow();
int sinkActiveWindow();
int floatWindow(WindowInfo* win);
int sinkWindow(WindowInfo* win);
int moveToLayer(int index);
int sendWindowToWorkspaceByName(char*,int window);

void cycleLayouts(int dir);
void setLayout(Layout* layout);
#endif
