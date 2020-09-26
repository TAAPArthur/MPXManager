#include <xcb/xcb.h>
#include <xcb/xcb_ewmh.h>
#include <xcb/xcb_icccm.h>

#include "device-grab.h"
#include "../util/logger.h"
#include "../monitors.h"
#include "../mywm-structs.h"
#include "../system.h"
#include "../util/time.h"
#include "../user-events.h"
#include "../boundfunction.h"
#include "../window-masks.h"
#include "window-properties.h"
#include "../windows.h"
#include "xsession.h"

void setWindowTransientFor(WindowID win, WindowID transientTo) {
    xcb_icccm_set_wm_transient_for(dis, win, transientTo);
}
void setWindowClass(WindowID win, const char* className, const char* instanceName) {
    char classInstance[strlen(className) + strlen(instanceName) + 2];
    strcpy(classInstance, instanceName);
    strcpy(classInstance + strlen(instanceName) + 1, className);
    xcb_icccm_set_wm_class(dis, win, LEN(classInstance), classInstance);
}
void setWindowTitle(WindowID win, const char* title) {
    xcb_ewmh_set_wm_name(ewmh, win, strlen(title), title);
}
void setWindowTypes(WindowID win, xcb_atom_t* atoms, int num) {
    xcb_ewmh_set_wm_window_type(ewmh, win, num, atoms);
}
bool loadClassInfo(WindowID win, char* className, char* instanceName) {
    xcb_icccm_get_wm_class_reply_t prop;
    xcb_get_property_cookie_t cookie = xcb_icccm_get_wm_class(dis, win);
    if(xcb_icccm_get_wm_class_reply(dis, cookie, &prop, NULL)) {
        strcpy(className, prop.class_name);
        strcpy(instanceName, prop.instance_name);
        xcb_icccm_get_wm_class_reply_wipe(&prop);
        return 1;
    }
    return 0;
}
bool getWindowTitle(WindowID win, char* title) {
    xcb_ewmh_get_utf8_strings_reply_t wtitle;
    xcb_get_property_cookie_t cookie = xcb_ewmh_get_wm_name(ewmh, win);
    if(xcb_ewmh_get_wm_name_reply(ewmh, cookie, &wtitle, NULL)) {
        strncpy(title, wtitle.strings, MIN_NAME_LEN(wtitle.strings_len));
        xcb_ewmh_get_utf8_strings_reply_wipe(&wtitle);
        return 1;
    }
    else {
        xcb_icccm_get_text_property_reply_t icccName;
        cookie = xcb_icccm_get_wm_name(dis, win);
        if(xcb_icccm_get_wm_name_reply(dis, cookie, &icccName, NULL)) {
            strncpy(title, icccName.name, MIN_NAME_LEN(icccName.name_len));
            xcb_icccm_get_text_property_reply_wipe(&icccName);
            return 1;
        }
    }
    return 0;
}
bool loadWindowType(WindowInfo* winInfo) {
    xcb_ewmh_get_atoms_reply_t name;
    bool failed = 0;
    if(xcb_ewmh_get_wm_window_type_reply(ewmh,
            xcb_ewmh_get_wm_window_type(ewmh, winInfo->id), &name, NULL)) {
        winInfo->type = name.atoms[0];
        xcb_ewmh_get_atoms_reply_wipe(&name);
    }
    else {
        TRACE("could not read window type; using default based on transient being set to %d",
            winInfo->transientFor);
        winInfo->type = winInfo->transientFor ? ewmh->_NET_WM_WINDOW_TYPE_DIALOG : ewmh->_NET_WM_WINDOW_TYPE_NORMAL;
        failed = 1;
    }
    getAtomName(winInfo->type, winInfo->typeName);
    return !failed;
}


void loadWindowHints(WindowInfo* winInfo) {
    xcb_icccm_wm_hints_t hints;
    if(xcb_icccm_get_wm_hints_reply(dis, xcb_icccm_get_wm_hints(dis, winInfo->id), &hints, NULL)) {
        if(xcb_icccm_wm_hints_get_urgency(&hints)) {
            addMask(winInfo, URGENT_MASK);
        }
        winInfo->groupID = hints.window_group;
        if(hints.input)
            addMask(winInfo, INPUT_MASK);
        else
            removeMask(winInfo, INPUT_MASK);
    }
}
/* TODO
static void loadWindowSizeHints(WindowInfo* winInfo) {
    xcb_size_hints_t sizeHints;
    if(xcb_icccm_get_wm_size_hints_reply(dis, xcb_icccm_get_wm_size_hints(dis, winInfo->id, XCB_ATOM_WM_NORMAL_HINTS),
            &sizeHints
            , NULL))
        *getWindowSizeHints(winInfo) = sizeHints;
}
*/

