
#include "mywm-structs.h"
#include "system.h"
#include "logger.h"
#include "windows.h"
#include "workspaces.h"
#include "monitors.h"
#include "xsession.h"
#include "boundfunction.h"
#include "user-events.h"

#define assertEquals(A,B)do{auto __A=A; auto __B =B; int __result=(__A==__B); if(!__result){valid=0;std::cout<<__LINE__<<":"<<__A<<"!="<<__B<<"\n";assert(0 && #A "!=" #B);}}while(0)

bool isWindowMapped(WindowID win) {
    xcb_get_window_attributes_reply_t* reply;
    reply = xcb_get_window_attributes_reply(dis, xcb_get_window_attributes(dis, win), NULL);
    bool result = reply->map_state != XCB_MAP_STATE_UNMAPPED;
    free(reply);
    return result;
}

bool validate() {
    bool valid = 1;
    for(WindowInfo* winInfo : getAllWindows()) {
        assertEquals(getWindowInfo(winInfo->getID()), winInfo);
        if(winInfo->getWorkspace())
            assertEquals(winInfo->getWorkspace()->getWindowStack().find(winInfo->getID()), winInfo);
        else assertEquals(winInfo->getWorkspaceIndex(), NO_WORKSPACE);
    }
    for(Master* m : getAllMasters()) {
        assertEquals(getAllMasters().find(m->getID()), m);
        for(WindowInfo* winInfo : m->getWindowStack())
            assertEquals(getWindowInfo(winInfo->getID()), winInfo);
        for(Slave* slave : m->getSlaves())
            assertEquals(slave->getMaster(), m);
    }
    for(Slave* slave : getAllSlaves()) {
        assertEquals(getAllSlaves().find(slave->getID()), slave);
        if(slave->getMasterID())
            assertEquals(slave->getMaster()->getSlaves().find(slave->getID()), slave);
    }
    for(Workspace* w : getAllWorkspaces()) {
        assertEquals(getAllWorkspaces().find(w->getID()), w);
        for(WindowInfo* winInfo : w->getWindowStack())
            assertEquals(w, winInfo->getWorkspace());
        if(w->getMonitor())
            assertEquals(w->getMonitor()->getWorkspace(), w);
    }
    for(Monitor* m : getAllMonitors()) {
        assertEquals(getAllMonitors().find(m->getID()), m);
        if(m->getWorkspace())
            assertEquals(m->getWorkspace()->getMonitor(), m);
    }
    LOG(LOG_LEVEL_DEBUG, "validation result: %d\n", valid);
    return valid;
}
void dieOnIntegrityCheckFail() {
    if(!validate())
        quit(3);
}
void addDieOnIntegrityCheckFailRule() {
    getEventRules(TrueIdle).add(DEFAULT_EVENT(dieOnIntegrityCheckFail), PREPEND_UNIQUE);
}
