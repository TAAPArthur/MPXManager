/**
 * @file wmfunctions.h
 * @brief Connect our internal structs to X
 *
 */

#ifndef MYWM_FUNCTIONS_H_
#define MYWM_FUNCTIONS_H_

#include <xcb/xcb.h>

#include "mywm-util.h"

#define SRC_INDICATION_OFFSET 12
typedef enum {
    /// no special properties
    NO_MASK = 0,
    /**The window's X size will equal the size of the monitor's viewport*/
    X_MAXIMIZED_MASK =      1<<0,
    /**The window's Y size will equal the size of the monitor's viewport*/
    Y_MAXIMIZED_MASK =      1<<1,
    /**The window's size will equal the size of the monitor (not viewport)     */
    FULLSCREEN_MASK =       1<<2,
    /**The window's size will equal the size of the screen (union of all monitors)*/
    ROOT_FULLSCREEN_MASK =  1<<3,
    ///Window will not be tiled
    NO_TILE_MASK =          1<<4,

    ///Window is effectively associated with its monitor instead of its workspace
    /// (it is moveded between workspaces to stay on its monitor
    STICKY_MASK =           1<<5,
    ///The window will be treated as unmapped until this mask is removed (iconic state)
    HIDDEN_MASK =        1<<6,


    /// Window can be externally resized (configure requests are allowed)
    EXTERNAL_RESIZE_MASK =  1<<8,
    /// Window can be externally moved (configure requests are allowed)
    EXTERNAL_MOVE_MASK =    1<<9,
    /// Window border size can be externally changed
    EXTERNAL_BORDER_MASK =    1<<10,
    /// Window cannot be externally raised/lowered (configure requests are blocked)
    EXTERNAL_RAISE_MASK =   1<<11,
    FLOATING_MASK =            EXTERNAL_RESIZE_MASK|EXTERNAL_MOVE_MASK|EXTERNAL_BORDER_MASK|EXTERNAL_RAISE_MASK,

    ///Allow client requests from older clients who don't specify what they are
    SRC_INDICATION_OTHER=   1<<SRC_INDICATION_OFFSET,
    ///Allow client requests from normal applications
    SRC_INDICATION_APP=     1<<(SRC_INDICATION_OFFSET+1),
    ///Allow client requests from pagers
    SRC_INDICATION_PAGER=   1<<(SRC_INDICATION_OFFSET+2),
    /// allow from any source
    SRC_ANY =                SRC_INDICATION_OTHER|SRC_INDICATION_APP|SRC_INDICATION_PAGER,


    /**The window can receive input focus*/
    INPUT_MASK =           1<<16,
    /**The WM will not forcibly set focus but request the application focus itself*/
    WM_TAKE_FOCUS_MASK =    1<<17,
    /**The WM will not forcibly set focus but request the application focus itself*/
    WM_DELETE_WINDOW_MASK = 1<<18,
    /**Used in conjuction with WM_DELETE_WINDOW_MASK to kill the window */
    WM_PING_MASK = 1<<19,


    ///Keeps track on the visibility state of the window
    PARTIALLY_VISIBLE =     1<<27,
    FULLY_VISIBLE =         1<<28 | PARTIALLY_VISIBLE,
    ///Inidicates the window is not withdrawn
    MAPABLE_MASK =           1<<29,
    ///the window is currently mapped
    MAPPED_MASK =           1<<30,

    /**Marks the window as urgent*/
    URGENT_MASK =           1<<31,

}WindowMasks;


/**
 * Window Manager name used to comply with EWMH
 */
#define WM_NAME "MPX Manger"


/**
 * @param winInfo
 * @return 1 iff the window is PARTIALLY_VISIBLE or VISIBLE
 */
int isWindowVisible(WindowInfo* winInfo);

/**
 *
 * @param winInfo the window whose map state to query
 * @return the window map state either UNMAPPED(0) or MAPPED(1)
 */
int isMappable(WindowInfo* winInfo);


/**
 * Determines if a window should be tiled given its mapState and masks
 * @param winInfo
 * @return true if the window should be tiled
 */
int isTileable(WindowInfo* winInfo);

/**
 * @param winInfo
 * @return 1 iff external resize requests should be granted
 */