/**
 * Load window protocols like WM_DELETE_WINDOW
 * @param winInfo
 */
/*TODO
static void loadProtocols(WindowInfo* winInfo) {
    xcb_icccm_get_wm_protocols_reply_t reply;
    if(xcb_icccm_get_wm_protocols_reply(dis,
            xcb_icccm_get_wm_protocols(dis, winInfo->id, ewmh->WM_PROTOCOLS),
            &reply, NULL)) {
        TRACE("Found %d protocols", reply.atoms_len);
        TRACE(getAtomsAsString(reply.atoms, reply.atoms_len));
        for(uint32_t i = 0; i < reply.atoms_len; i++)
            if(reply.atoms[i] == WM_DELETE_WINDOW)
                addMask(winInfo, WM_DELETE_WINDOW_MASK);
            else if(reply.atoms[i] == ewmh->_NET_WM_PING)
                addMask(winInfo, WM_PING_MASK);
            else if(reply.atoms[i] == WM_TAKE_FOCUS)
                addMask(winInfo, WM_TAKE_FOCUS_MASK);
            else
                DEBUG("Unsupported protocol detected %d", reply.atoms[i]);
        xcb_icccm_get_wm_protocols_reply_wipe(&reply);
    }
}
*/
void loadWindowTitle(WindowInfo* winInfo) {
    getWindowTitle(winInfo->id, winInfo->title);
}

void setWindowRole(WindowID wid, const char* s) {
    setWindowPropertyString(wid, WM_WINDOW_ROLE, XCB_ATOM_STRING, s);
}

void loadWindowRole(WindowInfo* winInfo) {
    getWindowPropertyString(winInfo->id, WM_WINDOW_ROLE, XCB_ATOM_STRING, winInfo->role);
}

void loadWindowProperties(WindowInfo* winInfo) {
    TRACE("loading window properties %d", winInfo->id);
    loadClassInfo(winInfo->id, winInfo->className, winInfo->instanceName);
    loadWindowTitle(winInfo);
    xcb_window_t prop;
    if(xcb_icccm_get_wm_transient_for_reply(dis, xcb_icccm_get_wm_transient_for(dis, winInfo->id), &prop, NULL))
        winInfo->transientFor = prop;
    bool result = !loadWindowType(winInfo);
    winInfo->implicitType = result;
    // TODO loadProtocols(winInfo);
    loadWindowHints(winInfo);
    // TODO loadWindowSizeHints(winInfo);
    loadWindowRole(winInfo);
    if(winInfo->type == ewmh->_NET_WM_WINDOW_TYPE_DOCK) {
        DEBUG("Marking window as dock");
        winInfo->dock = 1;
        loadDockProperties(winInfo);
    }
}
void setBorderColor(WindowID win, unsigned int color) {
    if(color > 0xFFFFFF) {
        WARN("Color %d is out f range", color);
        return;
    }
    XCALL(xcb_change_window_attributes, dis, win, XCB_CW_BORDER_PIXEL, &color);
    TRACE("setting border for window %d to %#08x", win, color);
}
void setBorder(WindowID win) {
    setBorderColor(win, getActiveMaster()->focusColor);
}
void resetBorder(WindowID win) {
    TimeStamp maxValue = 0;
    Master* master = NULL;
    FOR_EACH(Master*, m, getAllMasters()) {
        if(getFocusedWindowOfMaster(m) && getFocusedWindowOfMaster(m)->id == win)
            if(m->focusedTimeStamp >= maxValue) {
                master = m;
                maxValue = m->focusedTimeStamp;
            }
    }
    if(master)
        setBorderColor(win, master->focusColor);
    else
        setBorderColor(win, DEFAULT_UNFOCUS_BORDER_COLOR);
}

