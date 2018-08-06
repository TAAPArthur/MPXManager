/**
 * @file devices.c
 */

/// \cond
#include <assert.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>

#include <xcb/xinput.h>
#include <X11/extensions/XInput2.h>
/// \endcond

#include "devices.h"

#include "globals.h"
#include "logger.h"
#include "mywm-util.h"

//Master device methods
int getAssociatedMasterDevice(int deviceId){
    int ndevices;
    XIDeviceInfo *masterDevices;
    int id;
    masterDevices = XIQueryDevice(dpy, deviceId, &ndevices);
    id=masterDevices[0].attachment;
    XIFreeDeviceInfo(masterDevices);
    return id;
}

void setClientPointerForWindow(int window){
    xcb_input_xi_set_client_pointer(dis, window, getActiveMasterPointerID());
}
int getClientKeyboard(int win){
    int masterPointer;
    XIGetClientPointer(dpy, win, &masterPointer);
    return getAssociatedMasterDevice(masterPointer);
}

void destroyMasterDevice(int id,int returnPointer,int returnKeyboard){
    XIRemoveMasterInfo remove;
    remove.type = XIRemoveMaster;
    remove.deviceid = id;
    remove.return_mode = XIAttachToMaster;
    remove.return_pointer = returnPointer;
    remove.return_keyboard = returnKeyboard;
    XIChangeHierarchy(dpy, (XIAnyHierarchyChangeInfo*)&remove, 1);
}
void createMasterDevice(char*name){
    XIAddMasterInfo add;
    add.type=XIAddMaster;
    add.name=name;
    add.send_core=1;
    add.enable=1;
    XIChangeHierarchy(dpy, (XIAnyHierarchyChangeInfo*)&add, 1);
    //xcb_input_xi_change_hierarchy(c, num_changes, changes);

}
void initCurrentMasters(){

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
    XIDeviceInfo *devices, *device;
    devices = XIQueryDevice(dpy, XIAllMasterDevices, &ndevices);
    for (int i = 0; i < ndevices; i++) {
        device = &devices[i];
        if(device->use == XIMasterKeyboard){
            addMaster(device->deviceid,device->attachment);
        }
    }
    XIFreeDeviceInfo(devices);
}
void setActiveMasterByDeviceId(int id){
    int ndevices;
    XIDeviceInfo *masterDevices;
    masterDevices = XIQueryDevice(dpy, id, &ndevices);

    switch(masterDevices->use){
        case XIMasterKeyboard:
            setActiveMaster(getMasterById(masterDevices->deviceid));
            break;
        case XIMasterPointer:
            setActiveMaster(getMasterById(masterDevices->attachment));
            break;
        case XISlaveKeyboard:
            setActiveMaster(getMasterById(masterDevices->attachment));
            break;
        case XISlavePointer:
            setActiveMasterByDeviceId(masterDevices->attachment);
            break;
    }
    XIFreeDeviceInfo(masterDevices);
    assert(getActiveMaster());
}

Node* getSlavesOfMaster(int*ids,int num,int*numberOfSlaves){
    assert(ids);
    int ndevices;
    XIDeviceInfo *devices, *device;
    devices = XIQueryDevice(dpy, XIAllDevices, &ndevices);
    Node*list=createEmptyHead();
    assert(ndevices);

    int count=0;
    for (int i = 0; i < ndevices; i++) {
        device = &devices[i];
        for(int n=0;n<num;n++)
            if(device->attachment==ids[n]){
                if(device->use == XISlaveKeyboard||
                        device->use == XISlavePointer){

                    if(isTestDevice(device->name))
                        continue;
                    count++;
                    SlaveDevice*slaveDevice=malloc(sizeof(SlaveDevice));
                    slaveDevice->id=device->deviceid;
                    slaveDevice->attachment=ids[n];
                    slaveDevice->offset=n;
                    insertHead(list,slaveDevice);
                }
                break;
            }
    }
    XIFreeDeviceInfo(devices);
    if(numberOfSlaves)
        *numberOfSlaves=count;
    return list;
}


void passiveGrab(int window,int maskValue){
    XIEventMask eventmask = {XIAllDevices,2,(unsigned char*)&maskValue};
    XISelectEvents(dpy, window, &eventmask, 1);
}
int isKeyboardMask(int mask){
    return mask & (XCB_INPUT_XI_EVENT_MASK_KEY_PRESS|XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE)?1:0;
}

int grabPointer(int id){
    return grabDevice(id,XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS|XCB_INPUT_XI_EVENT_MASK_BUTTON_RELEASE);
}
int grabActivePointer(){
    return grabPointer(getActiveMasterPointerID());
}
int grabKeyboard(int id){
    return grabDevice(id,XCB_INPUT_XI_EVENT_MASK_KEY_PRESS|XCB_INPUT_XI_EVENT_MASK_KEY_RELEASE);
}
int grabActiveKeyboard(){
    return grabKeyboard(getActiveMasterKeyboardID());
}
int getDeviceIDByMask(int mask){
        int global = mask&1;
        return global?XIAllMasterDevices:
                isKeyboardMask(mask)?getActiveMasterKeyboardID():
                getActiveMasterPointerID();
}
int grabDevice(int deviceID,int maskValue){
    XIEventMask eventmask = {deviceID,2,(unsigned char*)&maskValue};
    return XIGrabDevice(dpy, deviceID,  root, CurrentTime, None, GrabModeAsync,
                                       GrabModeAsync, 1, &eventmask);
}
int ungrabDevice(int id){
    return XIUngrabDevice(dpy, id, 0);
}


