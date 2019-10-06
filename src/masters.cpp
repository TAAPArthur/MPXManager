/**
 * @file masters.c
 * @copybrief masters.h
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "ext.h"
#include "globals.h"
#include "logger.h"
#include "masters.h"
#include "windows.h"
#include "mywm-structs.h"
#include "time.h"

///the active master
static Master* master;
///lists of all masters
static ArrayList<Master*>masterList;
ArrayList<Master*>& getAllMasters(void) {
    return masterList;
}
Master* getActiveMaster(void) {
    return master;
}
void setActiveMaster(Master* newMaster) {
    master = newMaster;
}
std::ostream& operator<<(std::ostream& strm, const Master& m) {
    return strm << "{ ID:" << m.getID() << "(" << m.getPointerID() << "), name:" << m.getName() <<
           ", color: " << m.getFocusColor() << ", window stack:"
           >> m.windowStack << ", slaves: " >> m.getSlaves() << "}";
}

void addDefaultMaster() {
    if(getAllMasters().find(DEFAULT_KEYBOARD) == NULL) {
        Master* m = new Master(DEFAULT_KEYBOARD, DEFAULT_POINTER, "Virtual core");
        getAllMasters().add(m);
        if(getActiveMaster() == NULL)
            setActiveMaster(m);
    }
}
Master::~Master() {
    if(getActiveMaster() == this)
        setActiveMaster(getAllMasters().front());
    removeID(this);
}
void Master::onWindowFocus(WindowID win) {
    WindowInfo* winInfo = getWindowInfo(win);
    if(!winInfo)
        return;
    int pos = getWindowStack().indexOf(winInfo);
    LOG(LOG_LEVEL_DEBUG, "updating focus for win %d at position %d out of%d\n", win, pos, getWindowStack().size());
    if(! isFocusStackFrozen()) {
        if(pos == -1)
            getWindowStack().add(winInfo);
        else getWindowStack().shiftToEnd(pos);
    }
    else if(pos != -1)
        focusedWindowIndex = pos;
    else {
        getWindowStack().add(winInfo);
        focusedWindowIndex = getWindowStack().size() - 1;
    }
    focusedTimeStamp = getTime();
}
WindowInfo* Master::getFocusedWindow(void) {
    return windowStack.empty() ? nullptr : isFocusStackFrozen() &&
           focusedWindowIndex < getWindowStack().size() ? getWindowStack()[focusedWindowIndex] : getWindowStack().back();
}
void Master::setFocusStackFrozen(int value) {
    if(freezeFocusStack != value) {
        freezeFocusStack = value;
        if(value)
            focusedWindowIndex = getWindowStack().size();
        else
            getWindowStack().shiftToEnd(focusedWindowIndex);
    }
}

WindowInfo* Master::getMostRecentlyFocusedWindow(bool(*filter)(WindowInfo*)) {
    for(int i = getWindowStack().size() - 1; i >= 0; i--)
        if(filter(getWindowStack()[i]))
            return getWindowStack()[i];
    return NULL;
}
Master* getMasterById(MasterID id, bool keyboard) {
    if(keyboard)
        return getAllMasters().find(id);
    else
        for(Master* master : getAllMasters())
            if(master->getPointerID() == id)
                return master;
    return NULL;
}
Master* getMasterByName(std::string name) {
    for(Master* master : getAllMasters())
        if(master->getName() == name)
            return master;
    return NULL;
}
