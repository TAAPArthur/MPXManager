#ifndef WINDOWS_H_
#define WINDOWS_H_

#include "mywm-util.h"
#include <xcb/xcb.h>
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
    X_MAXIMIZED_MASK =      1 << 0,
    /**The window's Y size will equal the size of the monitor's viewport*/
    Y_MAXIMIZED_MASK =      1 << 1,
    /**The window's size will equal the size of the monitor (not viewport)     */
    FULLSCREEN_MASK =       1 << 2,
    /**The window's size will equal the size of the screen (union of all monitors)*/
    ROOT_FULLSCREEN_MASK =  1 << 3,

    ///windows will be below normal windows unless ABOVE_MASK is set is set
    BELOW_MASK =            1 << 4,
    ///windows will be above other windows
    ABOVE_MASK =            1 << 5,
    ///Window will not be tiled
    NO_TILE_MASK =          1 << 6,

    /// or of all masks that will cause the layout function to skip a window
    ALL_NO_TILE_MASKS = FULLSCREEN_MASK | ROOT_FULLSCREEN_MASK | BELOW_MASK | ABOVE_MASK | NO_TILE_MASK,

    /**The window's size will equal the size of the monitor (not viewport)     */
    TRUE_FULLSCREEN_MASK =       FULLSCREEN_MASK | ABOVE_MASK,
    /**The window's size will equal the size of the screen (union of all monitors)*/
    TRUE_ROOT_FULLSCREEN_MASK =  ROOT_FULLSCREEN_MASK | ABOVE_MASK,

    ///Window is effectively associated with its monitor instead of its workspace
    /// (it is moveded between workspaces to stay on its monitor
    STICKY_MASK =           ABOVE_MASK | NO_TILE_MASK,
    ///The window will be treated as unmapped until this mask is removed (iconic state)
    HIDDEN_MASK =        1 << 7,

    USER_MASKS = ((1 << 8) - 1),

    /// Window can be externally resized (configure requests are allowed)
    EXTERNAL_RESIZE_MASK =  1 << 8,
    /// Window can be externally moved (configure requests are allowed)
    EXTERNAL_MOVE_MASK =    1 << 9,
    /// Window border size can be externally changed
    EXTERNAL_BORDER_MASK =    1 << 10,
    /// Window cannot be externally raised/lowered (configure requests are blocked)
    EXTERNAL_RAISE_MASK =   1 << 11,
    /// Window is floating (ie is not tiled and can be freely moved like by external programs/mouse
    EXTERNAL_CONFIGURABLE_MASK =            EXTERNAL_RESIZE_MASK | EXTERNAL_MOVE_MASK | EXTERNAL_BORDER_MASK | EXTERNAL_RAISE_MASK,
    /// Window is floating (ie is not tiled and can be freely moved like by external programs/mouse) and is above other windows
    FLOATING_MASK =            ABOVE_MASK | EXTERNAL_CONFIGURABLE_MASK,

    ///Allow client requests from older clients who don't specify what they are
    SRC_INDICATION_OTHER =   1 << SRC_INDICATION_OFFSET,
    ///Allow client requests from normal applications
    SRC_INDICATION_APP =     1 << (SRC_INDICATION_OFFSET + 1),
    ///Allow client requests from pagers
    SRC_INDICATION_PAGER =   1 << (SRC_INDICATION_OFFSET + 2),
    /// allow from any source
    SRC_ANY =                SRC_INDICATION_OTHER | SRC_INDICATION_APP | SRC_INDICATION_PAGER,


    /**The window can receive input focus*/
    INPUT_MASK =           1 << 16,
    /**The WM will not forcibly set focus but request the application focus itself*/
    WM_TAKE_FOCUS_MASK =    1 << 17,
    /**The WM will not forcibly set focus but request the application focus itself*/
    WM_DELETE_WINDOW_MASK = 1 << 18,
    /**Used in conjuction with WM_DELETE_WINDOW_MASK to kill the window */
    WM_PING_MASK = 1 << 19,

    ALWAYS_ON_TOP = 1 << 20,

    ///Keeps track on the visibility state of the window
    PARTIALLY_VISIBLE =     1 << 27,
    ///Keeps track on the visibility state of the window
    FULLY_VISIBLE =         1 << 28 | PARTIALLY_VISIBLE,
    ///Inidicates the window is not withdrawn
    MAPABLE_MASK =           1 << 29,
    ///the window is currently mapped
    MAPPED_MASK =           1 << 30,

    /**Marks the window as urgent*/
    URGENT_MASK =           1 << 31,

} WindowMasks;

/**
 *Returns the full mask of the given window
 */
int getMask(WindowInfo* winInfo);
/**
 *
 * @param win
 * @param mask
 * @return the intersection of mask and the window mask
 */
int hasPartOfMask(WindowInfo* winInfo, int mask);

/**
 *
 * @param win
 * @param mask
 * @return 1 iff the window containers the complete mask
 */
int hasMask(WindowInfo* win, int mask);
/**
 * Returns the subset of mask that refers to user set masks
 */
int getUserMask(WindowInfo* winInfo);

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
void addMask(WindowInfo* winInfo, int mask);
/**
 * Removes the states give by mask from the window
 * @param winInfo
 * @param mask
 */
void removeMask(WindowInfo* winInfo, int mask);
/**
 * Adds or removes the mask depending if the window already contains
 * the complete mask
 * @param winInfo
 * @param mask
 */
void toggleMask(WindowInfo* winInfo, int mask);

/**
 * Loads class and instance name for the given window
 * @param info
 */
