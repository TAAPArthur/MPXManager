#include <assert.h>
#include <string.h>

#include <xcb/xinput.h>
#ifndef NO_XRANDR
#include <xcb/randr.h>
#endif

#include "boundfunction.h"
#include "devices.h"
#include "globals.h"
#include "masters.h"
#include "monitors.h"
#include "system.h"
#include "time.h"
#include "user-events.h"
#include "util/arraylist.h"
#include "util/logger.h"
#include "xutil/xsession.h"


#pragma GCC diagnostic ignored "-Wnarrowing"

typedef struct {
    xcb_input_hierarchy_change_t change;
    xcb_input_hierarchy_change_data_t data;
} ChangeHierachyHolder;
//holder = {{XCB_INPUT_HIERARCHY_CHANGE_TYPE_ADD_MASTER, sizeof(xcb_input_hierarchy_change_data_t)}, {.add_master={strlen(name), .send_core = 1, .enable = 1, .name=(char*)name } }};
//xcb_input_xi_change_hierarchy(dis, 1, &holder.change);

static char hierachy_buffer[sizeof(xcb_input_hierarchy_change_t) + sizeof(xcb_input_hierarchy_change_data_t ) + MAX_NAME_LEN];

static inline xcb_input_hierarchy_change_t* create_hierarchy_change_struct(uint16_t type, xcb_input_hierarchy_change_data_t** data_ptr) {
    xcb_input_hierarchy_change_t* change = (void*)hierachy_buffer;
    xcb_input_hierarchy_change_data_t* data = (void*)(change + 1);
    change->type = type;
    change->len = 2;
    *data_ptr = data;
    return change;
}
void createMasterDevice(const char* name) {
    xcb_input_hierarchy_change_data_t* data;
    xcb_input_hierarchy_change_t* change;
    change = create_hierarchy_change_struct(XCB_INPUT_HIERARCHY_CHANGE_TYPE_ADD_MASTER, &data);
    char* name_start = ((char*)data)+4;
    strncpy(name_start, name,MAX_NAME_LEN - 1);
    data->add_master.name_len = strlen(name_start);
    data->add_master.send_core = 1;
    data->add_master.enable = 1;
    xcb_input_xi_change_hierarchy(dis, 1, change);
    //xcb_input_xi_change_hierarchy(c, num_changes, changes);
}
void attachSlaveToMaster(Slave* slave, Master* master) {
    assert(!isTestDevice(slave->name, strlen(slave->name)));
    if(master)
        attachSlaveToMasterDevice(slave->id, slave->keyboard ? getKeyboardID(master) : getPointerID(master));
    else floatSlave(slave->id);
}
void floatSlave(SlaveID slaveID) {
    xcb_input_hierarchy_change_data_t* data;
    xcb_input_hierarchy_change_t* change;
    change = create_hierarchy_change_struct(XCB_INPUT_HIERARCHY_CHANGE_TYPE_DETACH_SLAVE, &data);
    data->detach_slave.deviceid=slaveID;
    memcpy(data, &data->detach_slave, sizeof(data->detach_slave));
    INFO("floating %d", slaveID);
    xcb_input_xi_change_hierarchy(dis, 1, change);
}
void attachSlaveToMasterDevice(SlaveID slaveID, MasterID masterID) {
    INFO("attaching %d to %d", slaveID, masterID);
    xcb_input_hierarchy_change_data_t* data;
    xcb_input_hierarchy_change_t* change;
    change = create_hierarchy_change_struct(XCB_INPUT_HIERARCHY_CHANGE_TYPE_ATTACH_SLAVE, &data);
    data->attach_slave.deviceid = slaveID;
    data->attach_slave.master = masterID;
    memcpy(data, &data->attach_slave, sizeof(data->attach_slave));
    xcb_input_xi_change_hierarchy(dis, 1, change);
}
void destroyMasterDevice2(MasterID id, int returnPointer, int returnKeyboard) {
    assert(id != DEFAULT_KEYBOARD && id != DEFAULT_POINTER);
    xcb_input_hierarchy_change_data_t* data;
    xcb_input_hierarchy_change_t* change;
    change = create_hierarchy_change_struct(XCB_INPUT_HIERARCHY_CHANGE_TYPE_REMOVE_MASTER, &data);
    data->remove_master.return_mode = XCB_INPUT_HIERARCHY_CHANGE_TYPE_ADD_MASTER;
    data->remove_master.deviceid = id;
    data->remove_master.return_pointer = returnPointer;
    data->remove_master.return_keyboard = returnKeyboard;
    memcpy(data, &data->remove_master, sizeof(data->remove_master));
    INFO("Removing %d", id);
    xcb_input_xi_change_hierarchy(dis, 1, change);
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

void initCurrentMasters() {
    xcb_input_xi_query_device_cookie_t cookie = xcb_input_xi_query_device(dis, XCB_INPUT_DEVICE_ALL);
    xcb_input_xi_query_device_reply_t *reply = xcb_input_xi_query_device_reply(dis, cookie, NULL);
    xcb_input_xi_device_info_iterator_t  iter = xcb_input_xi_query_device_infos_iterator(reply);

    while(iter.rem){
        xcb_input_xi_device_info_t* device = iter.data;
        const char* deviceName = xcb_input_xi_device_info_name (device);
        switch(device->type) {
            case XCB_INPUT_DEVICE_TYPE_MASTER_POINTER:
                break;
            case XCB_INPUT_DEVICE_TYPE_SLAVE_KEYBOARD:
            case XCB_INPUT_DEVICE_TYPE_SLAVE_POINTER:
            case XCB_INPUT_DEVICE_TYPE_FLOATING_SLAVE:
                if(!isTestDevice(deviceName, device->name_len)) {
                    Slave* slave = findElement(getAllSlaves(), &device->deviceid, MIN(sizeof(MasterID), sizeof(xcb_input_device_id_t)));
                    if(slave) {
                        setMasterForSlave(slave, device->attachment);
                    }
                    else
                        newSlave(device->deviceid, device->attachment, device->type == XCB_INPUT_DEVICE_TYPE_SLAVE_KEYBOARD, deviceName, device->name_len);
                }
                break;
            case XCB_INPUT_DEVICE_TYPE_MASTER_KEYBOARD: {
                if(findElement(getAllMasters(), &device->deviceid, MIN(sizeof(MasterID), sizeof(xcb_input_device_id_t))))
                    break;
                const char*c = strstr(deviceName, " ");
                newMaster(device->deviceid, device->attachment, deviceName, c ? c - deviceName : device->name_len);
            }
        }
        xcb_input_xi_device_info_next (&iter);
    }
    free(reply);
    assert(getAllMasters()->size >= 1);
    assert(getActiveMaster());
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
void setClientPointerForWindow(WindowID window, MasterID id) {
    XCALL(xcb_input_xi_set_client_pointer, dis, window, id);
}
MasterID getClientPointerForWindow(WindowID win) {
    xcb_input_xi_get_client_pointer_reply_t* reply;
    reply = xcb_input_xi_get_client_pointer_reply(dis, xcb_input_xi_get_client_pointer(dis, win), NULL);

    MasterID masterPointer = reply ? reply->deviceid: DEFAULT_POINTER;
    free(reply);
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
