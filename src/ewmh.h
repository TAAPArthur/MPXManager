/**
 * @file
 * Provides macros to set various EWMH properties
 */

#ifndef MPX_EWMH_H_
#define MPX_EWMH_H_

#include <string.h>
#include <string>
#include <xcb/xcb_ewmh.h>

#include "boundfunction.h"
#include "masters.h"
#include "mywm-structs.h"
#include "windows.h"
#include "xsession.h"
/**
 * Helper class to concate strings (along with the null pointer) so that they
 * can be posted as a window property
 */
struct StringJoiner {
private:
    char* buffer;
    int bufferSize;
    int usedBufferSize;
public:
    StringJoiner() {
        bufferSize = 64;
        usedBufferSize = 0;
        buffer = (char*)malloc(bufferSize);
    }
    ~StringJoiner() {
        free(buffer);
    }
    /**
     * Adds a string (with its null terminator) to the end of the buffer
     *
     * @param str the string to be added
     */
    void add(std::string str) {
        const int requiredSize = usedBufferSize + (str.length()) + 1;
        if(bufferSize < requiredSize) {
            bufferSize = requiredSize * 2;
            buffer = (char*)realloc(buffer, bufferSize);
        }
        strcpy(&buffer[usedBufferSize], str.c_str());
        usedBufferSize += (str.length()) + 1;
    }
    /**
     *
     *
     * @return the concatenation of every string added along with each null terminator
     */
    char* getBuffer() { return buffer; }
    /**
     * @return the length (including null terminator) of every string added
     */
    int getSize() { return usedBufferSize; }
};

/**
 *
 *
 * @return 1 iff there is a running instance of MPXManager and it is acting like a window manger (RUN_AS_WM==1)
 */
bool isMPXManagerRunning(void);

/**
 * Do everything needed to comply when EWMH.
 * Creates a functionless window with set all the Atoms we support, clear any
 * unsupported values from the root window
 */
void broadcastEWMHCompilence();
/**
 * Sets all the ewmh we support.
 *
 * Note that by default we don't support large desktops
 *
 * We don't currently support _NET_CLIENT_LIST_STACKING because it is a pain to properly implement
 */
void setSupportedActions();

/**
 * Updates the root window with a list of windows we are managing
 */
void updateEWMHClientList();
/**
 * Mark the active window and active workspace
 */
void setActiveProperties();


/**
 * Adds a series of rules to be compliant with EWMH/ICCCM spec
 *
 * @param remove
 */
void addEWMHRules(bool remove = 0);

/**
 * Reads EWMH desktop property to find the workspace the window should be long.
 * If the field is not present the active workspace is used.
 * If the field is greater than the number of workspaces, the last workspace is used
 * @param win
 * @return the workspace the window should be placed in by default
 */
WorkspaceID getSavedWorkspaceIndex(WindowID win);
/**
 * Sets the EWMH workspace property for this window to its workspace
 *
 * @param winInfo
 */
void setSavedWorkspaceIndex(WindowInfo* winInfo);

/**
 * Moves a window to its saved workspace index
 *
 * @see getSavedWorkspaceIndex
 *
 * @param winInfo
 */
void autoResumeWorkspace(WindowInfo* winInfo);
/**
 * Set all the workspace names to the root window
 */
void updateWorkspaceNames();

/**
 * Check to see if we are showing the desktop.
 * @return 1 if we are currently showing the desktop
 */
bool isShowingDesktop(void);
/**
 * show or hide desktop depending on values
 * @param value 1 or 0
 */
void setShowingDesktop(int value);

/**
 * Responds to client messages according the ICCCM/EWMH spec
 */
void onClientMessage(void);

/// the default source when sending a WM request
extern xcb_ewmh_client_source_type_t source;

/**
 * Send ClientMessage requesting to change the _NET_ACTIVE_WINDOW
 *
 * @param wid
 */
static inline void sendActivateWindowRequest(WindowID wid) {
    xcb_ewmh_request_change_active_window(ewmh, defaultScreenNumber, wid, source, 0,
        getFocusedWindow() ? getFocusedWindow()->getID() : root);
}
/**
 * Wrapper for xcb_ewmh_request_change_wm_state
 *
 * @param wid
 * @param action
 * @param state
 * @param state2
 */
static inline void sendChangeWindowStateRequest(WindowID wid, xcb_ewmh_wm_state_action_t action,
    xcb_atom_t state, xcb_atom_t state2 = 0) {
    xcb_ewmh_request_change_wm_state(ewmh, defaultScreenNumber, wid, action,
        state, state2, source);
}
/**
 * wrapper for xcb_ewmh_request_change_wm_desktop
 *
 * @param wid
 * @param index
 */
static inline void sendChangeWindowWorkspaceRequest(WindowID wid, WorkspaceID index) {
    xcb_ewmh_request_change_wm_desktop(ewmh, defaultScreenNumber, wid, index, source);
}
/**
 * Wrapper for xcb_ewmh_request_change_current_desktop
 *
 * @param id
 */
static inline void sendChangeWorkspaceRequest(WorkspaceID id) {
    xcb_ewmh_request_change_current_desktop(ewmh, defaultScreenNumber, id, 0);
}
/**
 *
 *
 * @param num
 */
static inline void sendChangeNumWorkspaceRequestAbs(int num) {
    xcb_ewmh_request_change_number_of_desktops(ewmh, defaultScreenNumber, num);
}
/**
 * wrapper for sendChangeNumWorkspaceRequest
 * @param num
 */
