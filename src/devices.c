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

#include "xsession.h"
#include "globals.h"
#include "logger.h"
#include "masters.h"
#include "test-functions.h"

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

void attachSlaveToMaster(int slaveId,int masterId){
    LOG(LOG_LEVEL_DEBUG,"attaching %d to %d\n",slaveId,masterId);
    XIAnyHierarchyChangeInfo changes;
    changes.type=XIAttachSlave;
    changes.attach.deviceid=slaveId;
    changes.attach.new_master=masterId;
    XIChangeHierarchy(dpy, &changes, 1);
}
void destroyMasterDevice(int id,int returnPointer,int returnKeyboard){
    if(id==DEFAULT_KEYBOARD||id==DEFAULT_POINTER)return;
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

void destroyAllNonDefaultMasters(void){
    FOR_EACH(Master*master,getAllMasters(),if(master->id!=DEFAULT_KEYBOARD)destroyMasterDevice(master->id, DEFAULT_POINTER, DEFAULT_KEYBOARD));
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
            int lastSpace=0;
            for(int n=0;device->name[n];n++)
                if(device->name[n]==' ')
                    lastSpace=n;
            if(lastSpace)
                device->name[lastSpace]=0;
            addMaster(device->deviceid,device->attachment,device->name,DEFAULT_FOCUS_BORDER_COLOR);
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
ArrayList*getSlavesOfMaster(Master*master){
    return getSlavesOfMasterByID((int[]){master->id,master->pointerId},2,NULL);
}
ArrayList* getSlavesOfMasterByID(int*ids,int num,int*numberOfSlaves){
    int ndevices;
    XIDeviceInfo *devices, *device;
    devices = XIQueryDevice(dpy, XIAllDevices, &ndevices);
    ArrayList*list=calloc(1,sizeof(ArrayList));
    assert(ndevices);

    int count=0;
    for (int i = 0; i < ndevices; i++) {
        device = &devices[i];
        if(device->use == XISlaveKeyboard|| device->use == XISlavePointer){
            int n=0;
            if(ids){
                for(;n<num;n++)
                    if(device->attachment==ids[n])
                        break;
                if(n>=num)continue;
            }
            if(isTestDevice(device->name))
                continue;
            count++;
            SlaveDevice*slaveDevice=malloc(sizeof(SlaveDevice));
            slaveDevice->id=device->deviceid;
            slaveDevice->attachment=device->attachment;
            strncpy(slaveDevice->name,device->name,sizeof(slaveDevice->name));
            slaveDevice->name[NAME_BUFFER-1]=0;
            //force NULL terminate the string it were to overfil the buffer
            slaveDevice->name[sizeof(slaveDevice->name)-1]=0;
            slaveDevice->keyboard=device->use == XISlaveKeyboard;
            slaveDevice->offset=n;
            addToList(list,slaveDevice);
        }
    }
    XIFreeDeviceInfo(devices);
    if(numberOfSlaves)
        *numberOfSlaves=count;
    return list;
}


void passiveUngrab(int window){
    passiveGrab(window,0);
}
void passiveGrab(int window,int maskValue){
    XIEventMask eventmask = {XIAllDevices,2,(unsigned char*)&maskValue};
    XISelectEvents(dpy, window, &eventmask, 1);
}
int isKeyboardMask(int mask){
    return mask & (KEYBOARD_MASKS)?1:0;
}

int grabPointer(int id){
    return grabDevice(id,POINTER_MASKS);
}
int grabActivePointer(){
    return grabPointer(getActiveMasterPointerID());
}
int grabKeyboard(int id){
    return grabDevice(id,KEYBOARD_MASKS);
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
int grabAllMasterDevices(void){
    int result=1;
    FOR_EACH(Master*master,getAllMasters(),
        result&=grabDevice(master->id,KEYBOARD_MASKS);
        result&=grabDevice(master->pointerId,POINTER_MASKS);
    );
    return result;
}
int ungrabAllMasterDevices(void){
    int result=1;
    FOR_EACH(Master*master,getAllMasters(),
        result&=ungrabDevice(master->id);
        result&=ungrabDevice(master->pointerId);
    );
    return result;
}
int ungrabDevice(int id){
    return XIUngrabDevice(dpy, id, 0);
}


int grabDetail(int deviceID,int detail,int mod,int maskValue){
    XIEventMask eventmask = {deviceID,2,(unsigned char*)&maskValue};
    XIGrabModifiers modifiers[2]={{mod},{mod|IGNORE_MASK}};

    LOG(LOG_LEVEL_TRACE,"Grabbing device:%d detail:%d mod:%d mask: %d %d\n",
            deviceID,detail,mod,maskValue,isKeyboardMask(maskValue));
    if(!isKeyboardMask(maskValue))
        return XIGrabButton(dpy, deviceID, detail, root, 0,
                XIGrabModeAsync, XIGrabModeAsync, 1, &eventmask, 2, modifiers);
    else
        return XIGrabKeycode(dpy, deviceID, detail, root, XIGrabModeAsync, XIGrabModeAsync,
                    1, &eventmask, 2, modifiers);
}
int ungrabDetail(int deviceID,int detail,int mod,int isKeyboard){
    LOG(LOG_LEVEL_TRACE,"UNGrabbing device:%d detail:%d mod:%d %d\n",
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
int getMousePosition(int id,int relativeWindow,int result[2]){
    xcb_input_xi_query_pointer_reply_t* reply1=
        xcb_input_xi_query_pointer_reply(dis, xcb_input_xi_query_pointer(dis, relativeWindow, id), NULL);
    if(reply1){
        result[0]=reply1->win_x;
        result[1]=reply1->win_y;
        free(reply1);
    }
    return reply1?1:0;
}
int getActiveFocus(int id){
    int win;
    xcb_input_xi_get_focus_reply_t *reply;
    reply=xcb_input_xi_get_focus_reply(dis, xcb_input_xi_get_focus(dis, id), NULL);
    win=reply->focus;
    free(reply);
    return win;
}
void swapDeviceId(Master*master1,Master*master2){
    if(master1==master2)
        return;

    LOG(LOG_LEVEL_DEBUG,"Swapping %d(%d) with %d (%d)\n",
            master1->id,master1->pointerId,master2->id,master2->pointerId);
    //swap keyboard focus

    xcb_input_xi_set_focus(dis, getActiveFocus(master1->id), 0, master2->id);
    xcb_input_xi_set_focus(dis, getActiveFocus(master2->id), 0, master2->id);

    xcb_input_xi_query_pointer_reply_t* reply1=
        xcb_input_xi_query_pointer_reply(dis, xcb_input_xi_query_pointer(dis, root, master1->pointerId), NULL);
    xcb_input_xi_query_pointer_reply_t* reply2=
        xcb_input_xi_query_pointer_reply(dis, xcb_input_xi_query_pointer(dis, root, master2->pointerId), NULL);

    assert(reply1 && reply2);
    movePointer(master2->pointerId,root,reply1->root_x, reply1->root_y);
    movePointer(master1->pointerId,root,reply2->root_x, reply2->root_y);
    free(reply1);
    free(reply2);

    int ndevices;

    int ids[]={master1->id,master1->pointerId,master2->id,master2->pointerId};
    int idMap[]={master2->id,master2->pointerId,master1->id,master1->pointerId};
    ArrayList*slaves=getSlavesOfMasterByID(ids, 4, &ndevices);
    FOR_EACH(SlaveDevice*device,slaves,
        //swap slave devices
        attachSlaveToMaster(
            device->id,
            idMap[device->offset]
        );
    )
    deleteList(slaves);
    free(slaves);

    int tempId=master1->id,tempPointerId=master1->pointerId;
    master1->id=master2->id;
    master1->pointerId=master2->pointerId;
    master2->id=tempId;
    master2->pointerId=tempPointerId;
    XFlush(dpy);
}
void setLastKnowMasterPosition(int x,int y){
    memcpy(getActiveMaster()->prevMousePos,getActiveMaster()->currentMousePos,sizeof(getActiveMaster()->prevMousePos));
    getActiveMaster()->currentMousePos[0]=x;
    getActiveMaster()->currentMousePos[1]=y;
}
const short* getLastKnownMasterPosition(void){
    return getActiveMaster()->currentMousePos;
}
void getMouseDelta(Master*master,short result[2]){
    for(int i=0;i<2;i++)
        result[i]=master->currentMousePos[i]-master->prevMousePos[i];
}
