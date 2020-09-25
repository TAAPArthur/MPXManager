#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>
#include "../util/arraylist.h"
#include "ewmh.h"
#include "../util/logger.h"
#include "../util/time.h"
#include "../user-events.h"
#include "../wmfunctions.h"
#include "../xutil/xsession.h"

//static ArrayList<WindowID> mappedOrder;
static void addDefaultMappableMask(WindowInfo* winInfo) {
    if(getWindowPropertyValueInt(winInfo->id, WM_STATE, XCB_ATOM_CARDINAL))
        addMask(winInfo, MAPPABLE_MASK);
}


void setSupportedActions() {
    xcb_atom_t net_atoms[] = {
        ewmh->_NET_SUPPORTED,
        ewmh->_NET_SUPPORTING_WM_CHECK,
        ewmh->_NET_WM_NAME,
        ewmh->_NET_CLIENT_LIST,
        ewmh->_NET_WM_STRUT, ewmh->_NET_WM_STRUT_PARTIAL,
        ewmh->_NET_WM_STATE,
        ewmh->_NET_WM_STATE_MODAL,
        ewmh->_NET_WM_STATE_ABOVE,
        MPX_WM_STATE_CENTER_X,
        MPX_WM_STATE_CENTER_Y,
        MPX_WM_STATE_NO_TILE,
        MPX_WM_STATE_ROOT_FULLSCREEN,
        ewmh->_NET_WM_STATE_BELOW,
        ewmh->_NET_WM_STATE_FULLSCREEN,
        ewmh->_NET_WM_STATE_HIDDEN,
        ewmh->_NET_WM_STATE_STICKY,
        ewmh->_NET_WM_STATE_DEMANDS_ATTENTION,
        ewmh->_NET_WM_STATE_MAXIMIZED_HORZ,
        ewmh->_NET_WM_STATE_MAXIMIZED_VERT,
        ewmh->_NET_WM_WINDOW_TYPE,
        ewmh->_NET_WM_WINDOW_TYPE_NORMAL,
        ewmh->_NET_WM_WINDOW_TYPE_DOCK,
        ewmh->_NET_WM_WINDOW_TYPE_DIALOG,
        ewmh->_NET_WM_WINDOW_TYPE_UTILITY,
        ewmh->_NET_WM_WINDOW_TYPE_TOOLBAR,
        ewmh->_NET_WM_WINDOW_TYPE_SPLASH,
        ewmh->_NET_WM_WINDOW_TYPE_MENU,
        ewmh->_NET_WM_WINDOW_TYPE_DROPDOWN_MENU,
        ewmh->_NET_WM_WINDOW_TYPE_POPUP_MENU,
        ewmh->_NET_WM_WINDOW_TYPE_NOTIFICATION,
        ewmh->_NET_ACTIVE_WINDOW, ewmh->_NET_CLOSE_WINDOW,
        ewmh->_NET_WM_DESKTOP, ewmh->_NET_NUMBER_OF_DESKTOPS,
        ewmh->_NET_CURRENT_DESKTOP, ewmh->_NET_SHOWING_DESKTOP,
        ewmh->_NET_DESKTOP_NAMES, ewmh->_NET_DESKTOP_VIEWPORT,
        ewmh->_NET_WM_USER_TIME_WINDOW, ewmh->_NET_WM_USER_TIME,
        ewmh->_NET_WM_ACTION_ABOVE, ewmh->_NET_WM_ACTION_BELOW,
        ewmh->_NET_WM_ACTION_CHANGE_DESKTOP,
        ewmh->_NET_WM_ACTION_CLOSE, ewmh->_NET_WM_ACTION_STICK,
        ewmh->_NET_WM_ACTION_FULLSCREEN, ewmh->_NET_WM_ACTION_MINIMIZE,
        ewmh->_NET_WM_ACTION_MAXIMIZE_HORZ, ewmh->_NET_WM_ACTION_MAXIMIZE_VERT,
        ewmh->_NET_WM_ACTION_MOVE, ewmh->_NET_WM_ACTION_RESIZE,
        ewmh->_NET_DESKTOP_GEOMETRY, ewmh->_NET_DESKTOP_VIEWPORT, ewmh->_NET_WORKAREA
    };
    xcb_ewmh_set_supported_checked(ewmh, defaultScreenNumber, LEN(net_atoms), net_atoms);
}
void setAllowedActions(WindowInfo* winInfo) {
    xcb_atom_t actions[] = {
        ewmh->_NET_WM_ACTION_CHANGE_DESKTOP, ewmh->_NET_WM_ACTION_CLOSE,
        ewmh->_NET_WM_ACTION_ABOVE, ewmh->_NET_WM_ACTION_BELOW,
        ewmh->_NET_WM_ACTION_CHANGE_DESKTOP,
        ewmh->_NET_WM_ACTION_CLOSE, ewmh->_NET_WM_ACTION_STICK,
        ewmh->_NET_WM_ACTION_FULLSCREEN, ewmh->_NET_WM_ACTION_MINIMIZE,
        ewmh->_NET_WM_ACTION_MAXIMIZE_HORZ, ewmh->_NET_WM_ACTION_MAXIMIZE_VERT,
        ewmh->_NET_WM_ACTION_MOVE, ewmh->_NET_WM_ACTION_RESIZE,
    };
    xcb_ewmh_set_wm_allowed_actions_checked(ewmh, winInfo->id, LEN(actions), actions);
}