//TODO consider moving to devices
int focusWindow(WindowID win, Master* master) {
    DEBUG("Trying to set focus to %d for master %d", win, master->id);
    assert(win);
    xcb_void_cookie_t cookie = xcb_input_xi_set_focus_checked(dis, win, XCB_CURRENT_TIME, getKeyboardID(master));
    return !catchError(cookie);
}
int focusWindowInfo(WindowInfo* winInfo, Master* master) {
    if(!hasPartOfMask(winInfo, INPUT_MASK | WM_TAKE_FOCUS_MASK))
        return 0;
    if(focusWindow(winInfo->id, master)) {
        onWindowFocus(winInfo->id);
        if(hasMask(winInfo, WM_TAKE_FOCUS_MASK)) {
            //TODO MPX support
            uint32_t data[] = {WM_TAKE_FOCUS, getTime()};
            xcb_ewmh_send_client_message(dis, winInfo->id, winInfo->id, ewmh->WM_PROTOCOLS, 2, data);
        }
        return 1;
    }
    return 0;
}
int loadDockProperties(WindowInfo* winInfo) {
    TRACE("Reloading dock properties");
    xcb_window_t win = winInfo->id;
    xcb_ewmh_wm_strut_partial_t strut;
    if(xcb_ewmh_get_wm_strut_partial_reply(ewmh,
            xcb_ewmh_get_wm_strut_partial(ewmh, win), &strut, NULL)) {
        setDockProperties(winInfo, (int*)&strut, 1);
    }
    else if(xcb_ewmh_get_wm_strut_reply(ewmh,
            xcb_ewmh_get_wm_strut(ewmh, win),
            (xcb_ewmh_get_extents_reply_t*) &strut, NULL))
        setDockProperties(winInfo, (int*)&strut, 0);
    else {
        TRACE("could not read struct data");
        setDockProperties(winInfo, NULL, 0);
        return 0;
    }
    if(hasMask(winInfo, MAPPED_MASK))
        applyEventRules(SCREEN_CHANGE, NULL);
    return 1;
}

uint32_t getUserTime(WindowID win) {
    uint32_t timestamp = 1;
    xcb_ewmh_get_wm_user_time_window_reply(ewmh, xcb_ewmh_get_wm_user_time_window(ewmh, win), &win, NULL);
    xcb_ewmh_get_wm_user_time_reply(ewmh, xcb_ewmh_get_wm_user_time(ewmh, win), &timestamp, NULL);
    return timestamp;
}

void setUserTime(WindowID win, uint32_t time) {
    xcb_ewmh_set_wm_user_time_checked(ewmh, win, time);
}


uint32_t generateID() {
    return xcb_generate_id(dis);
}

WindowID createWindow(WindowID parent, xcb_window_class_t clazz, uint32_t mask, uint32_t* valueList,
    const Rect dims) {
    WindowID win = generateID();
    xcb_create_window(dis, XCB_COPY_FROM_PARENT, win, parent, dims.x, dims.y, dims.width, dims.height, 0, clazz,
        screen->root_visual, mask, valueList);
    return win;
}
int destroyWindow(WindowID win) {
    assert(win);
    DEBUG("Destroying window %d", win);
    return catchError(xcb_destroy_window_checked(dis, win));
}
WindowID mapWindow(WindowID id) {
    TRACE("Mapping %d", id);
    xcb_map_window(dis, id);
    return id;
}
void unmapWindow(WindowID id) {
    TRACE("UnMapping %d", id);
    xcb_unmap_window(dis, id);
}

int killClientOfWindow(WindowID win) {
    assert(win);
    DEBUG("Killing window %d", win);
    return catchError(xcb_kill_client_checked(dis, win));
}
void killClientOfWindowInfo(WindowInfo* winInfo) {
    killClientOfWindow(winInfo->id);
}

Rect getRealGeometry(WindowID id) {
    Rect rect;
    xcb_get_geometry_reply_t* reply = xcb_get_geometry_reply(dis, xcb_get_geometry(dis, id), NULL);
    if(reply) {
        rect = *(Rect*)&reply->x;
        free(reply);
    }
    return rect;
}
uint16_t getWindowBorder(WindowID id) {
    int border = 0;
    xcb_get_geometry_reply_t* reply = xcb_get_geometry_reply(dis, xcb_get_geometry(dis, id), NULL);
    if(reply) {
        border = reply->border_width;
        free(reply);
    }
    return border ;
}

