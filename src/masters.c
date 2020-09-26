#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "globals.h"
#include "util/logger.h"
#include "masters.h"
#include "mywm-structs.h"
#include "util/time.h"

///the active master
static Master* master = NULL;
///lists of all masters
static ArrayList masterList;
const ArrayList* getAllMasters(void) {
    return &masterList;
}

__DEFINE_GET_X_BY_NAME(Master)
Master* getActiveMaster(void) {
    return master;
}
void setActiveMaster(Master* newMaster) {
    if(master != newMaster && newMaster)
        DEBUG("Setting active master: %d", newMaster->id);
    master = newMaster;
}

Master* newMaster(MasterID pointerID, MasterID keyboardID, const char* name) {
    Master* master = (Master*)malloc(sizeof(Master));
    Master temp = {.id = pointerID, keyboardID = keyboardID, .focusColor = DEFAULT_BORDER_COLOR};
    memmove(master, &temp, sizeof(Master));
    addElement(&masterList, master);
    strncpy(master->name, name, MAX_NAME_LEN - 1);
    return master;
}
void freeMaster(Master* master) {
    clearFocusStack(master);
    clearArray(&master->slaves);
    clearArray(&master->bindings);
    if(getActiveMaster() == master)
        setActiveMaster(getHead(getAllMasters()));
    removeElement(&masterList, master, sizeof(MasterID));
    if(master->windowMoveResizer)
        free(master->windowMoveResizer);
    free(master);
}

void addDefaultMaster() {
    assert(getAllMasters()->size == 0);
    Master* m = newMaster(DEFAULT_KEYBOARD, DEFAULT_POINTER, "Virtual core");
    if(getActiveMaster() == NULL)
        setActiveMaster(m);
}
void onWindowFocusForMaster(WindowID win, Master* master) {
    WindowInfo* winInfo = getWindowInfo(win);
    if(!winInfo)
        return;
    int pos = getIndex(&master->windowStack, &win, sizeof(WindowID));
    DEBUG("updating focus for win %d at position %d", win, pos);
    if(! isFocusStackFrozen()) {
        if(pos == -1) {
            pos = master->windowStack.size;
            addElement(&master->windowStack, winInfo);
        }
        shiftToHead(&master->windowStack, pos);
    }
    else if(pos != -1)
        master->focusedWindowIndex = pos;
    else {
        addElementAt(&master->windowStack, winInfo, master->focusedWindowIndex);
    }
    master->focusedTimeStamp = getTime();
}
void onWindowFocus(WindowID win) {
    Master* master = getActiveMaster();
    onWindowFocusForMaster(win, master);
}
void clearFocusStack(Master* master) {
    clearArray(&master->windowStack);
}
WindowInfo* removeWindowFromFocusStack(Master* master, WindowID win) {
    int index  = getIndex(&master->windowStack, &win, sizeof(WindowID));
    if(index == -1)
        return NULL;
    if(master->focusedWindowIndex) {
        master->focusedWindowIndex -= master->focusedWindowIndex >= index;
    }
    return removeIndex(&master->windowStack, index);
}
WindowInfo* getFocusedWindowOfMaster(Master* master) {
    if(master->windowStack.size == 0)
        return NULL;
    return getElement(&master->windowStack, master->focusedWindowIndex);
}
WindowInfo* getFocusedWindow() {
    return getFocusedWindowOfMaster(getActiveMaster());
}

void setFocusStackFrozen(int value) {
    Master* master =  getActiveMaster();
    if(master->freezeFocusStack != value) {
        master->freezeFocusStack = value;
        if(master->focusedWindowIndex < master->windowStack.size) {
            shiftToHead(&master->windowStack, master->focusedWindowIndex);
            master->focusedWindowIndex = 0;
        }
    }
}

Master* getMasterByID(MasterID id) {
    FOR_EACH(Master*, master, getAllMasters()) {
        if(master->id == id || master->pointerID == id) {
            return master;
        }
    }
    TRACE("Could not find master matching %d out of %d Masters", id, getAllMasters()->size);
    return NULL;
}

Master* getMasterForSlave(Slave* slave) {
    return getMasterByID(slave->attachment);
}
void setMasterForSlave(Slave* slave, MasterID master) {
    if(slave->attachment != master) {
        if(slave->attachment) {
            Master* m = getMasterForSlave(slave);
            if(m)
                removeElement(&m->slaves, &slave->id, sizeof(MasterID));
        }
        slave->attachment = master;
        if(slave->attachment) {
            Master* m = getMasterForSlave(slave);
            if(m)
                addElement(&m->slaves, slave);
        }
    }
}