int isExternallyResizable(WindowInfo* winInfo);
/**
 * @param winInfo
 * @return 1 iff external move requests should be granted
 */
int isExternallyMoveable(WindowInfo* winInfo);

/**
 *A window is actiable if it is MAPPABLE and not HIDDEN
 * @param winInfo
 * @return true if the window can receive focus
 */
int isActivatable(WindowInfo* winInfo);


/**
 *
 * @param win
 * @param mask
 * @return 1 iff the window containers the complete mask
 */
int hasMask(WindowInfo* win,int mask);
/**
 * Resets the user controlled state to the defaults
 * @param winInfo
 */
void resetUserMask(WindowInfo* winInfo);
/**
 * Adds the states give by mask to the window
 * @param winInfo
 * @param mask
 */
void addMask(WindowInfo*winInfo,int mask);
/**
 * Removes the states give by mask from the window
 * @param winInfo
 * @param mask
 */
void removeMask(WindowInfo*winInfo,int mask);
/**
 * Adds or removes the mask depending if the window already contains
 * the complete mask
 * @param winInfo
 * @param mask
 */
void toggleMask(WindowInfo*winInfo,int mask);


/**
 * Establishes a connection with the X server.
 * Creates and Xlib and xcb display and set global vars like ewmh, root and screen
 *
 * This method creates at least 1 master and monitor such that other mywm functions
 * can be safely used
 */
void connectToXserver();

/**
 * Sync EWMH global properties with our internal state
 */
void syncState();

/**
 * Queries the XServer for all top level windows and process all of them
 * that do not have override redirect
 */
void scan(xcb_window_t baseWindow);

/**
 * Sets the EWMH property to be the given window
 * @param win
 */
void setActiveWindowProperty(int win);

/**
 * Determines if and how a given window should be managed
 * @param winInfo
 */
void processNewWindow(WindowInfo* winInfo);

/**
 * Loads class and instance name for the given window
 * @param info
 */
void loadClassInfo(WindowInfo*info);
/**
 * Loads title for a given window
 * @param winInfo
 */
void loadTitleInfo(WindowInfo*winInfo);

/**
 * Loads type name and atom value for a given window
 * @param winInfo
 */
void loadWindowType(WindowInfo *winInfo);

/**
 * Loads grouptId, input and window state for a given window
 * @param winInfo
 */
void loadWindowHints(WindowInfo *winInfo);

/**
 * Load various window properties
 * @param winInfo
 * @see loadClassInfo(),loadTitleInfo(),loadWindowType(),loadWindowHints()
 */
void loadWindowProperties(WindowInfo *winInfo);


/**
 * Adds the window to our list of managed windows as a non-dock
 * and load any ewhm saved state
 * @param winInfo
 */
void registerWindow(WindowInfo*winInfo);


/**
 * Sets the window state based on a list of atoms
 * @param winInfo
 * @param atoms
 * @param numberOfAtoms
 * @param action
 */
void setWindowStateFromAtomInfo(WindowInfo*winInfo,const xcb_atom_t* atoms,int numberOfAtoms,int action);

void setXWindowStateFromMask(WindowInfo*winInfo);
/**
 * Reads EWMH desktop property to find the workspace the window should be long.
 * If the field is not present the active workspace is used.
 * If the field is greater than the number of workspaces, the last workspace is used
 * @param win
 * @return the workspace the window should be placed in by default
 */
int getSavedWorkspaceIndex(xcb_window_t win);


/**
 * Sets the mapped state to UNMAPPED(0) or MAPPED(1)
 * @param win the window whose state is being modified
 * @param state the new state
 * @return  true if the state actuall changed
 */
void updateMapState(int id,int state);


/**
 * Tiles the specified workspace.
 * If the active layout is NULL then this is a no-op.
 * First the windows in the NORMAL_LAYER are tiled. If the layoutFunction is NULL
 * or the conditionFunction is not true then the windows are left as in.
 * Afterwards the remaining layers are tiled in ascending order
 * @param index
 */
void tileWorkspace(int index);

/**
 * Sets the border color for the given window
 * @param win   the window in question
 * @param color the new window border color
 * @return 1 iff no error was deteced
 */
