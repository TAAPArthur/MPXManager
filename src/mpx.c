/**
 * @file mpx.c
 * @copydoc mpx.h
 */


#include <assert.h>
#include <string.h>


#include "bindings.h"
#include "devices.h"
#include "globals.h"
#include "logger.h"
#include "masters.h"
#include "mpx.h"
#include "xsession.h"

/**
 * Holds info to match a master devices with a set of slaves
 */

typedef struct MasterInfo {
    /// The name of the master device pair (w/o Keyboard or Pointer suffix)
    char name[NAME_BUFFER];
    /// the focus color to give the master
    int focusColor;
    /// list of names of slaves
    ArrayList slaves;
} MasterInfo;

static ArrayList masterInfoList;
static int mpxEnabled;

static Rule deviceEventRule;
static Rule cleanupRule = CREATE_WILDCARD(BIND(endSplitMaster), .passThrough = NO_PASSTHROUGH);
static void attachActiveSlavesToMaster(int id){
    xcb_input_key_press_event_t* event = getLastEvent();
    LOG(LOG_LEVEL_TRACE, "device event %d %d %d %d\n\n", event->event_type, event->deviceid, event->sourceid, event->flags);
    int masterId;
    Master* master = getMasterById(id);
    switch(event->event_type){
        case XCB_INPUT_KEY_PRESS:
        case XCB_INPUT_KEY_RELEASE:
            masterId = master->id;
            break;
        default:
            masterId = master->pointerId;
    }
    attachSlaveToMaster(event->sourceid, masterId);
}

static char* getNameOfMaster(Master* master){
    return master->name;
}
static char* getNameOfSlave(SlaveDevice* slave){
    return slave->name;
}
static void autoAttachSlave(void){
    xcb_input_hierarchy_event_t* event = getLastEvent();
    if(event->flags & (XCB_INPUT_HIERARCHY_MASK_SLAVE_ADDED | XCB_INPUT_HIERARCHY_MASK_DEVICE_ENABLED))
        restoreMPX();
}
void addAutoMPXRules(void){
    static Rule autoAttachRule = CREATE_WILDCARD(BIND(autoAttachSlave));
    static Rule attachOnStartRule = CREATE_WILDCARD(BIND(restoreMPX));
    prependRule(GENERIC_EVENT_OFFSET + XCB_INPUT_HIERARCHY, &autoAttachRule);
    prependRule(onXConnection, &attachOnStartRule);
}
int splitMaster(void){
    endSplitMaster();
    grabAllMasterDevices();
    char* name = "dummy";
    createMasterDevice(name);
    initCurrentMasters();
    setActiveMaster(getLast(getAllMasters()));
    assert(strcmp(getNameOfMaster(getActiveMaster()), name) == 0);
    deviceEventRule = (Rule) CREATE_WILDCARD(BIND(attachActiveSlavesToMaster, getActiveMaster()->id),
                      .passThrough = NO_PASSTHROUGH);
    prependRule(Idle, &cleanupRule);
    prependRule(GENERIC_EVENT_OFFSET + XCB_INPUT_KEY_PRESS, &deviceEventRule);
    prependRule(GENERIC_EVENT_OFFSET + XCB_INPUT_BUTTON_PRESS, &deviceEventRule);
    return getActiveMasterKeyboardID();
}
void endSplitMaster(void){
    removeRule(GENERIC_EVENT_OFFSET + XCB_INPUT_KEY_PRESS, &deviceEventRule);
    removeRule(GENERIC_EVENT_OFFSET + XCB_INPUT_BUTTON_PRESS, &deviceEventRule);
    removeRule(Idle, &cleanupRule);
}

char* getMasterNameForSlave(char* slaveName){
    for(int i = 0; i < getSize(&masterInfoList); i++){
        ArrayList* slaves = &((MasterInfo*)getElement(&masterInfoList, i))->slaves;
        for(int n = 0; n < getSize(slaves); n++)
            if(strcmp(slaveName, getElement(slaves, n)) == 0)
                return ((MasterInfo*)getElement(&masterInfoList, i))->name;
    }
    return NULL;
}
Master* getMasterByName(char* name){
    Master* master;
    UNTIL_FIRST(master, getAllMasters(),
                strcmp(getNameOfMaster(master), name) == 0);
    return master;
}
Master* getMasterForSlave(char* slaveName){
    char* masterName = getMasterNameForSlave(slaveName);
    if(!masterName)return NULL;
    return getMasterByName(masterName);
}
int getMasterIdForSlave(SlaveDevice* slaveDevice){
    Master* master = getMasterForSlave(getNameOfSlave(slaveDevice));
    return !master ? 0 : slaveDevice->keyboard ? master->id : master->pointerId;
}

