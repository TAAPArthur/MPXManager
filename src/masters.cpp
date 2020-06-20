#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "ext.h"
#include "globals.h"
#include "logger.h"
#include "masters.h"
#include "mywm-structs.h"
#include "time.h"
#include "windows.h"

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
    if(master != newMaster && newMaster)
        LOG(LOG_LEVEL_DEBUG, "Setting new master %d", newMaster->getID());
    master = newMaster;
}

std::ostream& operator>>(std::ostream& strm, const Master& m) {
    return strm << "ID:" << m.getID() << "(" << m.getPointerID() << "), name:" << m.getName();
}
std::ostream& operator<<(std::ostream& strm, const Master& m) {
    return strm >> m << ", color: " << m.getFocusColor() << ", window stack:"
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
    LOG(LOG_LEVEL_DEBUG, "updating focus for win %d at position %d out of %d", win, pos, getWindowStack().size());
    if(! isFocusStackFrozen()) {
        if(pos == -1)
            windowStack.add(winInfo);
        else windowStack.shiftToEnd(pos);
    }
    else if(pos != -1)
        focusedWindowIndex = pos;
    else {
        windowStack.add(winInfo);
        focusedWindowIndex = getWindowStack().size() - 1;
    }
    focusedTimeStamp = getTime();
}
void Master::clearFocusStack() {
    windowStack.clear();
}
WindowInfo* Master::removeWindowFromFocusStack(WindowID win) {
    return windowStack.removeElement(win);
}
WindowInfo* Master::getFocusedWindow(void) {
    if(windowStack.empty())
        return NULL;
    WindowInfo* winInfo = isFocusStackFrozen() &&
        focusedWindowIndex < getWindowStack().size() ? getWindowStack()[focusedWindowIndex] : getWindowStack().back();
    return winInfo->isFocusable() ? winInfo : NULL;
}

WindowID Master::getFocusedWindowID() {
    return getFocusedWindow() ? getFocusedWindow()->getID() : 0;
}
void Master::setFocusStackFrozen(int value) {
    if(freezeFocusStack != value) {
        freezeFocusStack = value;
        if(value)
            focusedWindowIndex = getWindowStack().size();
        else if(focusedWindowIndex < getWindowStack().size())
            windowStack.shiftToEnd(focusedWindowIndex);
    }
}

Master* getMasterByID(MasterID id, bool keyboard) {
    if(keyboard)
        return getAllMasters().find(id);
    else
        for(Master* master : getAllMasters())
            if(master->getPointerID() == id)
                return master;
    LOG(LOG_LEVEL_TRACE, "Could not find master matching %d", id);
    return NULL;
}
Master* getMasterByName(std::string name) {
    for(Master* master : getAllMasters())
        if(master->getName() == name)
            return master;
    return NULL;
}
