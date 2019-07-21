
#include <string>

#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>


#include "bindings.h"
#include "device-grab.h"
#include "ewmh.h"
#include "ext.h"
#include "logger.h"
#include "monitors.h"
#include "mywm-structs.h"
#include "system.h"
#include "time.h"
#include "user-events.h"
#include "window-masks.h"
#include "window-properties.h"
#include "windows.h"
#include "xsession.h"
using namespace std;
void setWindowTransientFor(WindowID win, WindowID transientTo) {
    xcb_icccm_set_wm_transient_for(dis, win, transientTo);
}
void setWindowClass(WindowID win, string className, string instanceName) {
    char classInstance[className.size() + instanceName.size() + 2];
    strcpy(classInstance, instanceName.c_str());
    strcpy(classInstance + instanceName.size() + 1, className.c_str());
    xcb_icccm_set_wm_class(dis, win, LEN(classInstance), classInstance);
}
void setWindowTitle(WindowID win, std::string title) {
    xcb_ewmh_set_wm_name(ewmh, win, title.size(), title.c_str());
}
void setWindowType(WindowID win, xcb_atom_t* atoms, int num) {
    xcb_ewmh_set_wm_window_type(ewmh, win, num, atoms);
}
bool loadClassInfo(WindowID win, string* className, string* instanceName) {
    xcb_icccm_get_wm_class_reply_t prop;
    xcb_get_property_cookie_t cookie = xcb_icccm_get_wm_class(dis, win);
    if(xcb_icccm_get_wm_class_reply(dis, cookie, &prop, NULL)) {
        *className = string(prop.class_name);
        *instanceName = string(prop.instance_name);
        xcb_icccm_get_wm_class_reply_wipe(&prop);
        return 1;
    }
    return 0;
}
string getWindowTitle(WindowID win) {
    xcb_ewmh_get_utf8_strings_reply_t wtitle;
    xcb_get_property_cookie_t cookie = xcb_ewmh_get_wm_name(ewmh, win);
    string str = "";
    if(xcb_ewmh_get_wm_name_reply(ewmh, cookie, &wtitle, NULL)) {
        str = string(wtitle.strings, wtitle.strings_len);
        xcb_ewmh_get_utf8_strings_reply_wipe(&wtitle);
    }
    else {
        xcb_icccm_get_text_property_reply_t icccName;
        cookie = xcb_icccm_get_wm_name(dis, win);
        if(xcb_icccm_get_wm_name_reply(dis, cookie, &icccName, NULL)) {
            str = string(icccName.name, icccName.name_len);
            xcb_icccm_get_text_property_reply_wipe(&icccName);
        }
    }
    return str;
}
bool loadWindowType(WindowID win, bool transient, uint32_t* type, string* typeName) {
    xcb_ewmh_get_atoms_reply_t name;
    bool failed = 0;
    if(xcb_ewmh_get_wm_window_type_reply(ewmh,
                                         xcb_ewmh_get_wm_window_type(ewmh, win), &name, NULL)) {
        *type = name.atoms[0];
        xcb_ewmh_get_atoms_reply_wipe(&name);
    }
    else {
        LOG(LOG_LEVEL_TRACE, "could not read window type; using default based on transient being set to %d\n", transient);
        *type = transient ? ewmh->_NET_WM_WINDOW_TYPE_DIALOG : ewmh->_NET_WM_WINDOW_TYPE_NORMAL;
        failed = 1;
    }
    *typeName = getAtomValue(*type);
    return !failed;
}


/**
 * Loads grouptId, input and window state for a given window
 * @param winInfo
 */
static void loadWindowHints(WindowInfo* winInfo) {
    xcb_icccm_wm_hints_t hints;
    if(xcb_icccm_get_wm_hints_reply(dis, xcb_icccm_get_wm_hints(dis, winInfo->getID()), &hints, NULL)) {
        winInfo->setGroup(hints.window_group);
        if(hints.initial_state == XCB_ICCCM_WM_STATE_NORMAL)
            winInfo->addMask(MAPPABLE_MASK);
        if(hints.input)
            winInfo->addMask(INPUT_MASK);
        else
            winInfo->removeMask(INPUT_MASK);
    }
    xcb_size_hints_t sizeHints;
    if(xcb_icccm_get_wm_size_hints_reply(dis, xcb_icccm_get_wm_size_hints(dis, winInfo->getID(), XCB_ATOM_WM_NORMAL_HINTS),
                                         &sizeHints
                                         , NULL))
        *getWindowSizeHints(winInfo) = sizeHints;
}

/**
 * Load window protocols like WM_DELETE_WINDOW
 * @param winInfo
 */
