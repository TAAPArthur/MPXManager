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
    if(master)
        attachSlaveToMaster(slave->getID(), slave->isKeyboardDevice() ? master->getKeyboardID() : master->getPointerID());
    else floatSlave(slave->getID());
}
void floatSlave(SlaveID slaveID) {
    LOG(LOG_LEVEL_INFO, "floating %d \n", slaveID);
    XIAnyHierarchyChangeInfo changes;
    changes.type = XIDetachSlave;
    changes.attach.deviceid = slaveID;
    XIChangeHierarchy(dpy, &changes, 1);
}
void attachSlaveToMaster(SlaveID slaveID, MasterID masterID) {
    LOG(LOG_LEVEL_INFO, "attaching %d to %d\n", slaveID, masterID);
    XIAnyHierarchyChangeInfo changes;
    changes.type = XIAttachSlave;
    changes.attach.deviceid = slaveID;
    changes.attach.new_master = masterID;
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
    LOG(LOG_LEVEL_DEBUG, "Detected %d devices\n", ndevices);
    for(int i = 0; i < ndevices; i++) {
        device = &devices[i];
        switch(device->use) {
            case XIMasterPointer:
                continue;
            case XISlaveKeyboard:
            case XISlavePointer :
            case XIFloatingSlave:
                if(!Slave::isTestDevice(device->name)) {
                    Slave* slave = new Slave{device->deviceid, device->attachment, device->use == XISlaveKeyboard, device->name};
                    getAllSlaves().add(slave);
                }
                break;
            case XIMasterKeyboard: {
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
    }
    XIFreeDeviceInfo(devices);
    for(int i = getAllMasters().size() - 1; i >= 0; i--)
        if(!masters.find(getAllMasters()[i]->getID()))
            delete getAllMasters().remove(i);
    assert(getAllMasters().size() >= 1);
    assert(getActiveMaster());
}
Master* getMasterByDeviceID(MasterID id) {
    Master* master = getMasterByID(id, 1);
    if(!master)
        master = getMasterByID(id, 0);
    if(!master) {
        Slave* slave = getAllSlaves().find(id);
        if(slave)
            master = slave->getMaster();
    }
    return master;
}
void setActiveMasterByDeviceID(MasterID id) {
    Master* master = getMasterByDeviceID(id);
    if(master)
        setActiveMaster(master);
    assert(getActiveMaster());
}


bool getMousePosition(MasterID id, int relativeWindow, int16_t result[2]) {
    xcb_input_xi_query_pointer_reply_t* reply =
        xcb_input_xi_query_pointer_reply(dis, xcb_input_xi_query_pointer(dis, relativeWindow, id), NULL);
    assert(reply);
    if(reply) {
        result[0] = reply->win_x >> 16;
        result[1] = reply->win_y >> 16;
        LOG(LOG_LEVEL_DEBUG, "Mouse position for Master %d is %d, %d\n", id, result[0], result[1]);
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
    xcb_input_xi_set_client_pointer(dis, window, id);
}
MasterID getClientPointerForWindow(WindowID win) {
    MasterID masterPointer;
    XIGetClientPointer(dpy, win, (int*)&masterPointer);
    return masterPointer;
}
Master* getClientMaster(WindowID win) {
    return getMasterByID(getClientPointerForWindow(win), 0);
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
void removeAllFakeMonitors() {
    for(int i = getAllMonitors().size() - 1; i >= 0; i--)
        if(getAllMonitors()[i]->isFake())
            delete getAllMonitors().remove(i);
}
Monitor* addFakeMonitor(Rect bounds, std::string name) {
    MonitorID id;
    for(id = 2;; id++)
        if(!getAllMonitors().find(id))
            break;
    Monitor* m = new Monitor(id, bounds, 0, name, 1);
    getAllMonitors().add(m);
    return m;
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
    while(iter.rem) {
        xcb_randr_monitor_info_t* monitorInfo = iter.data;
        Monitor* m = getAllMonitors().find(monitorInfo->name);
        if(!m) {
            m = new Monitor(monitorInfo->name, &monitorInfo->x);
            getAllMonitors().add(m);
        }
        else m->setBase(*(Rect*)&monitorInfo->x);
        m->setName(getAtomName(monitorInfo->name));
        m->setPrimary(monitorInfo->primary);
        monitorNames.add(monitorInfo->name);
        xcb_randr_monitor_info_next(&iter);
    }
    LOG(getAllMonitors().size() ? LOG_LEVEL_DEBUG : LOG_LEVEL_WARN, "Detected %d monitors\n", monitorNames.size());
    free(monitors);
    for(int i = getAllMonitors().size() - 1; i >= 0; i--)
        if(!getAllMonitors()[i]->isFake() && !monitorNames.find(getAllMonitors()[i]->getID()))
            delete getAllMonitors().remove(i);
#endif
    removeDuplicateMonitors();
    assignUnusedMonitorsToWorkspaces();
}
