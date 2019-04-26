/**
 * @file masters.c
 * @copybrief masters.h
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "globals.h"
#include "masters.h"

Master* createMaster(MasterID id, int partnerId, char* name, int focusColor){
    Master* master = calloc(1, sizeof(Master));
    master->id = id;
    master->pointerId = partnerId;
    master->focusColor = focusColor;
    master->bindingMode = NORMAL_MODE;
    strncpy(master->name, name, sizeof(master->name));
    master->name[NAME_BUFFER - 1] = 0;
    return master;
}
///lists of all masters
static ArrayList masterList;
///the active master
static Master* master;
ArrayList* getAllMasters(void){
    return &masterList;
}
ArrayList* getActiveMasterWindowStack(){
    return getWindowStackByMaster(getActiveMaster());
}
ArrayList* getWindowStackByMaster(Master* master){
    return &master->windowStack;
}

//master methods
int getActiveMasterKeyboardID(){
    return getActiveMaster()->id;
}
int getActiveMasterPointerID(){
    return getActiveMaster()->pointerId;
}
int isFocusStackFrozen(){
    return getActiveMaster()->freezeFocusStack;
}
void setFocusStackFrozen(int value){
    Master* master = getActiveMaster();
    if(master->freezeFocusStack != value){
        master->freezeFocusStack = value;
        shiftToHead(getWindowStackByMaster(master), master->focusedWindowIndex);
        master->focusedWindowIndex = 0;
    }
}
WindowInfo* getFocusedWindowByMaster(Master* master){
    ArrayList* list = getWindowStackByMaster(master);
    if(!isNotEmpty(list))return NULL;
    return getElement(list, master->focusedWindowIndex);
}
WindowInfo* getFocusedWindow(){
    return getFocusedWindowByMaster(getActiveMaster());
}
unsigned int getFocusedTime(Master* m){
    return m->focusedTimeStamp;
}
int addMaster(MasterID keyboardMasterId, MasterID masterPointerId, char* name, int focusColor){
    assert(keyboardMasterId);
    if(indexOf(&masterList, &keyboardMasterId, sizeof(int)) != -1)
        return 0;
    Master* keyboardMaster = createMaster(keyboardMasterId, masterPointerId, name, focusColor);
    if(!isNotEmpty(&masterList))
        setActiveMaster(keyboardMaster);
    addToList(&masterList, keyboardMaster);
    return 1;
}
int removeMaster(MasterID id){
    int index = indexOf(getAllMasters(), &id, sizeof(int));
    //if id is not in node
    if(index == -1)
        return 0;
    Master* master = removeFromList(getAllMasters(), index);
    clearList(getWindowStackByMaster(master));
    clearList(&master->activeChains);
    clearList(getWindowCache(master));
    if(getSize(getAllMasters()) == 0)
        setActiveMaster(NULL);
    else if(getActiveMaster() == master)
        setActiveMaster(getHead(getAllMasters()));
    free(master);
    return 1;
}
int removeWindowFromMaster(Master* master, WindowID winToRemove){
    int index = indexOf(getWindowStackByMaster(master), &winToRemove, sizeof(int));
    if(index != -1)
        removeFromList(getWindowStackByMaster(master), index);
    if(getSize(getWindowStackByMaster(master)) <= master->focusedWindowIndex)
        master->focusedWindowIndex = 0;
    return index != -1;
}

Master* getActiveMaster(void){
    return master;
}
Master* getMasterById(int keyboardID){
    return find(getAllMasters(), &keyboardID, sizeof(int));
}
void setActiveMaster(Master* newMaster){
    assert(!isNotEmpty(getAllMasters()) || newMaster);
    master = newMaster;
}

ArrayList* getWindowCache(Master* master){
    return &master->windowsToIgnore;
}
void clearWindowCache(void){
    clearList(getWindowCache(getActiveMaster()));
}
int updateWindowCache(WindowInfo* targetWindow){
    assert(targetWindow);
    return addUnique(getWindowCache(getActiveMaster()), targetWindow, sizeof(WindowID));
}
void setCurrentMode(int mode){
    getActiveMaster()->bindingMode = mode;
}
int getCurrentMode(void){
    return getActiveMaster()->bindingMode;
}
