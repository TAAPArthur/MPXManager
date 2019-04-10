/**
 * @file masters.h
 * Contains methods related to the Master struct
 */
#ifndef MASTERS_H
#define MASTERS_H

#include "mywm-structs.h"

/**
 * Creates a pointer to a Master instance and sets its id to id.
 * @param id keyboard id
 * @param pointerId pointer id
 * @param name the name of the master keyboard
 * @param focusColor
 * @return pointer to object
 */
Master* createMaster(MasterID id, int pointerId, char* name, int focusColor);
/**
 *
 * @return a list of all master devices
 */
ArrayList* getAllMasters();
/**
 * Creates and adds a given master with id = keyboardMasterId
 * if a master with this id does not already exist to the head of the
 * master list.
 * Note the new master does not replace the active master unless there
 * the master list is empty
 * @param keyboardMasterId  the id of the master device.
 * @param pointerMasterId  the id of the associated master pointer
 * @param name the name of the master keyboard
 * @param focusColor
 * @return 1 iff a new master was inserted; 0 o.w.
 */
int addMaster(MasterID keyboardMasterId, MasterID pointerMasterId, char* name, int focusColor);

/**
 * Removes the master with the specifed id
 * @param id    the id to remove
 * @return 1 iff a node was removed 0 o.w.
 */
int removeMaster(MasterID id);
/**
 * Removes a window from the master stack
 * @param master
 * @param winToRemove
 * @return 1 iff the window was actually removed
 */
int removeWindowFromMaster(Master* master, WindowID winToRemove);

/**
 * @return the id of the active master
 */
int getActiveMasterKeyboardID(void);
/**
 * @return the pointer id of the active master
 */
int getActiveMasterPointerID(void);
/**
 * @return  whether of not the master window stack will be updated on focus change
*/
int isFocusStackFrozen(void);
/**
 * If value, the master window stack will not be updated on focus change
 * Else the the focused window will be shifted to the top of the master stack,
 * the master stack and the focused will remain in sync after all focus changes
 * @param value whether the focus stack is frozen or not
*/
void setFocusStackFrozen(int value);

/**
 * @param master
 * @return Returns a list of windows that have been put in the cache
*/
ArrayList* getWindowCache(Master* master);
/**
 * Clears the window cache for the active master
 */
void clearWindowCache(void);
/**
 * Adds a window to the cache if not already present
 * @param winInfo  the window to add
 */
int updateWindowCache(WindowInfo* winInfo);

/**
 * Returns the top window in the stack relative to given master.
 * @return
 */
ArrayList* getActiveMasterWindowStack(void);


/**
 * Returns the top window in the stack relative to given master.
 * @return
 */
ArrayList* getWindowStackByMaster(Master* master);


/**
 * Get the node containing the window master is
 * currently focused on. This should never be null
 * @param master
 * @return the currently focused window for master
 */
WindowInfo* getFocusedWindowByMaster(Master* master);
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
ArrayList* getFocusedWindowNode();
/**
 *
 * @param m
 * @return the time the master focused the window
 */
unsigned getFocusedTime(Master* m);
/**
 *
 * @return The master device currently interacting with the wm
 * @see setActiveMasterNodeById
 */
Master* getActiveMaster(void);
/**
 * The active master should be set whenever the user interacts with the
 * wm (key/mouse  binding, mouse press etc)
 * @param master the new active master
 */
void setActiveMaster(Master* master);
/**
 * @brief returns the master node with id == keyboardId
 * @param keyboardID id of the master device
 * @return the master device with the give node
 */
Master* getMasterById(int keyboardID);
/**
 * Get the master who most recently focused window.
 * Should be called to reset border focus after the active master's
 * focused has changed
 * @param win
 * @return
 */
Master* getLastMasterToFocusWindow(WindowID win);
#endif
