#include <assert.h>
#include <string.h>

#include <X11/extensions/XInput2.h>
#include <xcb/xinput.h>
#ifndef NO_XRANDR
    #include <xcb/randr.h>
#endif

#include "device-grab.h"
#include "logger.h"
#include "xsession.h"

#pragma GCC diagnostic ignored "-Wnarrowing"

void passiveGrab(WindowID window, uint32_t maskValue) {
    XIEventMask eventmask = {XIAllDevices, 4, (unsigned char*)& maskValue};
    XISelectEvents(dpy, window, &eventmask, 1);
}
void passiveUngrab(WindowID window) {
    passiveGrab(window, 0);
}

int grabDevice(MasterID deviceID, uint32_t maskValue) {
    assert(!isSpecialID(deviceID));
    XIEventMask eventmask = {deviceID, 4, (unsigned char*)& maskValue};
    LOG(LOG_LEVEL_INFO, "Grabbing device %d with mask %d\n", deviceID, maskValue);
    return XIGrabDevice(dpy, deviceID,  root, CurrentTime, None, GrabModeAsync,
                        GrabModeAsync, 1, &eventmask);
}
int ungrabDevice(MasterID id) {
    LOG(LOG_LEVEL_INFO, "Ungrabbing device %d\n", id);
    return XIUngrabDevice(dpy, id, 0);
}

int grabDetail(MasterID deviceID, uint32_t detail, uint32_t mod, uint32_t maskValue) {
    XIEventMask eventmask = {deviceID, 2, (unsigned char*)& maskValue};
    XIGrabModifiers modifiers[2] = {{mod}, {mod | IGNORE_MASK}};
    LOG(LOG_LEVEL_VERBOSE, "Grabbing device:%d detail:%d mod:%d mask: %d %d\n",
        deviceID, detail, mod, maskValue, isKeyboardMask(maskValue));
    if(!isKeyboardMask(maskValue))
        return XIGrabButton(dpy, deviceID, detail, root, 0,
                            XIGrabModeAsync, XIGrabModeAsync, 1, &eventmask, 2, modifiers);
    else
        return XIGrabKeycode(dpy, deviceID, detail, root, XIGrabModeAsync, XIGrabModeAsync,
                             1, &eventmask, 2, modifiers);
}
int ungrabDetail(MasterID deviceID, uint32_t detail, uint32_t mod, bool isKeyboard) {
    LOG(LOG_LEVEL_TRACE, "UNGrabbing device:%d detail:%d mod:%d %d\n",
        deviceID, detail, mod, isKeyboard);
    XIGrabModifiers modifiers[2] = {{mod}, {mod | IGNORE_MASK}};
    if(!isKeyboard)
        return XIUngrabButton(dpy, deviceID, detail, root, 2, modifiers);
    else
        return XIUngrabKeycode(dpy, deviceID, detail, root, 2, modifiers);
}
void registerForMonitorChange() {
#ifndef NO_XRANDR
    catchError(xcb_randr_select_input_checked(dis, root, XCB_RANDR_NOTIFY_MASK_SCREEN_CHANGE));
#endif
}
int registerForWindowEvents(WindowID window, int mask) {
    xcb_void_cookie_t cookie;
    cookie = xcb_change_window_attributes_checked(dis, window, XCB_CW_EVENT_MASK, &mask);
    return catchErrorSilent(cookie);
}