static inline void sendChangeNumWorkspaceRequest(int num) {
    sendChangeNumWorkspaceRequest(getNumberOfWorkspaces() + num);
}
/**
 * wrapper for xcb_ewmh_request_close_window
 * @param wid
 */
static inline void sendCloseWindowRequest(WindowID wid) {
    xcb_ewmh_request_close_window(ewmh, defaultScreenNumber, wid, 0, source);
}
/**
 * Wrapper for sendWMMoveResizeRequest
 * @param wid
 * @param x
 * @param y
 * @param direction
 * @param button
 */
static inline void sendWMMoveResizeRequest(WindowID wid, short x, short y,
    xcb_ewmh_moveresize_direction_t direction,
    uint8_t button
) {
    xcb_ewmh_request_wm_moveresize(ewmh, defaultScreenNumber, wid, x, y, direction, (xcb_button_index_t)button,
        source);
}
/**
 *
 *
 * @param wid
 * @param values
 * @param mask
 */
static inline void sendMoveResizeRequest(WindowID wid, const short* values, int mask) {
    assert(XCB_EWMH_MOVERESIZE_WINDOW_X == 1 << 8);
    xcb_ewmh_request_moveresize_window(ewmh, defaultScreenNumber, wid,
        XCB_GRAVITY_NORTH_WEST, source, (xcb_ewmh_moveresize_window_opt_flags_t)(mask << 8), values[0], values[1], values[2],
        values[3]);
}
/**
 * Wrapper for xcb_ewmh_request_restack_window
 *
 * @param wid
 * @param sibling_window
 * @param detail
 */
static inline void sendRestackRequest(WindowID wid, xcb_window_t sibling_window, xcb_stack_mode_t detail) {
    xcb_ewmh_request_restack_window(ewmh, defaultScreenNumber, wid, sibling_window, detail);
}

/**
 * Starts a window move-resize request
 *
 * The current window position and mouse position is stored. ON subsequent calls
 * to updateWindowMoveResize the window position will be change by the delta in the mouse position.
 *
 * If this method is called twice, the previous stored value is forgotten.
 * This method does not itself affect the window position
 *
 * @param winInfo
 * @param move whether to move (change position) or resize (change width/height)
 * @param allowMoveX allow x position/width to be changed
 * @param allowMoveY allow y position/height to be changed
 * @param btn
 * @param m
 */
void startWindowMoveResize(WindowInfo* winInfo, bool move, bool allowMoveX = 1, bool allowMoveY = 1, bool btn = 0,
    Master* m = getActiveMaster());
/**
 * Update a window-move resize with the new mouse position.
 * If a request has not been started (@see startWindowMoveResize) this method is a no-op
 * The current master position is calculated, and the window is move/resized according to the displacement of the current position and the stored position.
 *
 * If the mouse delta is 0, this is a no-op
 * If the resize would cause dimension to be exactly 0, that dimension would have size 1
 * If the resize make a dimension negative, the window is moved and resized in a standard manner
 * @param m
 */
void updateWindowMoveResize(Master* m = getActiveMaster()) ;
/**
 * Commits the window-move resize
 * Subsequent calls to updateWindowMoveResize or cancelWindowMoveResize won't have any effect
 *
 * @param m
 */
void commitWindowMoveResize(Master* m = getActiveMaster()) ;

/**
 * A DEVICE_EVENT function that will triggered commitWindowMoveResize when the client specified button is released
 * @param m
 *
 * @return 1 iff commitWindowMoveResize was called
 */
bool detectWindowMoveResizeButtonRelease(Master* m);
/**
 * Cancels the window-move resize operation
 *
 * The window is reverted to its original position/size it had when startWindowMoveResize was called
 *
 * Subsequent calls to updateWindowMoveResize or commitWindowMoveResize won't have any effect
 *
 *
 * @param m
 */
void cancelWindowMoveResize(Master* m = getActiveMaster());
/**
 * Sets the WM_STATE from the window masks
 */
void setXWindowStateFromMask(WindowInfo* winInfo, xcb_atom_t* atoms = NULL, int len = 0);
/**
 * Reads the WM_STATE fields from the given window and sets the window mask to be consistent with the state
 * If the WM_STATE cannot be read, then nothing is done
 * @see setWindowStateFromAtomInfo
 */
void loadSavedAtomState(WindowInfo* winInfo);

/**
 * Updates WM_STATE for all MAPPABLE windows where MASKS_TO_SYNC has changed
 */
void updateXWindowStateForAllWindows();

/**
 * Sets the window state based on a list of atoms
 * @param winInfo the window whose mask will be modified
 * @param atoms the list of atoms to add,remove or toggle depending on ACTION
 * @param numberOfAtoms the number of atoms
 * @param action whether to add,remove or toggle atoms. For toggle, if all of the corresponding masks for the list of atoms is set, then they all will be removed else they all will be added
 * @see XCB_EWMH_WM_STATE_ADD, XCB_EWMH_WM_STATE_REMOVE, XCB_EWMH_WM_STATE_TOGGLE
 */
bool setWindowStateFromAtomInfo(WindowInfo* winInfo, const xcb_atom_t* atoms, uint32_t numberOfAtoms, int action);
/**
 * Sets the WM_ACTIONS supported for this given window (which is the same for all of them)
 *
 * @param winInfo
 */
void setAllowedActions(WindowInfo* winInfo);
/**
 * Sync EWMH global properties with our internal state
 */
void syncState();
#endif /* EWMH_H_ */
