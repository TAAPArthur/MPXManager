/**
 * @file devices.c
 * @copybrief devices.h
 */


#include <assert.h>
#include <string.h>

#include <X11/extensions/XInput2.h>
#include <xcb/xinput.h>
#ifndef NO_XRANDR
    #include <xcb/randr.h>
#endif

#include "monitors.h"
#include "arraylist.h"
#include "devices.h"
#include "globals.h"
#include "logger.h"
#include "masters.h"
#include "test-functions.h"
#include "xsession.h"
#include "monitors.h"
#include "system.h"
#include "time.h"
using namespace std;


#pragma GCC diagnostic ignored "-Wnarrowing"

void createMasterDevice(std::string name) {
    XIAddMasterInfo add = {.type = XIAddMaster, .name = (char*)name.c_str(), .send_core = 1, .enable = 1};
    XIChangeHierarchy(dpy, (XIAnyHierarchyChangeInfo*)&add, 1);
    //xcb_input_xi_change_hierarchy(c, num_changes, changes);
}
void attachSlaveToMaster(Slave* slave, Master* master) {
    assert(!Slave::isTestDevice(slave->getName()));
    attachSlaveToMaster(slave->getID(), slave->isKeyboardDevice() ? master->getKeyboardID() : master->getPointerID());
}
void attachSlaveToMaster(int slaveId, int masterId) {
    LOG(LOG_LEVEL_DEBUG, "attaching %d to %d\n", slaveId, masterId);
    XIAnyHierarchyChangeInfo changes;
    changes.type = XIAttachSlave;
    changes.attach.deviceid = slaveId;
    changes.attach.new_master = masterId;
    XIChangeHierarchy(dpy, &changes, 1);
}
void destroyMasterDevice(MasterID id, int returnPointer, int returnKeyboard) {
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
    for(Master* master : getAllMasters()) {
        if(master->getKeyboardID() != DEFAULT_KEYBOARD)
            destroyMasterDevice(master->getKeyboardID(), DEFAULT_POINTER, DEFAULT_KEYBOARD);
    }
}

void initCurrentMasters() {
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
    devices = XIQueryDevice(dpy, XIAllDevices, &ndevices);
    ArrayList<MasterID>masters;
    getAllSlaves().deleteElements();
    for(int i = 0; i < ndevices; i++) {
        device = &devices[i];
        if(device->use == XIMasterPointer)
            continue;
        if(device->use == XISlaveKeyboard || device->use == XISlavePointer) {
            if(!Slave::isTestDevice(device->name)) {
                Slave* slave = new Slave{device->deviceid, device->attachment, device->use == XISlaveKeyboard, device->name};
                getAllSlaves().add(slave);
            }
        }
        if(device->use == XIMasterKeyboard) {
            int lastSpace = 0;
            for(int n = 0; device->name[n]; n++)
                if(device->name[n] == ' ')
                    lastSpace = n;
            if(lastSpace)
                device->name[lastSpace] = 0;
            Master* master = new Master(device->deviceid, device->attachment, device->name);
            masters.add(device->deviceid);
            if(!getAllMasters().addUnique(master))
                delete master;
        }
    }
    XIFreeDeviceInfo(devices);
    for(int i = getAllMasters().size() - 1; i >= 0; i--)
        if(!masters.find(getAllMasters()[i]->getID()))
            delete getAllMasters().remove(i);
    assert(getAllMasters().size() >= 1);
    assert(getActiveMaster());
}
void setActiveMasterByDeviceId(MasterID id) {
    Master* master = getMasterById(id, 1);
    if(!master)
        master = getMasterById(id, 0);
    if(!master) {
        Slave* slave = getAllSlaves().find(id);
        if(slave)
            master = slave->getMaster();
    }
    if(master)
        setActiveMaster(master);
    assert(getActiveMaster());
}


