#include <assert.h>
#include <string.h>

#include <xcb/xinput.h>
#ifndef NO_XRANDR
#include <xcb/randr.h>
#endif

#include "../util/logger.h"
#include "device-grab.h"
#include "xsession.h"

/**
 * @param id
 *
 * @return 1 iff id is 0 or 1
 */
static inline bool isSpecialID(MasterID id) {
    return id == XCB_INPUT_DEVICE_ALL_MASTER || id == XCB_INPUT_DEVICE_ALL;
}

void passiveGrab(WindowID window, uint32_t maskValue) {
    TRACE("Passively Selecting %d  on window %d", maskValue, window);
    struct {
        xcb_input_event_mask_t iem;
        xcb_input_xi_event_mask_t xiem;
    } se_mask;
    se_mask.iem.deviceid = XCB_INPUT_DEVICE_ALL;
    se_mask.iem.mask_len = 1;

    se_mask.xiem = maskValue;
    xcb_input_xi_select_events(dis, window, 1, &se_mask.iem);
}
void passiveUngrab(WindowID window) {
    passiveGrab(window, 0);
}

int grabDevice(MasterID deviceID, uint32_t maskValue) {
    assert(!isSpecialID(deviceID));
    INFO("Grabbing device %d with mask %d", deviceID, maskValue);
    xcb_input_xi_grab_device_reply_t* reply;
    reply = xcb_input_xi_grab_device_reply(dis, xcb_input_xi_grab_device(dis, root, XCB_CURRENT_TIME, XCB_NONE, deviceID, XCB_INPUT_GRAB_MODE_22_ASYNC, XCB_INPUT_GRAB_MODE_22_ASYNC, 1,  1, &maskValue), NULL);
    free(reply);
    return reply->status;
}

int ungrabDevice(MasterID id) {
    xcb_input_xi_ungrab_device(dis, XCB_CURRENT_TIME, id);
    return 1;
}

int grabDetail(MasterID deviceID, uint32_t detail, uint32_t mod, uint32_t maskValue, uint32_t ignoreMod) {
    uint32_t modifiers[4] = {mod, mod | IGNORE_MASK, mod | ignoreMod, mod | IGNORE_MASK | ignoreMod };
    DEBUG("Grabbing detail on %d detail:%d mod:%d mask: %d", deviceID, detail, mod, maskValue);
    int size = ignoreMod ? LEN(modifiers) : LEN(modifiers) / 2;
    xcb_input_grab_type_t grabType = getKeyboardMask(maskValue) ? XCB_INPUT_GRAB_TYPE_KEYCODE: XCB_INPUT_GRAB_TYPE_BUTTON;
    xcb_input_xi_passive_grab_device_reply_t *reply;
    reply = xcb_input_xi_passive_grab_device_reply(dis, xcb_input_xi_passive_grab_device(dis,XCB_CURRENT_TIME,root,XCB_CURSOR_NONE, detail, deviceID, size, 1, grabType, XCB_INPUT_GRAB_MODE_22_ASYNC, XCB_INPUT_GRAB_MODE_22_ASYNC, XCB_INPUT_GRAB_OWNER_OWNER, &maskValue, modifiers), NULL);

    int errors = LEN(modifiers);
    if(reply) {
        errors -= reply->num_modifiers;
        free(reply);
    }
    return errors;
}
int ungrabDetail(MasterID deviceID, uint32_t detail, uint32_t mod, uint32_t ignoreMod, bool isKeyboard) {
    DEBUG("UNGrabbing device:%d detail:%d mod:%d %d",
        deviceID, detail, mod, isKeyboard);
    uint32_t modifiers[4] = {mod, mod | IGNORE_MASK, mod | ignoreMod, mod | IGNORE_MASK | ignoreMod };
    xcb_input_grab_type_t grabType = isKeyboard ? XCB_INPUT_GRAB_TYPE_KEYCODE: XCB_INPUT_GRAB_TYPE_BUTTON;

    xcb_input_xi_passive_ungrab_device(dis, root, detail, deviceID, LEN(modifiers), grabType, modifiers);
    return 0;
}
void replayPointerEvent() {
    TRACE("Replaying pointer events");
    xcb_allow_events(dis, XCB_ALLOW_REPLAY_POINTER, XCB_CURRENT_TIME);
}
void replayKeyboardEvent() {
    TRACE("Replaying keyboard event");
    xcb_allow_events(dis, XCB_ALLOW_REPLAY_KEYBOARD, XCB_CURRENT_TIME);
}

int registerForWindowEvents(WindowID window, int mask) {
    xcb_void_cookie_t cookie;
    cookie = xcb_change_window_attributes_checked(dis, window, XCB_CW_EVENT_MASK, &mask);
    return catchErrorSilent(cookie);
}
int unregisterForWindowEvents(WindowID window) {
    return registerForWindowEvents(window, 0);
}
