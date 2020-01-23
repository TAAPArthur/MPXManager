#include <assert.h>
#include <string.h>

#include "../bindings.h"
#include "../device-grab.h"
#include "../devices.h"
#include "../globals.h"
#include "../logger.h"
#include "../masters.h"
#include "../test-functions.h"
#include "../time.h"
#include "../xsession.h"
#include "../communications.h"
#include "mpx.h"

/**
 * Holds info to match a master devices with a set of slaves
 */
struct MPXMasterInfo {
    /// The name of the master device pair (w/o Keyboard or Pointer suffix)
    const std::string masterName;
    /// the focus color to give the master
    int focusColor;
    /// list of names of slaves
    ArrayList<std::string> slaveNames;
    MPXMasterInfo(std::string name): masterName(name) {}
} ;


static UniqueArrayList<MPXMasterInfo*> masterInfoList;
static bool mpxEnabled;

MasterID markedMasterID = 0;
void markMaster(Master* master) {
    markedMasterID = master ? master->getID() : 0;
}
void attachActiveSlaveToMarkedMaster() {
    xcb_input_key_press_event_t* event = (xcb_input_key_press_event_t*)getLastEvent();
    LOG(LOG_LEVEL_DEBUG, "device event %d %d %d %d\n", event->event_type, event->deviceid, event->sourceid, event->flags);
    Master* master = getMasterByID(markedMasterID);
    if(!master) {
        LOG(LOG_LEVEL_WARN, "could not find master %d\n", markedMasterID);
        return;
    }
    Slave* slave = getAllSlaves().find(event->sourceid);
    if(slave)
        attachSlaveToMaster(slave, master);
    else
        LOG(LOG_LEVEL_DEBUG, "Slave %d (%d) not found\n", event->sourceid, event->deviceid);
}

void startSplitMaster(void) {
    LOG(LOG_LEVEL_INFO, "Started splitting master\n");
    passiveGrab(root, XCB_INPUT_XI_EVENT_MASK_HIERARCHY | XCB_INPUT_XI_EVENT_MASK_KEY_PRESS |
        XCB_INPUT_XI_EVENT_MASK_BUTTON_PRESS | XCB_INPUT_XI_EVENT_MASK_MOTION);
    std::string name = "dummy" + std::to_string(getTime());
    createMasterDevice(name);
    initCurrentMasters();
    markMaster(getMasterByName(name));
}

void endSplitMaster(void) {
    LOG(LOG_LEVEL_INFO, "Finished splitting master\n");
    passiveGrab(root, ROOT_DEVICE_EVENT_MASKS);
    markMaster(0);
}
void cycleSlave(int dir) {
    xcb_input_key_press_event_t* event = (xcb_input_key_press_event_t*)getLastEvent();
    LOG(LOG_LEVEL_DEBUG, "device event %d %d %d %d\n", event->event_type, event->deviceid, event->sourceid, event->flags);
    Slave* slave = getAllSlaves().find(event->sourceid);
    Master* newMaster = getAllMasters().getNextValue(getAllMasters().indexOf(getActiveMaster()), dir);
    if(slave)
        attachSlaveToMaster(slave, newMaster);
}

void swapSlaves(Master* master1, Master* master2) {
    for(Slave* slave : master1->getSlaves()) {
        attachSlaveToMaster(slave, master2);
    }
    for(Slave* slave : master2->getSlaves()) {
        attachSlaveToMaster(slave, master1);
    }
}

void swapSlaves(Master* master, int dir) {
    auto index = getAllMasters().indexOf(master);
    swapSlaves(master, getAllMasters().getNextValue(index, dir));
}

void swapDeviceID(Master* master1, Master* master2) {
    if(master1 == master2)
        return;
    LOG(LOG_LEVEL_DEBUG, "Swapping %d(%d) with %d (%d)\n",
        master1->getID(), master1->getPointerID(), master2->getID(), master2->getPointerID());
    //swap keyboard focus
    xcb_input_xi_set_focus(dis, getActiveFocus(master1->getID()), 0, master2->getID());
    xcb_input_xi_set_focus(dis, getActiveFocus(master2->getID()), 0, master2->getID());
    short pos1[2];
    short pos2[2];
    if(getMousePosition(master1->getPointerID(), root, pos1) && getMousePosition(master2->getPointerID(), root, pos2)) {
        movePointer(pos1[0], pos1[1], root, master2->getPointerID());
        movePointer(pos2[0], pos2[1], root, master1->getPointerID());
    }
    swapSlaves(master1, master2);
    flush();
}

