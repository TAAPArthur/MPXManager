#include "boundfunction.h"
#include "debug.h"
#include "logger.h"
#include "monitors.h"
#include "mywm-structs.h"
#include "system.h"
#include "user-events.h"
#include "window-masks.h"
#include "windows.h"
#include "workspaces.h"
#include "xsession.h"

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
    LOG(LOG_LEVEL_DEBUG, "validation result: %d", valid);
    return valid;
}
void dieOnIntegrityCheckFail() {
    if(!validate())
        quit(3);
}
void addDieOnIntegrityCheckFailRule(bool remove) {
    getEventRules(TRUE_IDLE).add(DEFAULT_EVENT(dieOnIntegrityCheckFail), remove);
}
void addAllDebugRules(bool remove) {
    getEventRules(CLIENT_MAP_ALLOW).add({+[](WindowInfo * winInfo) {INFO(*winInfo);}, "WindowDump"}, remove);
    addDieOnIntegrityCheckFailRule(remove);
}

void dumpRules(void) {
    for(int i = 0; i < NUMBER_OF_MPX_EVENTS; i++)
        if(!getEventRules(i).empty())
            std::cout << eventTypeToString(i) << ": " << getEventRules(i) << std::endl;
    for(int i = 0; i < NUMBER_OF_BATCHABLE_EVENTS; i++)
        if(!getBatchEventRules(i).empty())
            std::cout << eventTypeToString(i) << "(Batch): " << getBatchEventRules(i) << std::endl;
}
void printSummary(void) {
    dumpRules();
    std::cout << "Summary:" << std::endl;
    std::cout << "Slaves: " << getAllSlaves() << std::endl;
    std::cout << "Masters: " << getAllMasters() << std::endl;
    std::cout << "Monitors: " << getAllMonitors() << std::endl;
    std::cout << "Workspaces: " << getAllWorkspaces() << std::endl;
    dumpWindow();
}
void dumpWindow(WindowMask filterMask) {
    std::cout << "Dumping:" << std::endl;
    for(WindowInfo* winInfo : getAllWindows()) {
        if(!filterMask || winInfo->hasMask(filterMask))
            std::cout << *winInfo << std::endl;
    }
}
void dumpMaster(Master* master) {
    if(!master)
        master = getActiveMaster();
    std::cout << "Dumping:" << *master << std::endl;
    std::cout << master->getSlaves() << std::endl;
    for(WindowInfo* winInfo : master->getWindowStack()) {
        std::cout << *winInfo << std::endl;
    }
}
void dumpWindow(std::string match) {
    std::cout << "Dumping:" << std::endl;
    for(WindowInfo* winInfo : getAllWindows()) {
        if(winInfo->getClassName() == match || winInfo->getInstanceName() == match)
            std::cout << *winInfo << std::endl;
    }
}
void dumpSingleWindow(WindowID win) {
    WindowInfo* winInfo = getWindowInfo(win);
    if(!winInfo) {
        std::cout << "No window with id " << win << std::endl;
        return;
    }
    std::cout << "Dumping:" << std::endl;
    std::cout << *winInfo << std::endl;
}
void dumpWindowStack() {
    std::cout << "Dumping Window Stack:" << std::endl;
    for(WindowInfo* winInfo : getActiveWorkspace()->getWindowStack())
        std::cout << *winInfo << std::endl;
}
