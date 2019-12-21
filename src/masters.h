/**
 * @file masters.h
 * Contains methods related to the Master struct
 */
#ifndef MASTERS_H
#define MASTERS_H

#include "arraylist.h"
#include "globals.h"
#include "mywm-structs.h"
#include "slaves.h"
#include "workspaces.h"

/// will match any mode
#define ANY_MODE ((uint32_t)-1)

/**
 *
 * @return a list of all master devices
 */
ArrayList<Master*>& getAllMasters();
/**
 *
 * @return The master device currently interacting with the wm
 * @see setActiveMasterNodeByID
 */
Master* getActiveMaster(void);
/**
 * The active master should be set whenever the user interacts with the
 * wm (key/mouse binding, mouse press etc)
 * @param master the new active master
 */
void setActiveMaster(Master* master);

/// holds data on a master device pair like the ids and focus history
struct Master : WMStruct {
private:
    /// pointer master id associated with id
    const MasterID pointerID;
    /// the name of the master keyboard
    std::string name;
    /// the color windows when this device has the most recent focus
    uint32_t focusColor;
    /**Stack of windows in order of most recently focused*/
    ArrayList<Slave*>slaves;

    /**Stack of windows in order of most recently focused*/
    ReverseArrayList<WindowInfo*>windowStack;
    /**
     * Contains the window with current focus,
     * will be same as top of window stack if freezeFocusStack==0
     *
     */
    uint32_t focusedWindowIndex = 0;

    /**Time the focused window changed*/
    TimeStamp focusedTimeStamp = 0;
    /// The current binding mode
    uint8_t bindingMode = 0;
    /**When true, the focus stack won't be updated on focus change*/
    bool freezeFocusStack = 0;

    /// Index of active workspace;
    WorkspaceID activeWorkspaceIndex = 0;

public:
    /**
     * @param keyboardID the id of the master device.
     * @param pointerID the id of the associated master pointer
     * @param name the name of the master keyboard
     * @param focusColor
     */
    Master(MasterID keyboardID, MasterID pointerID, std::string name,
        int focusColor = 0x00FF00): WMStruct(keyboardID),
        pointerID(pointerID), name(name), focusColor(focusColor) {
        assert(keyboardID > 1 && pointerID > 1);
    }
    ~Master();
    /**
     * Returns an Iterable list where the first member will be the currently focused window
     * (if the stack isn't frozen)
     *
     * @return
     */
    const ReverseArrayList<WindowInfo*>& getWindowStack(void) {
        return windowStack;
    }
    /**
     * Prints the struct
     *
     * @param strm
     * @param m
     *
     * @return
     */
    friend std::ostream& operator<<(std::ostream& strm, const Master& m) ;
    /**
     * @copydoc Slave::setMasterID
     * @relates Slave
     */
    friend void Slave::setMasterID(MasterID id);
    /**
     * @return a list of all recorded slaves
     */
    const ArrayList<Slave*>& getSlaves(void)const {
        return slaves;
    }
    /// @return the keyboard
    MasterID getKeyboardID(void)const {return id;}
    /// @return the pointer id
    MasterID getPointerID(void)const {return pointerID;}
    /// @return the name of this device
    std::string getName(void)const {return name;}
    /**
     * @return whether of not the master window stack will be updated on focus change
    */
    int isFocusStackFrozen(void)const {
        return freezeFocusStack;
    }

    /**
     * If value, the master window stack will not be updated on focus change
     * Else the focused window will be shifted to the top of the master stack,
     * the master stack and the focused will remain in sync after all focus changes
     * @param value whether the focus stack is frozen or not
    */
    void setFocusStackFrozen(int value);
    /**
     * Get the WindowInfo representing the window the master is
     * currently focused on.
     * This value will be should be updated whenever the focus for the active
     * master changes. If the focused window is deleted, then the this value is
     * set the next window if the master focus stack
     * @return the currently focused window for the active master
     */
    WindowInfo* getFocusedWindow(void);
    /**
     * @return the time we noticed that the master focused its currently focused window
     */
    unsigned int getFocusedTime(void)const {
        return focusedTimeStamp;
    }
    /// @return the color to set to border of the focused window too
    unsigned int getFocusColor(void)const {
        return focusColor;
    }
    /**
     * @param color the new border color of the focused window
     */
    void setFocusColor(uint32_t color) {
        focusColor = color;
    }
    /**
     * Only bindings whose mode == this->mode || mode == ANY_MODE will be triggered by this master
     *
     * @param mode
     * @see allowsMode
     */
    void setCurrentMode(int mode) {bindingMode = mode;}
    /**
     * @copybrief setCurrentMode
     * @param mode the mode of the binding that might be triggered
     *
     * @return true if the mode is currently allowed by this master
     */
    bool allowsMode(uint32_t mode) const {
        return mode == ANY_MODE || bindingMode == mode;
    }
    /**
     * Returns the current binding mode for the active master
     *
     * @return
     */
    int getCurrentMode(void) const {
        return bindingMode;
    }
    /**
     * To be called when a master focuses a given window.
     *
     * If the window is not in the window list, nothing is done
     * Else if the window is not in the master's window stack, it is added to the end.
     * Else if the master is not frozen, the window is shifted to the end of the stack
     *
     * In all but the first case, a call to getFocusedWindow() will return the window represented by win
     *
     * @param win
     */
    void onWindowFocus(WindowID win);
    /**
     * Clears the focus stack
     */
    void clearFocusStack();
    /**
     * Removes a given window from the focus stack
     *
     * @param win the window to remove
     *
     * @return the window removed or NULL
     */
    WindowInfo* removeWindowFromFocusStack(WindowID win);

    /**
     *
     * @return the active workspace index
     */
    WorkspaceID getWorkspaceIndex(void)const {
        return activeWorkspaceIndex;
    }
    /**
     * Sets the active workspace index
     * @param index
     */
    void setWorkspaceIndex(WorkspaceID index) {
        activeWorkspaceIndex = index;
    }
    /**
     *
     * @return the active Workspace of this Master or NULL
     */
    Workspace* getWorkspace(void) {
        return ::getWorkspace(getWorkspaceIndex());
    }
};



/**
 * Creates and adds the default master if it doesn't already exits
 */
void addDefaultMaster();
/// @return the keyboard id of the active master
static inline MasterID getActiveMasterKeyboardID(void) {return getActiveMaster()->getKeyboardID();}
/// @return the pointer id of the active master
static inline MasterID getActiveMasterPointerID(void) {return getActiveMaster()->getPointerID();}
/// @return the ID of the active Workspace or NO_WORKSPACE
static inline WorkspaceID getActiveWorkspaceIndex(void) {return getActiveMaster()->getWorkspaceIndex();}
/// @return the active Workspace or NULL
static inline Workspace* getActiveWorkspace(void) {return getActiveMaster()->getWorkspace();}


/// @return the window with recorded focus for the active Master
static inline WindowInfo* getFocusedWindow() {return getActiveMaster()->getFocusedWindow();}

/**
 * @param name
 * @return the master with the given name
 */
Master* getMasterByName(std::string name) ;
/**
 * @brief returns the master node with id == keyboardID
 * @param id id of the master device
 * @param keyboard whether to look for a keyboard or pointer device
 * @return the master device with the give node
 */
Master* getMasterByID(MasterID id, bool keyboard = 1);
#endif