void loadClassInfo(WindowInfo* info);
/**
 * Loads title for a given window
 * @param winInfo
 */
void loadTitleInfo(WindowInfo* winInfo);

/**
 * Loads type name and atom value for a given window
 * @param winInfo
 */
void loadWindowType(WindowInfo* winInfo);

/**
 * Loads grouptId, input and window state for a given window
 * @param winInfo
 */
void loadWindowHints(WindowInfo* winInfo);

/**
 * Load various window properties
 * @param winInfo
 * @see loadClassInfo(),loadTitleInfo(),loadWindowType(),loadWindowHints()
 */
void loadWindowProperties(WindowInfo* winInfo);

/**
 * Sets the WM_STATE from the window masks
 */
void setXWindowStateFromMask(WindowInfo* winInfo);
/**
 * Reads the WM_STATE fields from the given window and sets the window mask to be consistent with the state
 * If the WM_STATE cannot be read, then nothing is done
 * @see setWindowStateFromAtomInfo
 */
void loadSavedAtomState(WindowInfo* winInfo);

/**
 * Sets the window state based on a list of atoms
 * @param winInfo the window whose mask will be modifed
 * @param atoms the list of atoms to add,remove or toggle depending on ACTION
 * @param numberOfAtoms the number of atoms
 * @param action wether to add,remove or toggle atoms. For toggle, if all of the corrosponding masks for the list of atoms is set, then they all will be removed else they all will be added
 * @see XCB_EWMH_WM_STATE_ADD, XCB_EWMH_WM_STATE_REMOVE, XCB_EWMH_WM_STATE_TOGGLE
 */
void setWindowStateFromAtomInfo(WindowInfo* winInfo, const xcb_atom_t* atoms, int numberOfAtoms, int action);

/**
 * Determines if and how a given window should be managed
 * @param winInfo
 * @return 1 iff the window wasn't ignored
 */
int processNewWindow(WindowInfo* winInfo);



/**
 * Adds the window to our list of managed windows as a non-dock
 * and load any ewhm saved state
 * @param winInfo
 */
void registerWindow(WindowInfo* winInfo);

/**
 * Returns User set geometry that will override that generated when tiling
 * @param winInfo
 * @see WindowInfo->config
 */
short* getConfig(WindowInfo* winInfo);
/**
 * Sets window geometry that will replace that which is generated when tiling
 * @param winInfo
 * @param geometry
 */
void setConfig(WindowInfo* winInfo, short* geometry);
/**
 * Get the last recorded geometry of the window
 * @param winInfo
 */
short* getGeometry(WindowInfo* winInfo);
/**
 * Set the last recorded geometry of the window
 * @param winInfo
 * @param geometry
 */
void setGeometry(WindowInfo* winInfo, short* geometry);
void lockWindow(WindowInfo* winInfo);
void unlockWindow(WindowInfo* winInfo);
/**
 * Queries the XServer for all top level windows and process all of them
 * that do not have override redirect
 */
void scan(xcb_window_t baseWindow);


/**
 * @param workspace
 * @return the windows stack of the workspace
 */
ArrayList* getWindowStack(Workspace* workspace);



int getWorkspaceIndexOfWindow(WindowInfo* winInfo);
Workspace* getWorkspaceOfWindow(WindowInfo* winInfo);
ArrayList* getWindowStackOfWindow(WindowInfo* winInfo);

/**
 * Creates a pointer to a WindowInfo object and sets its id to id
 *
 * The workspaceIndex of the window is set to NO_WORKSPACE
 * @param id    the id of the WindowInfo object
 * @return  pointer to the newly created object
 */
WindowInfo* createWindowInfo(unsigned int id);

/**
 * Clones a WindowInfo object and sets its id to id
 *
 * @param id    the id of the WindowInfo object
 * @param winInfo    the window to clone
 * @return  pointer to the newly created object
 */
WindowInfo* cloneWindowInfo(unsigned int id, WindowInfo* winInfo);

/**
 * Returns a struct with stored metadata on the given window
 * @param win
 * @return pointer to struct with info on the given window
 */
WindowInfo* getWindowInfo(unsigned int win);


/**
 * @return a list of windows with the most recently used windows first
 */
ArrayList* getAllWindows();




/**
 * To be called when a master focuses a given window.
 *
 * If the window is not in the window list, nothing is done
 * else
 * The window is added to the active master's stack
 * if the master is not frozen, then the window is added/shifted to the head of the master stack
 * The window is shifted the head of the global window list
 * The master is promoted to the head of the master stack
 *
 * Note that if a new window is focused, it won't be added to the master stack if it is
 * frozen
 * @param win
 */
void onWindowFocus(unsigned int win);







/**
 * Removes window from context and master(s)/workspace lists.
 * The Nodes containing the window and the windows value are freeded also
 * @param winToRemove
 */
int removeWindow(unsigned int winToRemove);
/**
 * Frees winInfo and interal structs
 * @param winInfo
 * @see removeWindow()
 */
void deleteWindowInfo(WindowInfo* winInfo);


/**
 * Adds a window to the list of windows in context iff it
 * isn't already in the list
 * Note that the creted window is not added to a master window list or
 * a workspace
 * @param wInfo    instance to add
 * @return 1 iff this pointer was added
 */
int addWindowInfo(WindowInfo* wInfo);
void markAsDock(WindowInfo* winInfo);

/**
 * Returns a list of clone windows for the given window
 * @param winInfo
 * @return
 */
ArrayList* getClonesOfWindow(WindowInfo* winInfo);

#endif