int grabDetail(int deviceID,int detail,int mod,int maskValue){
    XIEventMask eventmask = {deviceID,2,(unsigned char*)&maskValue};
    XIGrabModifiers modifiers[2]={{mod},{mod|IGNORE_MASK}};

    LOG(LOG_LEVEL_DEBUG,"Grabbing device:%d detail:%d mod:%d mask: %d %d\n",
            deviceID,detail,mod,maskValue,isKeyboardMask(maskValue));
    if(!isKeyboardMask(maskValue))
        return XIGrabButton(dpy, deviceID, detail, root, 0,
                XIGrabModeAsync, XIGrabModeAsync, 1, &eventmask, 2, modifiers);
    else
        return XIGrabKeycode(dpy, deviceID, detail, root, XIGrabModeAsync, XIGrabModeAsync,
                    1, &eventmask, 2, modifiers);
}
int ungrabDetail(int deviceID,int detail,int mod,int isKeyboard){
    LOG(LOG_LEVEL_DEBUG,"UNGrabbing device:%d detail:%d mod:%d %d\n",
                deviceID,detail,mod,isKeyboard);
    XIGrabModifiers modifiers[2]={{mod},{mod|IGNORE_MASK}};
    if(!isKeyboard)
        return XIUngrabButton(dpy, deviceID, detail, root, 2, modifiers);
    else
        return XIUngrabKeycode(dpy, deviceID, detail, root, 2, modifiers);
}

static int endsWith(const char *str, const char *suffix){
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr)
        return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}
int isTestDevice(char*str){
    return endsWith(str, "XTEST pointer")||endsWith(str, "XTEST keyboard");
}
void swapDeviceId(Master*master1,Master*master2){
    if(master1==master2)
        return;

    LOG(LOG_LEVEL_DEBUG,"Swapping %d(%d) with %d (%d)\n",
            master1->id,master1->pointerId,master2->id,master2->pointerId);
    //swap keyboard focus

    xcb_input_xi_set_focus(dis,getIntValue(getFocusedWindowByMaster(master1)), 0, master2->id);
    xcb_input_xi_set_focus(dis, getIntValue(getFocusedWindowByMaster(master2)), 0, master2->id);

    xcb_input_xi_query_pointer_reply_t* reply1=
        xcb_input_xi_query_pointer_reply(dis, xcb_input_xi_query_pointer(dis, root, master1->pointerId), NULL);
    xcb_input_xi_query_pointer_reply_t* reply2=
        xcb_input_xi_query_pointer_reply(dis, xcb_input_xi_query_pointer(dis, root, master2->pointerId), NULL);

    assert(reply1 && reply2);
    xcb_input_xi_warp_pointer(dis, None, root, 0, 0, 0, 0, reply1->root_x, reply1->root_y, master2->pointerId);
    xcb_input_xi_warp_pointer(dis, None, root, 0, 0, 0, 0, reply2->root_x, reply2->root_y, master1->pointerId);
    free(reply1);
    free(reply2);

    int ndevices;

    int ids[]={master1->id,master1->pointerId,master2->id,master2->pointerId};
    int idMap[]={master2->id,master2->pointerId,master1->id,master1->pointerId};
    Node*slaves=getSlavesOfMaster(ids, 4, &ndevices);
    Node*temp=slaves;
    int actualChanges=0;
    XIAnyHierarchyChangeInfo changes[ndevices];
    FOR_EACH(slaves,
           changes[actualChanges].type=XIAttachSlave;
           changes[actualChanges].attach.deviceid=getIntValue(slaves);
           changes[actualChanges].attach.new_master=idMap[((SlaveDevice*)getValue(slaves))->offset];
           actualChanges++;
    )
    deleteList(temp);
    assert(actualChanges==ndevices);
    //swap slave devices
    XIChangeHierarchy(dpy, changes, ndevices);

    int tempId=master1->id,tempPointerId=master1->pointerId;
    master1->id=master2->id;
    master1->pointerId=master2->pointerId;
    master2->id=tempId;
    master2->pointerId=tempPointerId;
    XFlush(dpy);
}

/*
int setBorder(xcb_window_t wid){
    return setBorderColor(wid,getActiveMaster()->focusColor);
}
int resetBorder(xcb_window_t win){
    Master*master=getValue(getNextMasterNodeFocusedOnWindow(win));
    if(master)
        return setBorderColor(win,master->focusColor);
    return setBorderColor(win,getActiveMaster()->unfocusColor);
}

*/