bool getMousePosition(MasterID id, int relativeWindow, int16_t result[2]) {
    xcb_input_xi_query_pointer_reply_t* reply1 =
        xcb_input_xi_query_pointer_reply(dis, xcb_input_xi_query_pointer(dis, relativeWindow, id), NULL);
    if(reply1) {
        result[0] = reply1->win_x >> 16;
        result[1] = reply1->win_y >> 16;
        free(reply1);
    }
    return reply1 ? 1 : 0;
}
static int getAssociatedMasterDevice(int deviceId) {
    int ndevices;
    XIDeviceInfo* masterDevices;
    MasterID id;
    masterDevices = XIQueryDevice(dpy, deviceId, &ndevices);
    id = masterDevices[0].attachment;
    XIFreeDeviceInfo(masterDevices);
    return id;
}
void setClientPointerForWindow(WindowID window, MasterID id) {
    xcb_input_xi_set_client_pointer(dis, window, id);
}
MasterID getClientKeyboard(WindowID win) {
    MasterID masterPointer;
    XIGetClientPointer(dpy, win, (int*)&masterPointer);
    Master* master = getMasterById(masterPointer, 0);
    if(master)
        return master->getKeyboardID();
    else {
        return getAssociatedMasterDevice(masterPointer);
    }
}
WindowID getActiveFocus(MasterID id) {
    WindowID win = 0;
    xcb_input_xi_get_focus_reply_t* reply;
    reply = xcb_input_xi_get_focus_reply(dis, xcb_input_xi_get_focus(dis, id), NULL);
    if(reply) {
        win = reply->focus;
        free(reply);
    }
    return win;
}
void swapDeviceId(Master* master1, Master* master2) {
    if(master1 == master2)
        return;
    LOG(LOG_LEVEL_DEBUG, "Swapping %d(%d) with %d (%d)\n",
        master1->getID(), master1->getPointerID(), master2->getID(), master2->getPointerID());
    //swap keyboard focus
    xcb_input_xi_set_focus(dis, getActiveFocus(master1->getID()), 0, master2->getID());
    xcb_input_xi_set_focus(dis, getActiveFocus(master2->getID()), 0, master2->getID());
    short pos1[2];
    short pos2[2];
    getMousePosition(master1->getPointerID(), root, pos1);
    getMousePosition(master2->getPointerID(), root, pos2);
    movePointer(master2->getPointerID(), root, pos1[0], pos1[1]);
    movePointer(master1->getPointerID(), root, pos2[0], pos2[1]);
    for(Slave* slave : master1->getSlaves()) {
        attachSlaveToMaster(slave, master2);
    }
    for(Slave* slave : master2->getSlaves()) {
        attachSlaveToMaster(slave, master1);
    }
    flush();
}
void clearFakeMonitors() {
    waitForChild(spawn("xsane-xrandr clear &>/dev/null"));
    detectMonitors();
}
void createFakeMonitor(Rect bounds) {
    char buffer[255];
    sprintf(buffer, "xsane-xrandr add-monitor %d %d %d %d  &>/dev/null", bounds.x, bounds.y, bounds.width, bounds.height);
    waitForChild(spawn(buffer));
}
void detectMonitors(void) {
#ifdef NO_XRANDR
    addRootMonitor();
    removeDuplicateMonitors();
#else
    LOG(LOG_LEVEL_DEBUG, "refreshing monitors\n");
    xcb_randr_get_monitors_cookie_t cookie = xcb_randr_get_monitors(dis, root, 1);
    xcb_randr_get_monitors_reply_t* monitors = xcb_randr_get_monitors_reply(dis, cookie, NULL);
    assert(monitors);
    xcb_randr_monitor_info_iterator_t iter = xcb_randr_get_monitors_monitors_iterator(monitors);
    ArrayList<MonitorID> monitorNames;
    TimeStamp t = getTime();
    while(iter.rem) {
        xcb_randr_monitor_info_t* monitorInfo = iter.data;
        Monitor* m = getAllMonitors().find(monitorInfo->name);
        if(!m) {
            m = new Monitor(monitorInfo->name, &monitorInfo->x);
            getAllMonitors().add(m);
        }
        else m->setBase(*(Rect*)&monitorInfo->x);
        m->setTimeStamp(t);
        m->setName(getAtomValue(monitorInfo->name));
        m->setPrimary(monitorInfo->primary);
        monitorNames.add(monitorInfo->name);
        xcb_randr_monitor_info_next(&iter);
    }
    free(monitors);
    for(int i = getAllMonitors().size() - 1; i >= 0; i--)
        if(getAllMonitors()[i]->getTimeStamp() < t)
            delete getAllMonitors().remove(i);
#endif
    assert(getAllMonitors().size() > 0);
    removeDuplicateMonitors();
    assignUnusedMonitorsToWorkspaces();
    resizeAllMonitorsToAvoidAllDocks();
    assert(getAllMonitors().size() > 0);
}
