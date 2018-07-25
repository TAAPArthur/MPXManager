#include <assert.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>

#include <xcb/xinput.h>
#include <X11/extensions/XInput2.h>

#include "defaults.h"
#include "devices.h"
#include "logger.h"
#include "mywm-util.h"


extern Display *dpy;
extern xcb_connection_t *dis;
extern int root;

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
    xcb_input_xi_set_client_pointer(dis, window, getAssociatedMasterDevice(getActiveMaster()->id));
}
int getClientKeyboard(int win){
    int masterPointer;
    XIGetClientPointer(dpy, win, &masterPointer);
    return getAssociatedMasterDevice(masterPointer);
}

void destroyMasterDevice(int id,int returnPointer,int returnKeyboard){
    XIRemoveMasterInfo remove;
    remove.type = XIRemoveMaster;
    remove.deviceid = id; /* assuming this is one of the master devices */
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
        if(device->use == XIMasterKeyboard ){
            addMaster(device->deviceid,device->attachment);
        }
    }
    XIFreeDeviceInfo(devices);
}
Node* getSlavesOfMaster(Master*master,char keyboard,char pointer){
    assert(master);
    int ndevices;
    XIDeviceInfo *devices, *device;
    devices = XIQueryDevice(dpy, XIAllDevices, &ndevices);
    Node*list=createEmptyHead();
    assert(ndevices);
    for (int i = 0; i < ndevices; i++) {
        device = &devices[i];
        if(device->attachment==master->id ||
            device->attachment==master->pointerId)
            if(device->use == XISlaveKeyboard && keyboard ||
                    device->use == XISlavePointer && pointer){

                int*p=malloc(sizeof(int));
                *p=device->deviceid;
                insertHead(list,p);
            }
    }
    XIFreeDeviceInfo(devices);
    return list;
}


//master methods
int setActiveMasterByMouseId(int mouseId){
    int masterMouseOrMasterKeyboaredId=getAssociatedMasterDevice(mouseId);

    //master pointer is functionly the same as keyboard slave
    return setActiveMasterByKeyboardId(masterMouseOrMasterKeyboaredId);
}
int setActiveMasterByKeyboardId(int keyboardId){
    Master*masterNode=getMasterById(keyboardId);
    if(masterNode){
        //if true then keyboardId was a master keyboard
        setActiveMaster(masterNode);
        return 1;
    }
    masterNode=getMasterById(getAssociatedMasterDevice(keyboardId));

    if(masterNode){
        setActiveMaster(masterNode);
        return 0;
    }
    LOG(LOG_LEVEL_ERROR,"device id was not a master keyboard or slave keyboard/slave pointer");
    assert(0);
    return -1;
}

int grabDetails(Binding*binding,int numKeys,int mask,int mouse){
    int errors=0;
    for (int i = 0; i < numKeys; i++){
        if(!mouse)
            binding[i].keyCode=XKeysymToKeycode(dpy, binding[i].detail);
        if(!binding[i].noGrab)
            errors+=grabDetail(XIAllMasterDevices,binding[i].keyCode,binding[i].mod,mask,mouse);
    }
    return errors;
}

int ungrabDevice(int id){
    return XIUngrabDevice(dpy, id, 0);
}
int grabActivePointer(){
    return grabDevice(getAssociatedMasterDevice(getActiveMaster()->id),XCB_INPUT_DEVICE_BUTTON_PRESS);
}
int grabActiveKeyboard(){
    return grabDevice(getActiveMaster()->id,XCB_INPUT_DEVICE_KEY_PRESS);
}
int grabDevice(int deviceID,int maskValue){
    XIEventMask eventmask = {deviceID,2,(unsigned char*)&maskValue};
    return XIGrabDevice(dpy, deviceID,  root, CurrentTime, None, GrabModeAsync,
                                       GrabModeAsync, False, &eventmask);
}
int grabActiveDetail(int detail,int mod,int mouse){
    int deviceID=mouse?
            getAssociatedMasterDevice(getActiveMaster()->id):
            getActiveMaster()->id;
    int mask=mouse?XCB_INPUT_KEY_RELEASE:XCB_INPUT_KEY_PRESS;
    return grabDetail(deviceID,detail,mod,mask,mouse);
}
int grabDetail(int deviceID,int detail,int mod,int maskValue,int mouse){
    XIEventMask eventmask = {deviceID,2,(unsigned char*)&maskValue};
    XIGrabModifiers modifiers[2]={{mod},{mod|IGNORE_MASK}};

    if(mouse)
        return XIGrabButton(dpy, deviceID, detail, root, 0,
                XIGrabModeAsync, XIGrabModeAsync, 1, &eventmask, 2, modifiers);
    else
        return XIGrabKeycode(dpy, deviceID, detail, root, XIGrabModeAsync, XIGrabModeAsync,
                    1, &eventmask, 2, modifiers);
}
int ungrabDetail(int deviceID,int detail,int mod,int mouse){
    XIGrabModifiers modifiers[2]={{mod},{mod|IGNORE_MASK}};
    if(mouse)
        return XIUngrabButton(dpy, deviceID, detail, root, 2, modifiers);
    else
        return XIUngrabKeycode(dpy, deviceID, detail, root, 1, modifiers);
}



