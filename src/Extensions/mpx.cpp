#include <assert.h>
#include <string.h>

#include "../bindings.h"
#include "../devices.h"
#include "../globals.h"
#include "../logger.h"
#include "../masters.h"
#include "../xsession.h"
#include "../device-grab.h"
#include "../time.h"
#include "mpx.h"

#if MPX_EXT_ENABLED

/**
 * Holds info to match a master devices with a set of slaves
 */
typedef struct MPXMasterInfo {
    /// The name of the master device pair (w/o Keyboard or Pointer suffix)
    const std::string masterName;
    /// the focus color to give the master
    int focusColor;
    /// list of names of slaves
    ArrayList<std::string> slaveNames;
    MPXMasterInfo(std::string name): masterName(name) {}
} MPXMasterInfo;


static UniqueArrayList<MPXMasterInfo*> masterInfoList;
static bool mpxEnabled;

static void attachActiveSlaveToActiveMaster() {
    xcb_input_key_press_event_t* event = (xcb_input_key_press_event_t*)getLastEvent();
    LOG(LOG_LEVEL_TRACE, "device event %d %d %d %d\n\n", event->event_type, event->deviceid, event->sourceid, event->flags);
    int masterID;
    Master* master = getAllMasters().back();
    switch(event->event_type) {
        case XCB_INPUT_KEY_PRESS:
        case XCB_INPUT_KEY_RELEASE:
            masterID = master->getID();
            break;
        default:
            masterID = master->getPointerID();
    }
    attachSlaveToMaster(event->sourceid, masterID);
}

static void autoAttachSlave(void) {
    xcb_input_hierarchy_event_t* event = (xcb_input_hierarchy_event_t*)getLastEvent();
    if(event->flags & (XCB_INPUT_HIERARCHY_MASK_SLAVE_ADDED | XCB_INPUT_HIERARCHY_MASK_DEVICE_ENABLED))
        restoreMPX();
}
void addAutoMPXRules(void) {
    ROOT_DEVICE_EVENT_MASKS |= XCB_INPUT_XI_EVENT_MASK_HIERARCHY;
    getEventRules(GENERIC_EVENT_OFFSET + XCB_INPUT_HIERARCHY).add({autoAttachSlave});
    getEventRules(onXConnection).add(restoreMPX);
}
static void toggleSplitRules(AddFlag flag) {
    getEventRules(Idle).add(DEFAULT_EVENT(endSplitMaster), flag);
    getEventRules(ProcessDeviceEvent).add(PASSTHROUGH_EVENT(attachActiveSlaveToActiveMaster, NO_PASSTHROUGH));
}
int splitMaster(void) {
    endSplitMaster();
    grabDevice(XIAllDevices, XCB_INPUT_KEY_PRESS | XCB_INPUT_BUTTON_PRESS | XCB_INPUT_MOTION);
    std::string name = "dummy" + getTime();
    createMasterDevice(name);
    initCurrentMasters();
    toggleSplitRules(ADD_UNIQUE);
    return getActiveMasterKeyboardID();
}
void endSplitMaster(void) {
    ungrabDevice(XIAllDevices);
    toggleSplitRules(ADD_REMOVE);
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
#endif
