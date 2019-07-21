
#include "mywm-structs.h"
#include "windows.h"
#include "workspaces.h"
#include "monitors.h"

static bool validating = 0;
#define assertEquals(A,B)do{auto __A=A; auto __B =B; int __result=(__A==__B); if(!__result){valid=0;std::cout<<__A<<"!="<<__B<<"\n";assert(0 && #A "!=" #B);}}while(0)
bool validate() {
    if(validating)
        return 0;
    validating = 1;
    bool valid = 1;
    for(WindowInfo* winInfo : getAllWindows()) {
        assert(getWindowInfo(winInfo->getID()) == winInfo);
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
    validating = 0;
    return valid;
}
