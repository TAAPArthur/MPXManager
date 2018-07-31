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

extern Layout DEFAULT_LAYOUTS[];
extern int NUMBER_OF_DEFAULT_LAYOUTS;

/**
 * @return the current time in seconds
 */
unsigned int getTime ();
/**
 * Sleep of mil milliseconds
 * @param mil number of milliseconds to sleep
 */
void msleep(int mil);

/**
 * Initializes a context object to default values.
 *
 * All Node points are set to point to an empty node.
 * Workspaces is set to point to the head of an array with size==
 * numberOfWorkspaces
 *
 * If a context already exists, it is first destroyed
 * @param numberOfWorkspaces    the number of workspaces
 */
void createContext(int numberOfWorkspaces);

/**
 * Destroys the reference to context and clears all resources.
 * It does nothing if context points to NULL
 */
void destroyContext();

/**
 * Creates a pointer to a WindowInfo object and sets its id to id
 *
 * The workspaceIndex of the window is set to NO_WORKSPACE
 * @param id    the id of the WindowInfo object
 * @return  pointer to the newly created object
 */
WindowInfo* createWindowInfo(unsigned int id);


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
 *
 * @return a list of all master devices
 */
Node* getAllMasters();
/**
 * Creates and adds a given master with id = keyboardMasterId
 * if a master with this id does not already exist to the head of the
 * master list.
 * Note the new master does not replace the active master unless there
 * the master list is empty
 * @param keyboardMasterId  the id of the master device.
 * This is expected to be the id of a master keyboard, but any int will work
 * @return 1 iff a new master was inserted; 0 o.w.
 */
int addMaster(unsigned int keyboardMasterId,unsigned int pointerMasterId);

/**
 * Removes the master with the specifed id
 * @param id    the id to remove
 * @return 1 iff a node was removed 0 o.w.
 */
int removeMaster(unsigned int id);
/**
 * Removes a window from the master stack
 * @param masterNode
 * @param winToRemove
 * @return
 */
int removeWindowFromMaster(Master*master,int winToRemove);

/**
 * @return the id of the active master
 */
int getActiveMasterKeyboardID();
/**
 * @return the pointer id of the active master
 */
int getActiveMasterPointerID();
/**
 * @return  whether of not the master window stack will be updated on focus change
*/
int isFocusStackFrozen();
/**
 * If value, the master window stack will not be updated on focus change
 * Else the the focused window will be shifted to the top of the master stack,
 * the master stack and the focused will remain in sync after all focus changes
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
 * @param dir wheter to get teh next (>0) or the prev (<0) window
 * @return the next or previous window depending on dir
 */
Node* getNextWindowInFocusStack(int dir);

/**
 * Get the node containing the window master is
 * currently focused on. This should never be null
 * @param master
 * @return the currently focused window for master
 */
Node* getFocusedWindowByMaster(Master*master);
/**
 * Get the WindowInfo representing the window the master is
 * currently focused on.
 * This value will be should be updated whenever the focus for the active
 * master changes. If the focused window is deleted, then the this value is
 * set the next window if the master focus stack
 * @return the currently focused window for the active master
 */
WindowInfo* getFocusedWindow();
/**
 *
 * @return the node in the master window list representing the focused window
 */
Node* getFocusedWindowNode();
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
 * @return a list of windows with the most recently used windows first
 */
Node*getAllWindows();




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
void setDockArea(WindowInfo* info,int numberofProperties,int properties[WINDOW_STRUCT_ARRAY_SIZE]);


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
 * @param windowInfo
 */
void onWindowFocus(unsigned int win);


/**
 *
 * @return The master device currently interacting with the wm
 * @see setActiveMasterNodeById
 */
Master*getActiveMaster();
/**
 * The active master should be set whenever the user interacts with the
 * wm (key/mouse  binding, mouse press etc)
 * @param the new active master
 */
void setActiveMaster(Master*master);
/**
 * @brief returns the master node with id == keyboardId
 * @param id id of the master device
 * @return the master device with the give node
 */
Master*getMasterById(int keyboardID);
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
 * Returns the windows in the active workspace at a given layer
 * @param layer
 * @return
 */
Node* getWindowStackAtLayer(Workspace*workspace,int layer);

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




/**
 *
 * @param i the workspace index
 * @return 1 iff the workspace has been assigned a monitor
 */
int isWorkspaceVisible(int i);
/**
 *
 * @param index
 * @return if the workspace has at least one window explicitly assigned to it
 */
int isWorkspaceNotEmpty(int index);
/**
 * Returns the next workspace not assigned to a monitor
 * @return
 */
Workspace*getNextHiddenWorkspace();
/**
 * Get the next workspace in the given direction according to the filters
 * @param dir the interval of workspaces to check
 * @param empty either 0,1,-1 for empty workspace, non empty workspace or either
 * @param hidden either 0,1,-1 for hidden workspace, visible workspace or either
 * @return the next workspace in the given direction
 */
Workspace*getNextWorkspace(int dir,int empty,int hidden);
/**
 * This method is a convince wrapper around getNextWorkspace
 * It is equalivent to calling getNextWorkspace(1,0,1)
 * @return the next hidden, non-empty workspace from the active workspace
 * @see getNextWorkspace
 */
Workspace*getNextHiddenNonEmptyWorkspace();
/**
 * This method is a convince wrapper around getNextWorkspace
 * It is equalivent to calling getNextWorkspace(1,-1,1)
 * @return the next hidden workspace from the active workspace
 * @see getNextWorkspace
 */
Workspace*getNextHiddenWorkspace();

/**
 * Returns the workspace currently displayed by the monitor or null
 * @param context
 * @param monitor
 * @return
 */
Workspace*getWorkspaceFromMonitor(Monitor*monitor);

/**
 * @param workspace
 * @return the monitor assoicated with the given workspace if any
 */
Monitor*getMonitorFromWorkspace(Workspace*workspace);

/**
 * Swaps the monitors assosiated with the given workspaces
 * @param index1
 * @param index2
 */
void swapMonitors(int index1,int index2);

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
Master* getLastMasterToFocusWindow(int win);

/**
 *
 * @param id id of monitor TODO need to convert handle long to int conversion
 * @param primary   if the monitor the primary
 * @param geometry an array containing the x,y,width,height of the monitor
 * @return 1 iff a new monitor was added
 */
int addMonitor(int id,int primary,short geometry[4]);
int isPrimary(Monitor*monitor);
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
 * Clears all layouts assosiated with the give workspace.
 * The layouts themselves are not freeded
 * @param workspaceIndex the workspace to clear
 */
void clearLayoutsOfWorkspace(int workspaceIndex);
/**
 * The the currently used layout for the active workspace.
 * Note that this layout does not have to be in the list of layout for the active workspace
 * @param layout the new active layout
 */
void setActiveLayout(Layout*layout);
/**
 * Sets the next layout to be the nth from the current position.
 * Note that the current position need not be the active layout
 * @param workspaceIndex
 * @param delta
 * @return
 *
 */
Layout* switchToNthLayout(int workspaceIndex,int n);
Layout* getActiveLayout();
Layout* getActiveLayoutOfWorkspace(int workspaceIndex);
void addLayoutsToWorkspace(int workspaceIndex,Layout*layout,int num);


#endif