static void autoAttachSlave(void) {
    xcb_input_hierarchy_event_t* event = (xcb_input_hierarchy_event_t*)getLastEvent();
    if(event->flags & (XCB_INPUT_HIERARCHY_MASK_SLAVE_ADDED | XCB_INPUT_HIERARCHY_MASK_DEVICE_ENABLED))
        restoreMPX();
}
void addAutoMPXRules(void) {
    ROOT_DEVICE_EVENT_MASKS |= XCB_INPUT_XI_EVENT_MASK_HIERARCHY;
    getEventRules(GENERIC_EVENT_OFFSET + XCB_INPUT_HIERARCHY).add(DEFAULT_EVENT(autoAttachSlave));
    getEventRules(X_CONNECTION).add(DEFAULT_EVENT(restoreMPX));
}

Master* getMasterForSlave(std::string slaveName) {
    for(MPXMasterInfo* masterInfo : masterInfoList) {
        if(masterInfo->slaveNames.indexOf(slaveName) != -1)
            return getMasterByName(masterInfo->masterName);
    }
    return NULL;
}

void restoreMPX(void) {
    if(masterInfoList.empty())
        loadMPXMasterInfo();
    for(MPXMasterInfo* masterInfo : masterInfoList) {
        Master* m = getMasterByName(masterInfo->masterName);
        if(m) {
            LOG(LOG_LEVEL_TRACE, "setting color to %0x\n", masterInfo->focusColor);
            m->setFocusColor(masterInfo->focusColor);
        }
    }
    for(Slave* slave : getAllSlaves()) {
        Master* master = getMasterForSlave(slave->getName());
        if(master)
            attachSlaveToMaster(slave, master);
    }
    initCurrentMasters();
}
void startMPX(void) {
    LOG(LOG_LEVEL_INFO, "Starting MPX\n");
    mpxEnabled = 1;
    if(masterInfoList.empty())
        loadMPXMasterInfo();
    initCurrentMasters();
    for(MPXMasterInfo* masterInfo : masterInfoList) {
        auto name = masterInfo->masterName;
        if(!getMasterByName(name)) {
            createMasterDevice(name);
        }
    }
    initCurrentMasters();
    restoreMPX();
}
void stopMPX(void) {
    mpxEnabled = 0;
    destroyAllNonDefaultMasters();
    flush();
    setActiveMaster(getMasterByID(DEFAULT_KEYBOARD));
    assert(getActiveMaster()->getID() == DEFAULT_KEYBOARD);
}
void restartMPX(void) {
    stopMPX();
    startMPX();
}
static FILE* openMPXMasterInfoFile(const char* mode) {
    std::string path;
    if(MASTER_INFO_PATH[0] == '~')
        path = getenv("HOME") + MASTER_INFO_PATH.substr(1);
    else path = MASTER_INFO_PATH;
    FILE* fp = fopen(path.c_str(), mode);
    return fp;
}

int saveMPXMasterInfo(void) {
    FILE* fp = openMPXMasterInfoFile("w");
    if(!fp)
        return 0;
    LOG(LOG_LEVEL_INFO, "Saving MPX state\n");
    for(Master* master : getAllMasters()) {
        fprintf(fp, "%s\n", master->getName().c_str());
        fprintf(fp, "%06X\n", master->getFocusColor());
        for(Slave* slave : master->getSlaves()) {
            fprintf(fp, "%s\n", slave->getName().c_str());
        }
        fprintf(fp, "\n");
    };
    fclose(fp);
    return 1;
}
int loadMPXMasterInfo(void) {
    FILE* fp;
    char* line = NULL;
    size_t bSize = 0;
    ssize_t len;
    fp = openMPXMasterInfoFile("r");
    if(fp == NULL)
        return 0;
    LOG(LOG_LEVEL_INFO, "Loading saved MPX state\n");
    masterInfoList.deleteElements();
    MPXMasterInfo* info = NULL;
    int state = 0;
    while((len = getline(&line, &bSize, fp)) != -1) {
        if(line[len - 1] == '\n')
            line[--len] = 0;
        if(len == 0) {
            state = 0;
            continue;
        }
        switch(state) {
            case 0:
                masterInfoList.add(info = new MPXMasterInfo(line));
                break;
            case 1:
                info->focusColor = (int)strtol(line, NULL, 16);
                break;
            default:
                info->slaveNames.add(line);
                break;
        }
        state++;
    }
    free(line);
    fclose(fp);
    return 1;
}

__attribute__((constructor)) static void registerModes() {
    addStartupMode("mpx-start", startMPX);
    addStartupMode("mpx-stop", stopMPX);
    addStartupMode("mpx-restart", restartMPX);
    addStartupMode("mpx-restore", restoreMPX);
}
