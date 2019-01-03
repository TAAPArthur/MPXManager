/**
 * @file wmfunctions.h
 * @brief Connect our internal structs to X
 *
 */

#ifndef MYWM_FUNCTIONS_H_
#define MYWM_FUNCTIONS_H_

#include <xcb/xcb.h>

#include "mywm-util.h"
/**
 * Offset for SRC_INDICATION_* masks
 * These masks specify which type of external sources are allowed to modify the window
 * Adding [0,2] and rasing 2 to that power yeilds OTHER,APP and PAGER SRC_INDICATION_ masks respectivly
 */
#define SRC_INDICATION_OFFSET 12
/**
 * Various flags that detail how a window should be treated.
 *
 */
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
    /// Window is floating (ie is not tiled and can be freely moved like by external programs/mouse
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
 * Returns true if the user can interact (ie focus,type etc)  with the window
 * For this to be true the window would have to be mapped and not hidden
 */
int isInteractable(WindowInfo* winInfo);

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
 *Returns the full mask of the given window
 */
int getMask(WindowInfo*winInfo);

/**
 *
 * @param win
 * @param mask
 * @return 1 iff the window containers the complete mask
 */
int hasMask(WindowInfo* win,int mask);
/**
 * Returns the subset of mask that refers to user set masks
 */
int getUserMask(WindowInfo*winInfo);

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
 * @return 1 iff the window wasn't ignored
 */
int processNewWindow(WindowInfo* winInfo);

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
 * Sets the WM_STATE from the window masks
 */
void setXWindowStateFromMask(WindowInfo*winInfo);
/**
 * Reads the WM_STATE fields from the given window and sets the window mask to be consistent with the state
 * If the WM_STATE cannot be read, then nothing is done
 * @see setWindowStateFromAtomInfo
 */
void loadSavedAtomState(WindowInfo*winInfo);

/**
 * Sets the window state based on a list of atoms
 * @param winInfo the window whose mask will be modifed
 * @param atoms the list of atoms to add,remove or toggle depending on ACTION
 * @param numberOfAtoms the number of atoms
 * @param action wether to add,remove or toggle atoms. For toggle, if all of the corrosponding masks for the list of atoms is set, then they all will be removed else they all will be added
 * @see XCB_EWMH_WM_STATE_ADD, XCB_EWMH_WM_STATE_REMOVE, XCB_EWMH_WM_STATE_TOGGLE
 */
void setWindowStateFromAtomInfo(WindowInfo*winInfo,const xcb_atom_t* atoms,int numberOfAtoms,int action);

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
 * @param id the window whose state is being modified
 * @param state the new state
 * @return  true if the state actuall changed
 */
void updateMapState(int id,int state);

/**
 * Updates our view of the focused window
 * @param winInfo
 * @see onWindowFocus()
 */
void updateFocusState(WindowInfo*winInfo);

/**
 * Tiles the specified workspace.
 * First the windows in the NORMAL_LAYER are tiled according to the active layout's layoutFunction 
 * (if sett) or the conditionFunction is not true then the windows are left as in.
 * Afterwards the remaining layers are tiled in ascending order
 * @param index the index of the workspace to tile
 * @see tileLowerLayers, tileUpperLayers
 */
void tileWorkspace(int index);
/**
 * Tile the windows in the layers below NORMAL_LAYER in DESC order
 */
void tileLowerLayers(Workspace*workspace);
/**
 *Tile the windows in the layers above NORMAL_LAYER in AESC order
 */
void tileUpperLayers(Workspace*workspace,int startingLayer);

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
/**
 * Focus the given window
 * This method is diffrent from focusWindow in that it allows diffrent protocals for 
 * focusing the window based on window masks
 */
int focusWindowInfo(WindowInfo*winInfo);
/**
 * Raises the given window
 * @param win   the window to raise
 * @return 1 iff no error was detected
 */
int raiseWindow(xcb_window_t win);
/**
 * Raise the given window within its onw layer
 */
int raiseWindowInfo(WindowInfo* winInfo);

/**
 * Maps the window specified by id
 * @param id
 * @return if the attempt was successful
 */
int attemptToMapWindow(int id);
/**
 * Removes a window from our records and updates EWMH client list if the window was present
 * @return 1 iff the window was already in our records
 */
int deleteWindow(xcb_window_t winToRemove);


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

/**
* Returns the name of the workspace at the specifed index
* @param index 
*
* @return the name of the workspace
*/
char*getWorkspaceName(int index);
/**
 * Make the active workspace the one designated by workspaceIndex
 * For the workspace to become active,it must become visible. For this to be true it must 
 * swap monitor with an already visible workspace
 * Upon switching, the focus for all masters is updated to be the last focused window out of 
 * all visible windows
 */
void switchToWorkspace(int workspaceIndex);

/**
 * Moves a window to the workspace given by destIndex at the NORMAL_LAYER
 */
void moveWindowToWorkspace(WindowInfo* winInfo,int destIndex);
/**
 *
 * Moves a window to the workspace given by destIndex at layer 
 */
void moveWindowToWorkspaceAtLayer(WindowInfo *winInfo,int destIndex,int layer);

/**
 * Filters the configure request to allowed actions then configures the window
 * @param win
 * @param values
 * @param sibling 
 * @param stackMode
 * @param configMask
 * @see xcb_configure_window
 */
void processConfigureRequest(int win,short values[5],xcb_window_t sibling,int stackMode,int configMask);

/**
 * Do everything needed to comply when EWMH.
 * Creates a functionless window with set all the Atoms we support, clear any
 * unsporrted values from the root window
 */
void broadcastEWMHCompilence();

/**
 * Moves a window to the specifed layer in the same workspace
 * @param win
 * @param layer
 */
void moveWindowToLayer(WindowInfo* win,int layer);

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


#endif