void setBorderColor(xcb_window_t win,unsigned int color);


/**
 * Switch to window's worksspace, raise and focus the window
 * @param winInfo
 * @return the id of the activated window or 0 if no window was activated
 */
int activateWindow(WindowInfo* winInfo);
/**
 * Focuses the given window
 * @param win   the window to focus
 * @return 1 iff no error was detected
 */
int focusWindow(int win);

int focusWindowInfo(WindowInfo*winInfo);
/**
 * Raises the given window
 * @param win   the window to raise
 * @return 1 iff no error was detected
 */
int raiseWindow(xcb_window_t win);

/**
 * Maps the window specified by id
 * @param id
 * @return if the attempt was successful
 */
int attemptToMapWindow(int id);

int deleteWindow(xcb_window_t winToRemove);

/**
 * Forks and executes command
 * @param command the command to execute
 */
//void runCommand(char* command);

/**
 * Returns the max dims allowed for this window based on its mask
 * @param m the monitor the window is one
 * @param winInfo   the window
 * @param height    whether to check the height(1) or the width(0)
 * @return the max dimension or 0 if the window has not relevant mask
 */
int getMaxDimensionForWindow(Monitor*m,WindowInfo*winInfo,int height);

/**
 * Returns User set geometry that will override that generated when tiling
 * @param winInfo
 * @see WindowInfo->config
 */
short*getConfig(WindowInfo*winInfo);
/**
 * Sets window geometry that will replace that which is generated when tiling
 * @param winInfo
 * @param geometry
 */
void setConfig(WindowInfo*winInfo,short*geometry);
/**
 * Get the last recorded geometry of the window
 * @param winInfo
 */
short*getGeometry(WindowInfo*winInfo);
/**
 * Set the last recorded geometry of the window
 * @param winInfo
 * @param geometry
 */
void setGeometry(WindowInfo*winInfo,short*geometry);

/**
 * Checks to see if the window has SRC* masks set that will allow. If not client requests with such a source will be ignored
 * @param winInfo
 * @param source
 * @return
 */
int allowRequestFromSource(WindowInfo* winInfo,int source);
/**
 * Applies the gravity to pos
 * @param win if gravity is 0 look up the gravity from the ICCCM normal hints on this window
 * @param pos array of values x,y,w,h,border
 * @param gravity the gravity to use
 */
void applyGravity(int win,short pos[5],int gravity);

/**
 * Assigns names to the first n workspaces
 * @param names
 * @param numberOfNames
 */
void setWorkspaceNames(char*names[],int numberOfNames);

/**
 * Returns the first workspace index whose names matches name
 * @param name
 * @return
 */
int getIndexFromName(char*name);

void activateWorkspace(int workspaceIndex);

void moveWindowToWorkspace(WindowInfo* winInfo,int destIndex);
void moveWindowToWorkspaceAtLayer(WindowInfo *winInfo,int destIndex,int layer);

void toggleShowDesktop();


void switchToWorkspace(int workspaceIndex);





void processConfigureRequest(int win,short values[5],xcb_window_t sibling,int stackMode,int mask);

/**
 * Do everything needed to comply when EWMH.
 * Creates a functionless window with set all the Atoms we support, clear any
 * unsporrted values from the root window
 */
void broadcastEWMHCompilence();

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

/**
 * Send a kill signal to the client with the window
 * @param win
 */
void killWindow(xcb_window_t win);

/**
 * Sends a WM_DELETE_WINDOW message or sends a kill requests
 * @param winInfo
 * @see killWindow();
 */
void killWindowInfo(WindowInfo* winInfo);
/**
 * Check to see if we are showing the desktop.
 * When showing the desktop all windows in the workspace are hidden except for windows in the Desktop Layer
 * @param index
 * @return 1 if we are currently showing the desktop
 */
int isShowingDesktop(int index);
/**
 * show or hide desktop depending on values
 * @param index workpsace index
 * @param value 1 or 0
 */
void setShowingDesktop(int index,int value);
/**
 * Toggle showing of the desktop
 * @see isShowingDesktop()
 */
void toggleShowDesktop();


void floatWindow(WindowInfo* win);
void sinkWindow(WindowInfo* win);
#endif
