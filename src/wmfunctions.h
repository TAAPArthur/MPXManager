/**
 * @file wmfunctions.c
 * @brief Connect our internal structs to X
 *
 */

#ifndef MYWM_FUNCTIONS_H_
#define MYWM_FUNCTIONS_H_

#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI.h>
#include <xcb/xinput.h>

#include "bindings.h"
#include "mywm-structs.h"
#include "logger.h"
#include "mywm-util.h"


/**
 * Window Manager name used to comply with EWMH
 */
#define WM_NAME "My Window Manger"

#define LEN(X) (sizeof X / sizeof X[0])


int isShuttingDown();


/**
 * Tile the layouts according to layout
 * @param workspace the workspace to tile
 * @param layout the layout to use
 */
void applyLayout(Workspace*workspace,Layout* layout);
/**
 * Retile all visible workspaces with their active layout
 */
void tileWindows();

/**
 * Sets the border color for the given window
 * @param win   the window in question
 * @param color the new window border color
 * @return 1 iff no error was deteced
 */
int setBorderColor(xcb_window_t win,unsigned int color);


/**
 * Switch to window's worksspace, raise and focus the window
 * @param id
 * @return
 */
int activateWindow(WindowInfo* winInfo);
/**
 * Focuses the given window
 * @param win   the window to focus
 * @return 1 iff no error was detected
 */
int focusWindow(xcb_window_t win);
/**
 * Raises the given window
 * @param win   the window to raise
 * @return 1 iff no error was detected
 */
int raiseWindow(xcb_window_t win);

int setUnmapped(xcb_window_t win);
int deleteWindow(xcb_window_t winToRemove);

/**
 * Forks and executes command
 * @param command the command to execute
 */
//void runCommand(char* command);




void setWorkspaceNames(char*names[],int numberOfNames);
void activateWorkspace(int workspaceIndex);

void removeWindowFromAllWorkspaces(WindowInfo* winInfo);
void moveWindowToWorkspace(WindowInfo* winInfo,int destIndex);
void moveWindowToWorkspaceAtLayer(WindowInfo *winInfo,int destIndex,int layer);

void toggleShowDesktop();


void switchToWorkspace(int workspaceIndex);

void scan();

/**
 * Determines if and how a given window should be managed
 * @param winInfo
 */
void processNewWindow(WindowInfo* winInfo);
void avoidStruct(WindowInfo*info);

/**
 * Sets the window state based on a list of attoms
 * @param winInfo
 * @param atoms
 * @param numberOfAtoms
 * @param action
 */
void setWindowStateFromAtomInfo(WindowInfo*winInfo,xcb_atom_t* atoms,int numberOfAtoms,int action);

/**
 * Adds the window to our list of managed windows as a non-dock
 * and load any ewhm saved state
 * @param winInfo
 */
void registerWindow(WindowInfo*winInfo);

void configureWindow(Monitor*m,WindowInfo* winInfo,short values[CONFIG_LEN]);
void processConfigureRequest(Window win,short values[5],xcb_window_t sibling,int stackMode,int mask);

/**
 * Do everything needed to comply when EWMH.
 * Creates a functionless window with set all the Atoms we support, clear any
 * unsporrted values from the root window
 */
void broadcastEWMHCompilence();

/**
 * Send a kill signal to the client with the window
 * @param win
 */
void killWindow(xcb_window_t win);

//extern const int bindingMasks[4];


/**
 * moves a window to a specified layer for all workspaces it is in
 * @param win
 * @param layer
 */
void moveWindowToLayerForAllWorkspaces(WindowInfo* win,int layer);
/**
 * MOves a window to the specifed layer in a given workspace
 * @param win
 * @param workspaceIndex
 * @param layer
 */
void moveWindowToLayer(WindowInfo* win,int workspaceIndex,int layer);

int* getWindowDimensions(WindowInfo*winInfo);

/**
 * Clears the dirty bit
 */
void setClean();
/**
 * marks the context as dirty and in need of a retile
 */
void setDirty();
/**
 * Checks to see if any visible window stack has been modified
 * @return true if a visible window stack has been modified
 */
int isDirty();

#endif