void registerForEvents(){
    xcb_void_cookie_t cookie;
    cookie=xcb_change_window_attributes_checked(dis, root,XCB_CW_EVENT_MASK, &ROOT_EVENT_MASKS);
    checkError(cookie, root, "could not sent window masks");

    for(int i=0;i<4;i++)
        if(deviceBindings[i])
            grabDetails(deviceBindings[i], deviceBindingLengths[i], bindingMasks[i], i>=2);

    //TODO listen on select devices
    LOG(LOG_LEVEL_TRACE,"listening for device event;  masks: %d\n",DEVICE_EVENT_MASKS);
    XIEventMask eventmask = {XIAllDevices,4,(unsigned char*)&DEVICE_EVENT_MASKS};


    //xcb_input_event_mask_t eventmask={XCB_INPUT_DEVICE_ALL_MASTER,DEVICE_EVENT_MASKS};
    XISelectEvents(dpy, root, &eventmask, 1);
    //xcb_input_xi_select_events(dis, root, 1, &eventmask);
}
void pushBinding(Binding*chain){
    assert(chain);
    insertHead(getActiveMaster()->activeChains,chain);
}
void popActiveBinding(){
    softDeleteNode(getActiveMaster()->activeChains);
}
Node* getActiveBindingNode(){
    return getActiveMaster()->activeChains;
}
Binding* getActiveBinding(){
    return getValue(getActiveMaster()->activeChains);
}

int endsWith(const char *str, const char *suffix)
{
    if (!str || !suffix)
        return 0;
    size_t lenstr = strlen(str);
    size_t lensuffix = strlen(suffix);
    if (lensuffix >  lenstr)
        return 0;
    return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
}
void swapSlaves(XIDeviceEvent *devev){
    int masterPointer;
    XIGetClientPointer(dpy, 0, &masterPointer);
    int masterKeyboard=getAssociatedMasterDevice(masterPointer);
    int pointer=getAssociatedMasterDevice(getAssociatedMasterDevice(devev->deviceid));
    int keyboard=getAssociatedMasterDevice(pointer);

    //printf("%d %d %d %d",masterPointer,masterKeyboard,pointer,keyboard);
    //only act if device isn't already default master
    if(masterKeyboard==keyboard||masterPointer==pointer)
        return;
    if(masterKeyboard==pointer || masterPointer==keyboard)
        return;

    //swap keyboard focus
    Window masterKeyboardFocus;
    Window keyboardFocus;
    XIGetFocus(dpy, masterKeyboard, &masterKeyboardFocus);
    XIGetFocus(dpy, keyboard, &keyboardFocus);

    if(masterKeyboardFocus==0 ||keyboardFocus==0)
        return;

    //printf("Window master:%lu other:%lu",masterKeyboardFocus,keyboardFocus);

    XISetFocus(dpy, keyboard, masterKeyboardFocus, CurrentTime);
    XISetFocus(dpy,  masterKeyboard, keyboardFocus,CurrentTime);


    //swap mouse position
    double ignore ;
    Window wIgnore;
    XIButtonState       buttons;
    XIModifierState     mods;
    XIGroupState        group;
    double x,y;
    double masterX,masterY;
    XIQueryPointer(dpy, pointer,root, &wIgnore, &wIgnore, &x, &y, &ignore, &ignore, &buttons, &mods, &group);
    XIQueryPointer(dpy, masterPointer, root, &wIgnore, &wIgnore, &masterX, &masterY,&ignore, &ignore, &buttons, &mods, &group);
    printf("POS master:%f,%f other:%f,%f",x,y,masterX,masterY);
    XIWarpPointer(dpy, masterPointer, None, root,0,0,0,0, x,y);
    XIWarpPointer(dpy, pointer, None, root,0,0,0,0, masterX, masterY);



    int ndevices;
    XIDeviceInfo *devices, device;
    devices = XIQueryDevice(dpy, XIAllDevices, &ndevices);
    XIAnyHierarchyChangeInfo changes[ndevices];
    int actualChanges=0;
    //swap slave devices
    for (int i = 0; i < ndevices; i++) {
        device = devices[i];

        switch(device.use) {
           case XISlavePointer:
               if(device.attachment!=masterPointer && device.attachment!=pointer)
                   continue;
               if(endsWith(device.name,"XTEST pointer"))
                   continue;

               changes[actualChanges].type=XIAttachSlave;
               changes[actualChanges].attach.deviceid=device.deviceid;
               changes[actualChanges].attach.new_master=device.attachment==masterPointer?pointer:masterPointer;
               actualChanges++;
               break;

           case XISlaveKeyboard:
               if(device.attachment!=masterKeyboard && device.attachment!=keyboard)
                   continue;
               if(endsWith(device.name,"XTEST keyboard"))
                   continue;
               changes[actualChanges].type=XIAttachSlave;
               changes[actualChanges].attach.deviceid=device.deviceid;
               changes[actualChanges].attach.new_master=device.attachment==masterKeyboard?keyboard:masterKeyboard;
               actualChanges++;
               break;
        }
    }
    XIChangeHierarchy(dpy, changes, actualChanges);
    XIFreeDeviceInfo(devices);

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
