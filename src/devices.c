#include <assert.h>
#include <string.h>

#include <X11/extensions/XInput2.h>
#include <xcb/xinput.h>
#ifndef NO_XRANDR
#include <xcb/randr.h>
#endif

#include "util/arraylist.h"
#include "boundfunction.h"
#include "devices.h"
#include "globals.h"
#include "util/logger.h"
#include "masters.h"
#include "monitors.h"
#include "system.h"
#include "time.h"
#include "user-events.h"
#include "xutil/xsession.h"


#pragma GCC diagnostic ignored "-Wnarrowing"

void createMasterDevice(const char* name) {
    XIAddMasterInfo add = {.type = XIAddMaster, .name = (char*)name, .send_core = 1, .enable = 1};
    XIChangeHierarchy(dpy, (XIAnyHierarchyChangeInfo*)&add, 1);
    //xcb_input_xi_change_hierarchy(c, num_changes, changes);
}
void attachSlaveToMaster(Slave* slave, Master* master) {
    assert(!isTestDevice(slave->name));
    if(master)
        attachSlaveToMasterDevice(slave->id, slave->keyboard ? getKeyboardID(master) : getPointerID(master));
    else floatSlave(slave->id);
}
void floatSlave(SlaveID slaveID) {
    INFO("floating %d ", slaveID);
    XIAnyHierarchyChangeInfo changes;
    changes.type = XIDetachSlave;
    changes.attach.deviceid = slaveID;
    XIChangeHierarchy(dpy, &changes, 1);
}
void attachSlaveToMasterDevice(SlaveID slaveID, MasterID masterID) {
    INFO("attaching %d to %d", slaveID, masterID);
    XIAnyHierarchyChangeInfo changes;
    changes.type = XIAttachSlave;
    changes.attach.deviceid = slaveID;
    changes.attach.new_master = masterID;
    XIChangeHierarchy(dpy, &changes, 1);
}
void destroyMasterDevice2(MasterID id, int returnPointer, int returnKeyboard) {
    assert(id != DEFAULT_KEYBOARD && id != DEFAULT_POINTER);
    XIRemoveMasterInfo remove = {
        .type = XIRemoveMaster,
        .deviceid = id,
        .return_mode = XIAttachToMaster,
        .return_pointer = returnPointer,
        .return_keyboard = returnKeyboard,
    };
    XIChangeHierarchy(dpy, (XIAnyHierarchyChangeInfo*)&remove, 1);
}

void destroyAllNonDefaultMasters(void) {
    FOR_EACH(Master*, master, getAllMasters()) {
        if(getKeyboardID(master) != DEFAULT_KEYBOARD)
            destroyMasterDevice(getKeyboardID(master));
    }
}

void destroyAllNonEmptyMasters(void) {
    FOR_EACH(Master*, master, getAllMasters()) {
        if(getKeyboardID(master) != DEFAULT_KEYBOARD && getSlaves(master)->size)
            destroyMasterDevice(getKeyboardID(master));
    }
}

void registerMasterDevice(MasterID id) {
    /*
    xcb_input_xi_query_device_cookie_t cookie=xcb_input_xi_query_device(dis, XIAllMasterDevices);
    xcb_input_xi_query_device_reply_t *reply=xcb_input_xi_query_device_reply(dis, cookie, NULL);

    xcb_input_xi_device_info_iterator_t *iter= xcb_input_xi_query_device_infos_iterator(reply);

    while(iter->rem){
        xcb_input_xi_device_info_t info=iter->data;
        xcb_input_xi_device_info_next(iter);
        xcb_input_xi_device_it
        xcb_input_xi_iter_n
        xcb_randr_monitor_info_next
        //detectMonitors()
    }*/
    //xcb_input_xi_query_device(c, deviceid);
    int ndevices;
    XIDeviceInfo* devices, *device;
    devices = XIQueryDevice(dpy, id, &ndevices);
    DEBUG("Detected %d devices", ndevices);
    for(int i = 0; i < ndevices; i++) {
        device = &devices[i];
        switch(device->use) {
            case XIMasterPointer:
                continue;
            case XISlaveKeyboard:
            case XISlavePointer :
            case XIFloatingSlave:
                if(!isTestDevice(device->name)) {
                    Slave* slave = findElement(getAllSlaves(), &device->deviceid, sizeof(MasterID));
                    if(slave) {
                        setMasterForSlave(slave, device->attachment);
                    }
                    else
                        newSlave(device->deviceid, device->attachment, device->use == XISlaveKeyboard, device->name);
                }
                break;
            case XIMasterKeyboard: {
                if(findElement(getAllMasters(), &device->deviceid, sizeof(MasterID)))
                    continue;
                int lastSpace = 0;
                for(int n = 0; device->name[n]; n++)
                    if(device->name[n] == ' ')
                        lastSpace = n;
                if(lastSpace)
                    device->name[lastSpace] = 0;
                newMaster(device->deviceid, device->attachment, device->name);
            }
        }
    }
    XIFreeDeviceInfo(devices);
    assert(getAllMasters()->size >= 1);
    assert(getActiveMaster());
}
void initCurrentMasters() {
    registerMasterDevice(XIAllDevices);
}