static void loadProtocols(WindowInfo* winInfo) {
    xcb_icccm_get_wm_protocols_reply_t reply;
    if(xcb_icccm_get_wm_protocols_reply(dis,
                                        xcb_icccm_get_wm_protocols(dis, winInfo->getID(), ewmh->WM_PROTOCOLS),
                                        &reply, NULL)) {
        for(int i = 0; i < reply.atoms_len; i++)
            if(reply.atoms[i] == WM_DELETE_WINDOW)
                winInfo->addMask(WM_DELETE_WINDOW_MASK);
            else if(reply.atoms[i] == ewmh->_NET_WM_PING)
                winInfo->addMask(WM_PING_MASK);
            else if(reply.atoms[i] == WM_TAKE_FOCUS)
                winInfo->addMask(WM_TAKE_FOCUS_MASK);
        xcb_icccm_get_wm_protocols_reply_wipe(&reply);
    }
}
void loadWindowTitle(WindowInfo* winInfo) {
    winInfo->setTitle(getWindowTitle(winInfo->getID()));
}
void loadWindowProperties(WindowInfo* winInfo) {
    LOG(LOG_LEVEL_VERBOSE, "loading window properties %d\n", winInfo->getID());
    loadClassInfo(winInfo->getID(), &winInfo->className, &winInfo->instanceName);
    loadWindowTitle(winInfo);
    xcb_window_t prop;
    if(xcb_icccm_get_wm_transient_for_reply(dis, xcb_icccm_get_wm_transient_for(dis, winInfo->getID()), &prop, NULL))
        winInfo->transientFor = prop;
    loadProtocols(winInfo);
    loadWindowHints(winInfo);
    if(winInfo->isDock())
        loadDockProperties(winInfo);
    if(!loadWindowType(winInfo->getID(), winInfo->transientFor, &winInfo->type, &winInfo->typeName))
        winInfo->addMask(IMPLICIT_TYPE);
    else winInfo->removeMask(IMPLICIT_TYPE);
}
void setBorderColor(WindowID win, unsigned int color) {
    if(color > 0xFFFFFF) {
        LOG(LOG_LEVEL_WARN, "Color %d is out f range\n", color);
        return;
    }
    xcb_change_window_attributes(dis, win, XCB_CW_BORDER_PIXEL, &color);
    LOG(LOG_LEVEL_TRACE, "setting border for window %d to %#08x\n", win, color);
}
void setBorder(WindowID win) {
    setBorderColor(win, getActiveMaster()->getFocusColor());
}
void resetBorder(WindowID win) {
    int maxValue = 0;
    Master* master = NULL;
    for(Master* m : getAllMasters()) {
        if(m != getActiveMaster() && m->getFocusedWindow() && m->getFocusedWindow()->getID() == win)
            if(m->getFocusedTime() >= maxValue) {
                master = m;
                maxValue = m->getFocusedTime();
            }
    }
    if(master)
        setBorderColor(win, master->getFocusColor());
    setBorderColor(win, DEFAULT_UNFOCUS_BORDER_COLOR);
}

void floatWindow(WindowInfo* winInfo) {
    winInfo->addMask(FLOATING_MASK);
}
void sinkWindow(WindowInfo* winInfo) {
    winInfo->removeMask(ALL_NO_TILE_MASKS);
}

xcb_size_hints_t* getWindowSizeHints(WindowInfo* winInfo) {
    static Index<xcb_size_hints_t> key;
    return get(key, winInfo);
}

//TODO consider moving to devices
int focusWindow(WindowID win, Master* master) {
    LOG(LOG_LEVEL_DEBUG, "Trying to set focus to %d for master %d\n", win, master->getID());
    assert(win);
    xcb_void_cookie_t cookie = xcb_input_xi_set_focus_checked(dis, win, XCB_CURRENT_TIME,
                               master->getKeyboardID());
    if(catchError(cookie)) {
        return 0;
    }
    if(SYNC_FOCUS)
        master->onWindowFocus(win);
    return 1;
}
int focusWindow(WindowInfo* winInfo, Master* master) {
    if(!winInfo->hasMask(INPUT_MASK))
        return 0;
    if(winInfo->hasMask(WM_TAKE_FOCUS_MASK)) {
        //TODO MPX support
        uint32_t data[] = {WM_TAKE_FOCUS, getTime()};
        LOG(LOG_LEVEL_TRACE, "sending request to take focus to ");
        xcb_ewmh_send_client_message(dis, winInfo->getID(), winInfo->getID(), ewmh->WM_PROTOCOLS, 2, data);
        return 1;
    }
    else
        return focusWindow(winInfo->getID(), master);
}
int loadDockProperties(WindowInfo* winInfo) {
    LOG(LOG_LEVEL_TRACE, "Reloading dock properties\n");
    xcb_window_t win = winInfo->getID();
    xcb_ewmh_wm_strut_partial_t strut;
    if(xcb_ewmh_get_wm_strut_partial_reply(ewmh,
                                           xcb_ewmh_get_wm_strut_partial(ewmh, win), &strut, NULL)) {
        winInfo->setDockProperties((int*)&strut, sizeof(xcb_ewmh_wm_strut_partial_t) / sizeof(int));
    }
    else if(xcb_ewmh_get_wm_strut_reply(ewmh,
                                        xcb_ewmh_get_wm_strut(ewmh, win),
                                        (xcb_ewmh_get_extents_reply_t*) &strut, NULL))
        winInfo->setDockProperties((int*)&strut, sizeof(xcb_ewmh_get_extents_reply_t) / sizeof(int));
    else {
        LOG(LOG_LEVEL_TRACE, "could not read struct data\n");
        winInfo->setDockProperties(NULL, 0);
        return 0;
    }
    if(winInfo->isDock())
        resizeAllMonitorsToAvoidAllDocks();
    return 1;
}