void restoreFocusColor(Master* master){
    master->focusColor = DEFAULT_FOCUS_BORDER_COLOR;
    for(int i = 0; i < getSize(&masterInfoList); i++)
        if(strcmp(((MasterInfo*)getElement(&masterInfoList, i))->name, master->name) == 0){
            master->focusColor = ((MasterInfo*)getElement(&masterInfoList, i))->focusColor;
            break;
        }
}

void attachSlaveToPropperMaster(SlaveDevice* slaveDevice){
    int id = getMasterIdForSlave(slaveDevice);
    if(id)
        attachSlaveToMaster(slaveDevice->id, id);
}
void restoreMPX(void){
    FOR_EACH(Master*, m, getAllMasters()) restoreFocusColor(m);
    int num;
    ArrayList* slaves = getSlavesOfMasterByID(NULL, 0, &num);
    FOR_EACH(SlaveDevice*, slave, slaves) attachSlaveToPropperMaster(slave);
    deleteList(slaves);
    free(slaves);
}
void startMPX(void){
    mpxEnabled = 1;
    if(!isNotEmpty(&masterInfoList))
        loadMasterInfo();
    for(int i = 0; i < getSize(&masterInfoList); i++){
        char* name = ((MasterInfo*)getElement(&masterInfoList, i))->name;
        if(!getMasterByName(name)){
            createMasterDevice(name);
            LOG(LOG_LEVEL_DEBUG, "creating master %s", name);
        }
    }
    initCurrentMasters();
    restoreMPX();
}
void stopMPX(void){
    mpxEnabled = 0;
    //destroyAllNonDefaultMasters();
    FOR_EACH_REVERSED(Master*, master, getAllMasters()){
        if(master->id != DEFAULT_KEYBOARD){
            destroyMasterDevice(master->id, DEFAULT_POINTER, DEFAULT_KEYBOARD);
            removeMaster(master->id);
        }
    }
    assert(getSize(getAllMasters()) == 1);
    assert(getActiveMaster()->id == DEFAULT_KEYBOARD);
    flush();
}

int saveMasterInfo(void){
    FILE* fp = fopen(MASTER_INFO_PATH, "w");
    if(!fp)
        return 0;
    FOR_EACH(Master*, master, getAllMasters()){
        fprintf(fp, "%s\n", getNameOfMaster(master));
        fprintf(fp, "%06X\n", master->focusColor);
        ArrayList* slaves = getSlavesOfMaster(master);
        FOR_EACH(SlaveDevice*, slave, slaves){
            fprintf(fp, "%s\n", getNameOfSlave(slave));
        }
        deleteList(slaves);
        free(slaves);
        fprintf(fp, "\n");
    };
    fclose(fp);
    return 1;
}
int loadMasterInfo(void){
    FILE* fp;
    char* line = NULL;
    size_t bSize = 0;
    ssize_t len;
    fp = fopen(MASTER_INFO_PATH, "r");
    if(fp == NULL)
        return 0;
    LOG(LOG_LEVEL_INFO, "Loading saved MPX state\n");
    for(int i = 0; i < getSize(&masterInfoList); i++){
        ArrayList* slaves = &((MasterInfo*)getElement(&masterInfoList, i))->slaves;
        for(int n = 0; n < getSize(slaves); n++)
            free(getElement(slaves, n));
        clearList(slaves);
        free(getElement(&masterInfoList, i));
    }
    clearList(&masterInfoList);
    MasterInfo* info = NULL;
    int state = 0;
    while((len = getline(&line, &bSize, fp)) != -1){
        if(line[len - 1] == '\n')
            line[--len] = 0;
        if(len == 0){
            state = 0;
            continue;
        }
        switch(state){
            case 0:
                info = calloc(1, sizeof(MasterInfo));
                addToList(&masterInfoList, info);
                strncpy(info->name, line, sizeof(info->name));
                info->name[NAME_BUFFER - 1] = 0;
                break;
            case 1:
                info->focusColor = (int)strtol(line, NULL, 16);
                break;
            default:
                addToList(&info->slaves, strncpy(malloc(len + 1), line, len + 1));
        }
        state++;
    }
    free(line);
    fclose(fp);
    return 1;
}