void updateEWMHClientList() {
    WindowID ids[getAllWindows()->size];
    int i = 0;
    FOR_EACH(WindowInfo*, winInfo, getAllWindows()) {
        ids[i++] = winInfo->id;
    }
    xcb_ewmh_set_client_list(ewmh, defaultScreenNumber, i, ids);
}


void setShowingDesktop(int value) {
    INFO("setting showing desktop %d", value);
    FOR_EACH(WindowInfo*, winInfo, getAllWindows()) {
        if(winInfo->type == ewmh->_NET_WM_WINDOW_TYPE_DESKTOP)
            raiseLowerWindowInfo(winInfo, 0, value);
    }
    xcb_ewmh_set_showing_desktop(ewmh, defaultScreenNumber, value);
}

bool isShowingDesktop(void) {
    unsigned int value = 0;
    xcb_ewmh_get_showing_desktop_reply(ewmh, xcb_ewmh_get_showing_desktop(ewmh, defaultScreenNumber), &value, NULL);
    return value;
}

void syncShowingDesktop() {
    unsigned int value = 0;
    xcb_ewmh_get_showing_desktop_reply(ewmh, xcb_ewmh_get_showing_desktop(ewmh, defaultScreenNumber), &value, NULL);
    setShowingDesktop(value);
}

void onClientMessage2(xcb_client_message_event_t* event) {
    xcb_client_message_data_t data = event->data;
    xcb_window_t win = event->window;
    Atom message = event->type;
    if(message == ewmh->WM_PROTOCOLS) {
        if(data.data32[0] == ewmh->_NET_WM_PING) {
            if(win == root) {
                DEBUG("Pong received");
                WindowInfo* winInfo = getWindowInfo(data.data32[2]);
                if(winInfo) {
                    DEBUG("Updated ping timestamp for window %d\n", winInfo->id);
                    winInfo->pingTimeStamp = getTime();
                }
            }
            else if(win == getPrivateWindow()) {
                DEBUG("Pong requested");
                xcb_ewmh_send_client_message(ewmh->connection, root, root, ewmh->WM_PROTOCOLS, sizeof(data), data.data32);
                flush();
            }
        }
    }
}

void addMoreEWMHRules() {
    addBatchEvent(POST_REGISTER_WINDOW, DEFAULT_EVENT(updateEWMHClientList));
    addBatchEvent(UNREGISTER_WINDOW, DEFAULT_EVENT(updateEWMHClientList));
    addEvent(POST_REGISTER_WINDOW, DEFAULT_EVENT(addDefaultMappableMask, HIGHER_PRIORITY));
    addEvent(POST_REGISTER_WINDOW, DEFAULT_EVENT(setAllowedActions));
    addEvent(X_CONNECTION, DEFAULT_EVENT(syncShowingDesktop));
    addEvent(XCB_CLIENT_MESSAGE, DEFAULT_EVENT(onClientMessage2));
}
