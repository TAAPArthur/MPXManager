/**
 * @file masters.h
 * Contains methods related to the Master struct
 */
#ifndef MASTERS_H
#define MASTERS_H

#include "arraylist.h"
#include "mywm-structs.h"
#include "workspaces.h"
#include "slaves.h"
#include "globals.h"
#include <string.h>
#include <string>

/**
 *
 * @return a list of all master devices
 */
ArrayList<Master*>& getAllMasters();
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

///holds data on a master device pair like the ids and focus history
struct Master : WMStruct {
private:
    /**pointer master id associated with id;*/
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
    char bindingMode = 0;
    char bindingFamily = 0;
    /**When true, the focus stack won't be updated on focus change*/
    bool freezeFocusStack = 0;
    /// If true ignore all events with the send_event bit
    bool ignoreSendEvents;
    /// If true ignore all device events with the key repeat flag set
    bool ignoreKeyRepeat;

    /**Index of active workspace;
     * */
    WorkspaceID activeWorkspaceIndex = 0;
    /// When set, device event rules will be passed this window instead of the active window or focused
    WindowID targetWindow = 0;

public:
    /**
     * @param keyboardID  the id of the master device.
     * @param pointerID  the id of the associated master pointer
     * @param name the name of the master keyboard
     * @param focusColor
     */
    Master(MasterID keyboardID, MasterID pointerID, std::string n,
        int focusColor = 0x00FF00): WMStruct(keyboardID),
        pointerID(pointerID), name(n),        focusColor(focusColor) {
        assert(keyboardID > 1 && pointerID > 1);
    }
    ~Master();
    ReverseArrayList<WindowInfo*>& getWindowStack(void) {
        return windowStack;
    }
    friend std::ostream& operator<<(std::ostream& strm, const Master& m) ;
    friend void Slave::setMasterID(MasterID id);
    const ArrayList<Slave*>& getSlaves(void)const {
        return slaves;
    }
    MasterID getKeyboardID(void)const {return id;}
    MasterID getPointerID(void)const {return pointerID;}
    std::string getName(void)const {return name;}
    /**
     * @return  whether of not the master window stack will be updated on focus change
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
     *
     * @return the time the master focused the window
     */
    unsigned int getFocusedTime(void)const {
        return focusedTimeStamp;
    }
    unsigned int getFocusColor(void)const {
        return focusColor;
    }
    void setFocusColor(uint32_t color) {
        focusColor = color;
    }
    bool isIgnoreKeyRepeat() {return ignoreKeyRepeat;}
    void setIgnoreKeyRepeat(bool b = 1) {ignoreKeyRepeat = b;}
    bool isIgnoreSendEvents() {return ignoreSendEvents;}
    /**
     * Only bindings whose mode & mode will be triggered by this master
     *
     * @param mode
     */
    void setCurrentMode(int mode) {bindingMode = mode;}
    bool allowsMode(int mode) {
        return bindingMode == mode || (bindingFamily & (1 << mode));
    }
    /**
     * Returns the current binding mode for the active master
     *
     * @return
     */
    int getCurrentMode(void) {
        return bindingMode;
    }
    WindowID getTargetWindow(void)const {return targetWindow;}
    void setTargetWindow(WindowID target) {targetWindow = target;}
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
     *
     * @return the active workspace index
     */
    int getWorkspaceIndex(void)const {
        return activeWorkspaceIndex;
    }
    /**
     * Sets the active workspace index
     * @param index
     */
    void setWorkspaceIndex(int index) {
        activeWorkspaceIndex = index;
    }
    Workspace* getWorkspace(void) {
        return ::getWorkspace(getWorkspaceIndex());
    }
    WindowInfo* getMostRecentlyFocusedWindow(bool(*filter)(WindowInfo*));
};



void addDefaultMaster();
/**
 * @return the id of the active master
 */
static inline MasterID getActiveMasterKeyboardID(void) {return getActiveMaster()->getKeyboardID();}
static inline MasterID getActiveMasterPointerID(void) {return getActiveMaster()->getPointerID();}
static inline WorkspaceID getActiveWorkspaceIndex(void) {return getActiveMaster()->getWorkspaceIndex();}
static inline Workspace* getActiveWorkspace(void) {return getActiveMaster()->getWorkspace();}


static inline WindowInfo* getFocusedWindow() {return getActiveMaster()->getFocusedWindow();}

Master* getMasterByName(std::string name) ;
/**
 * @brief returns the master node with id == keyboardId
 * @param id id of the master device
 * @return the master device with the give node
 */
Master* getMasterById(MasterID id, bool keyboard = 1);
#endif