Master* getMasterByDeviceID(MasterID id) {
    Master* master = getMasterByID(id);
    if(!master) {
        Slave* slave = findElement(getAllSlaves(), &id, sizeof(MasterID));
        if(slave)
            master = getMasterForSlave(slave);
    }
    return master;
}
bool setActiveMasterByDeviceID(MasterID id) {
    Master* master = getMasterByDeviceID(id);
    if(master)
        setActiveMaster(master);
    return master;
}


bool getMousePosition(MasterID id, int relativeWindow, int16_t result[2]) {
    xcb_input_xi_query_pointer_reply_t* reply =
        xcb_input_xi_query_pointer_reply(dis, xcb_input_xi_query_pointer(dis, relativeWindow, id), NULL);
    assert(reply);
    if(reply) {
        result[0] = reply->win_x >> 16;
        result[1] = reply->win_y >> 16;
        DEBUG("Mouse position for Master %d is %d, %d", id, result[0], result[1]);
        free(reply);
    }
    return reply ? 1 : 0;
}
/*
static int getAssociatedMasterDevice(int deviceID) {
    int ndevices;
    XIDeviceInfo* masterDevices;
    MasterID id;
    masterDevices = XIQueryDevice(dpy, deviceID, &ndevices);
    id = masterDevices[0].attachment;
    XIFreeDeviceInfo(masterDevices);
    return id;
}
*/
void setClientPointerForWindow(WindowID window, MasterID id) {
    XCALL(xcb_input_xi_set_client_pointer, dis, window, id);
}
MasterID getClientPointerForWindow(WindowID win) {
    MasterID masterPointer;
    XIGetClientPointer(dpy, win, (int*)&masterPointer);
    return masterPointer;
}
Master* getClientMaster(WindowID win) {
    return getMasterByID(getClientPointerForWindow(win));
}
WindowID getActiveFocusOfMaster(MasterID id) {
    WindowID win = 0;
    xcb_input_xi_get_focus_reply_t* reply;
    reply = xcb_input_xi_get_focus_reply(dis, xcb_input_xi_get_focus(dis, id), NULL);
    if(reply) {
        win = reply->focus;
        free(reply);
    }
    return win;
}
void detectMonitors(void) {
#ifdef NO_XRANDR
    addRootMonitor();
#else
    DEBUG("refreshing monitors");
    xcb_randr_get_monitors_cookie_t cookie = xcb_randr_get_monitors(dis, root, 1);
    xcb_randr_get_monitors_reply_t* monitors = xcb_randr_get_monitors_reply(dis, cookie, NULL);
    assert(monitors);
    xcb_randr_monitor_info_iterator_t iter = xcb_randr_get_monitors_monitors_iterator(monitors);
    while(iter.rem) {
        xcb_randr_monitor_info_t* monitorInfo = iter.data;
        Monitor* m = findElement(getAllMonitors(), &monitorInfo->name, sizeof(monitorInfo->name));
        if(!m) {
            DEBUG("New monitor detected: %d", monitorInfo->name);
            m = newMonitor(monitorInfo->name, *(Rect*)&monitorInfo->x, "", 0);
        }
        else
            setBase(m, *(Rect*)&monitorInfo->x);
        getAtomName(monitorInfo->name, m->name);
        if(monitorInfo->primary)
            setPrimary(m->id);
        m->_mark = 1;
        xcb_randr_monitor_info_next(&iter);
    }
    free(monitors);
    if(getPrimaryMonitor())
        DEBUG("Primary is %d", getPrimaryMonitor()->id);
    FOR_EACH_R(Monitor*, m, getAllMonitors()) {
        if(!m->fake && !m->_mark)
            freeMonitor(m);
        else
            m->_mark = 0;
    }
#endif
    removeDuplicateMonitors();
    if(ASSUME_PRIMARY_MONITOR && !getPrimaryMonitor() && getAllMonitors()->size ) {
        setPrimary(((Monitor*)getHead(getAllMonitors()))->id);
    }
    DEBUG("Number of monitors after consolidation %d", getAllMonitors()->size);
}
