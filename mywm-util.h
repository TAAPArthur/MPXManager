/**
 * @file mywm-util.h
 *
 * @brief Calls to these function need
 * do not need a corresponding X11 function
 */
#ifndef MYWM_XUTIL
#define MYWM_XUTIL


#include "util.h"
#include "mywm-structs.h"

extern Layout (DEFAULT_LAYOUTS[]);
extern int NUMBER_OF_DEFAULT_LAYOUTS;
/**
 * @return the current time in seconds
 */
unsigned int getTime ();

void destroyContext();
/**
 * Initializes a context object to default values.
 *
 * All Node points are set to point to an empty node.
 * Workspaces is set to point to the head of an array with size==
 * numberOfWorkspaces
 * @param numberOfWorkspaces    the number of workspaces
 */
void createContext(int numberOfWorkspaces);

/**
 * Creates a pointer to a WindowInfo object and sets its id to id
 * @param id    the id of the WindowInfo object
 * @return  pointer to the newly created object
 */
WindowInfo* createWindowInfo(Window id);








/**
 *
 * @return the number of workspaces
 * @see createContext
 */
int getNumberOfWorkspaces();

/**
 *Set the last event received.
 * @param event the last event received
 * @see getLastEvent
 */
void setLastEvent(void* event);
/**
 * Retries the last event received
 * @see getLastEvent
 */
void* getLastEvent();





/**
 * Creates and adds a given master with id = keyboardMasterId
 * if a master with this id does not already exist.

 * @param keyboardMasterId  the id of the master device.
 * This is expected to be the id of a master keyboard, but any int will work
 * @return 1 iff a new master was inserted; 0 o.w.
 */
int addMaster(unsigned int keyboardMasterId);
/**
 *
 * @return a list of all master devices
 */
Node* getAllMasters();
/**
 * Removes the master with the specifed id
 * @param id    the id to remove
 * @return 1 iff a node was removed 0 o.w.
 */
int removeMaster(unsigned int id);


/**
 * @return  whether of not the master window stack will be updated on focus change
*/
int isFocusStackFrozen();
/**
 * If value, the master window stack will not be updated of focuse change
 * @param value whether the focus stack is frozen or not
*/
void setFocusStackFrozen(int value);

/**
 * @return the id of the last window clicked for the active master
 * @see setLastWindowClicked
 */
int getLastWindowClicked();
/**
 * Sets the value of the last window clicked for the active master
 * @param value the value of a window
 * @see getLastWindowClicked
 */
void setLastWindowClicked(int value);

/**
 *
 * @return Returns a list of windows that have been put in the cache
*/
Node*getWindowCache();
/**
 * Clears the window cache for the active master
 */
void clearWindowCache();
/**
 * Adds a window to the cache if not already present
 * @param targetWindow  the window to add
 */
int updateWindowCache(WindowInfo*winInfo);

/**
 * Returns the top window in the stack relative to given master.
 * @return
 */
Node* getMasterWindowStack();

/**
 * Get next/prev windows in master's stacking order
 * @param dir wheter to get teh next (>0) or the prev (<=0) window
 * @return the next or previous window depending on dir
 */
Node* getNextWindowInFocusHistory(int dir);

/**
 * Get the node containing the window master is
 * currently focused on. This should never be null
 * @param master
 * @return the currently focused window for master
 */
Node* getFocusedWindowByMaster(Master*master);
/**
 * Get the node containing the window the master is
 * currently focused on. This should never be null
 * @return the currently focused window for the active master
 */
Node* getFocusedWindow();
/**
 *
 * @param m
 * @return the time the master focused the window
 */
int getFocusedTime(Master*m);







/**
 * Returns a struct with stored metadata on the given window
 * @param win
 * @return pointer to struct with info on the given window
 */
WindowInfo* getWindowInfo(unsigned int win);
/**
 * Retunrs list of windows in the active workspace
 * @param context
 * @return
 */
Node* getWindowsOfActiveMaster();

/**
 * @return a list of windows with the most recently used windows first
 */
Node*getAllWindows();


Node* getActiveWindowsAtLayer(int layer);














/**
 * Adds a window to the list of windows in context iff it
 * isn't already in the list
 * Note that the creted window is not added to a master window list or
 * a workspace
 * @param wInfo    instance to add
 * @return 1 iff this pointer was added
 */
int addWindowInfo(WindowInfo* wInfo);

/**
 * Add a dock
 * @param info the dock to add
 * @return true if this dock was added
 */
int addDock(WindowInfo* info);



/**
 * To be called when a master focuses a given window.
 * The window is added to
 * the active master's stack
 * The active workspace's stack
 * And the master is promoted to the head of the master stack
 * @param context
 * @param windowInfo
 */
void onWindowFocus(int win);


/**
 *
 * @return The master device currently interacting with the wm
 * @see setActiveMasterNodeById
 */
Master*getActiveMaster();
/**
 * The active master should be set whenever the user interacts with the
 * wm (key/mouse  binding, mouse press etc)
 * @param listOfMasters
 * @param keyboardMasterId
 */
void setActiveMasterNodeById(Node*node);
/**
 * @brief returns the master node with id == keyboardId
 * @param context
 * @param id
 * @return
 */
Node*getMasterNodeById(int keyboardID);
/**
 * Return the first non-special workspace that a window is in
 * starting from the least recently focused window
 * @param context
 * @return
 */
Workspace* getActiveWorkspace();


/**
 *
 * @return the active workspace index
 */
int getActiveWorkspaceIndex();
/**
 *
 * @param index
 */
void setActiveWorkspaceIndex(int index);



Node*getWindowStack(Workspace*workspace);

/**
 *
 * @return the windows in the active workspace at the DEFAULT_LAYER
 */
Node*getActiveWindowStack();


/**
 * Returns the workspace at a given index
 * @param context
 * @return
 */
Workspace* getWorkspaceByIndex(int index);

void moveWindowToLayerForAllWorksppaces(WindowInfo* win,int layer);
void moveWindowToLayer(WindowInfo* win,int workspaceIndex,int layer);


int isWorkspaceVisible(int i);
int isWorkspaceNotEmpty(int index);
/**
 * Returns the next workspace not assigned to a monitor
 * @param context
 * @return
 */
Workspace*getNextHiddenWorkspace();
/**
 * Returns the workspace currently displayed by the monitor or null
 * @param context
 * @param monitor
 * @return
 */
Workspace*getWorkspaceFromMonitor(Monitor*monitor);

Monitor*getMonitorFromWorkspace(Workspace*workspace);



/**
 * Get the next window in the stacking order in the given direction
 * @param context
 * @param dir
 * @return
 */
Node* getNextWindowInStack(int dir);




/**
 * Get the master who most recently focused window.
 * Should be called to reset border focus after the active master's
 * focused has changed
 * @param context
 * @param win
 * @return
 */
Node* getNextMasterNodeFocusedOnWindow(int win);


int addMonitor(int id,int primary,int x,int y,int width,int height);
int intersects(short int arg1[4],short int arg2[4]);
void setMonitorForWorkspace(Workspace*w,Monitor*m);


/**
 *
 * @return list of docks
 */
Node*getAllDocks();
/**
 *
 * @return list of all monitors
 */
Node*getAllMonitors();


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
