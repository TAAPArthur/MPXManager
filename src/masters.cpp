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
    return strm << "ID:" << m.getID() << "(" << m.getPointerID() << "), name: '" << m.getName() << "' Focused window: " <<
        m.getFocusedWindowID();
}
std::ostream& operator<<(std::ostream& strm, const Master& m) {
    return strm >> m << ", color: " << m.getFocusColor() << ", window stack:"
        >> m.windowStack << (m.focusedWindowIndex) << ", slaves: " >> m.getSlaves();
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
        if(pos == -1) {
            pos = getWindowStack().size();
            windowStack.add(winInfo);
        }
        windowStack.shiftToHead(pos);
    }
    else if(pos != -1)
        focusedWindowIndex = pos;
    else {
        windowStack.add(winInfo);
        windowStack.shiftToPos(getWindowStack().size() - 1, focusedWindowIndex);
    }
    focusedTimeStamp = getTime();
}
void Master::clearFocusStack() {
    windowStack.clear();
}
WindowInfo* Master::removeWindowFromFocusStack(WindowID win) {
    if(focusedWindowIndex) {
        focusedWindowIndex -= focusedWindowIndex >= getWindowStack().indexOf(win);
    }
    return windowStack.removeElement(win);
}
WindowInfo* Master::getFocusedWindow(void) const {
    if(windowStack.empty())
        return NULL;
    return isFocusStackFrozen() ? getWindowStack()[focusedWindowIndex] : getWindowStack().front();
}

WindowID Master::getFocusedWindowID() const {
    return getFocusedWindow() ? getFocusedWindow()->getID() : 0;
}
void Master::setFocusStackFrozen(int value) {
    if(freezeFocusStack != value) {
        freezeFocusStack = value;
        if(focusedWindowIndex < getWindowStack().size()) {
            windowStack.shiftToHead(focusedWindowIndex);
            focusedWindowIndex = 0;
        }
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
