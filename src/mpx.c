
/// \cond
#include <assert.h>
#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
/// \endcond
#include "mpx.h"
#include "devices.h"
#include "bindings.h"

#include "globals.h"
#include "logger.h"
#include "xsession.h"
typedef struct MasterInfo{
    char name[NAME_BUFFER];
    int focusColor;
    ArrayList slaves;
}MasterInfo;
static ArrayList masterInfoList;
static int mpxEnabled;

static Rule deviceEventRule;
static Rule cleanupRule = CREATE_WILDCARD(BIND(endSplitMaster),.passThrough=NO_PASSTHROUGH);
static void attachActiveSlavesToMaster(int id){
    xcb_input_key_press_event_t*event=getLastEvent();
    LOG(LOG_LEVEL_TRACE,"device event %d %d %d %d\n\n",event->event_type,event->deviceid,event->sourceid,event->flags);
    int masterId;
    Master*master=getMasterById(id);
    switch(event->event_type){
    case XCB_INPUT_KEY_PRESS:
    case XCB_INPUT_KEY_RELEASE:
        masterId=master->id;
        break;
    default:
        masterId=master->pointerId;
    }
    attachSlaveToMaster(event->sourceid,masterId);
}

static void autoAttachSlave(void){
    xcb_input_hierarchy_event_t*event=getLastEvent();
    if(event->flags & (XCB_INPUT_HIERARCHY_MASK_SLAVE_ADDED|XCB_INPUT_HIERARCHY_MASK_DEVICE_ENABLED) )
        restoreMPX();
}
void addAutoMPXRules(void){
    
    static Rule autoAttachRule= CREATE_WILDCARD(BIND(autoAttachSlave));
    static Rule attachOnStartRule= CREATE_WILDCARD(BIND(restoreMPX));
    prependRule(GENERIC_EVENT_OFFSET+XCB_INPUT_HIERARCHY,&autoAttachRule);
    prependRule(onXConnection,&attachOnStartRule);
}
int splitMaster(void){
    endSplitMaster();

    grabAllMasterDevices();
    createMasterDevice("dummy");
    initCurrentMasters();
    setActiveMaster(getValue(getAllMasters()));

    deviceEventRule =(Rule) CREATE_WILDCARD(BIND(attachActiveSlavesToMaster,getActiveMaster()->id),.passThrough=NO_PASSTHROUGH);
    prependRule(Idle,&cleanupRule);
    prependRule(GENERIC_EVENT_OFFSET+XCB_INPUT_KEY_PRESS,&deviceEventRule);
    prependRule(GENERIC_EVENT_OFFSET+XCB_INPUT_BUTTON_PRESS,&deviceEventRule);
    return getActiveMasterKeyboardID();
}
void endSplitMaster(void){
    removeRule(GENERIC_EVENT_OFFSET+XCB_INPUT_KEY_PRESS,&deviceEventRule);
    removeRule(GENERIC_EVENT_OFFSET+XCB_INPUT_BUTTON_PRESS,&deviceEventRule);
    removeRule(Idle,&cleanupRule);
    
}
char*getNameOfMaster(Master*master){
    return master->name;
}
char*getNameOfSlave(SlaveDevice*slave){
    return slave->name;
}

char* getMasterNameForSlave(char*slaveName){
    for(int i=0;i<getSizeOfList(&masterInfoList);i++){
        ArrayList* slaves=&((MasterInfo*)getElement(&masterInfoList,i))->slaves;
        for(int n=0;n<getSizeOfList(slaves);n++)
            if(strcmp(slaveName,getElement(slaves,n))==0)
                return ((MasterInfo*)getElement(&masterInfoList,i))->name;
    }
    return NULL;
}
Master*getMasterForSlave(char*slaveName){
    Node*masters=getAllMasters();
    masters=getAllMasters();
    char*masterName=getMasterNameForSlave(slaveName);
    if(!masterName)return NULL;
    UNTIL_FIRST(masters,
        strcmp(getNameOfMaster(getValue(masters)),masterName)==0); 
    return getValue(masters);
}
int getMasterIdForSlave(SlaveDevice*slaveDevice){
    Master*master=getMasterForSlave(getNameOfSlave(slaveDevice));
    return !master?0:slaveDevice->keyboard?master->id:master->pointerId;
}

void restoreFocusColor(Master*master){
    master->focusColor=DEFAULT_FOCUS_BORDER_COLOR;
    for(int i=0;i<getSizeOfList(&masterInfoList);i++)
        if(strcmp(((MasterInfo*)getElement(&masterInfoList,i))->name,master->name)==0){
            master->focusColor=((MasterInfo*)getElement(&masterInfoList,i))->focusColor;
            break;
        }
}

void attachSlaveToPropperMaster(SlaveDevice* slaveDevice){
    int id=getMasterIdForSlave(slaveDevice);
    if(id)
        attachSlaveToMaster(slaveDevice->id,id);
}
void restoreMPX(void){
    Node*n=getAllMasters();
    FOR_EACH(n,restoreFocusColor(getValue(n)));
    int num;
    Node*slaves=getSlavesOfMasterByID(NULL,0,&num);
    Node*copy=slaves;
    FOR_EACH(slaves, attachSlaveToPropperMaster(getValue(slaves)));
    deleteList(copy);
}
void startMPX(void){
    mpxEnabled=1;
    for(int i=0;i<getSizeOfList(&masterInfoList);i++){
        char *name=((MasterInfo*)getElement(&masterInfoList,i))->name;
        Node*masters=getAllMasters();
        UNTIL_FIRST(masters,strcmp(getNameOfMaster(getValue(masters)),name)==0);
        if(!masters)
            createMasterDevice(name);
    }
    initCurrentMasters();
    restoreMPX();
}
void stopMPX(void){
    mpxEnabled=0;
    destroyAllNonDefaultMasters();
    flush();
}

int saveMasterInfo(void){
    Node*masterNode=getAllMasters();
    FILE * fp = fopen(MASTER_INFO_PATH, "w");
    if(!fp)
        return 0;
    FOR_EACH(masterNode,
        Master*master=getValue(masterNode);
        fprintf(fp,"%s\n",getNameOfMaster(master));
        fprintf(fp,"%06X\n",master->focusColor);
        Node*slave=getSlavesOfMaster(master);
        Node*copy=slave;
        FOR_EACH(slave,
            fprintf(fp,"%s\n",getNameOfSlave(getValue(slave)));
        )
        deleteList(copy);
        fprintf(fp,"\n");
    )
    fclose(fp);
    return 1;
}
int loadMasterInfo(void){
    FILE * fp;
    char * line = NULL;
    size_t bSize = 0;
    ssize_t len;

    fp = fopen(MASTER_INFO_PATH, "r");
    if (fp == NULL)
        return 0;
    LOG(LOG_LEVEL_INFO,"Loading saved MPX state\n");
    for(int i=0;i<getSizeOfList(&masterInfoList);i++){
        ArrayList* slaves=&((MasterInfo*)getElement(&masterInfoList,i))->slaves;
        for(int n=0;n<getSizeOfList(slaves);n++)
            free(getElement(slaves,n));
        clearArrayList(slaves);
        free(getElement(&masterInfoList,i));
    }
    clearArrayList(&masterInfoList);
    initArrayList(&masterInfoList);
    MasterInfo*info=NULL;
    int state=0;
    while ((len = getline(&line, &bSize, fp)) != -1) {
        if(line[len-1]=='\n')
            line[--len]=0;
        if(len==0){
            state=0;
            continue;
        }
        switch(state){
        case 0:
            info=malloc(sizeof(MasterInfo));
            addToList(&masterInfoList,info);
            strncpy(info->name,line,sizeof(info->name));
            initArrayList(&info->slaves);
            break;
        case 1:
            info->focusColor=(int)strtol(line,NULL,16);
            break;
        default:
            addToList(&info->slaves,strncpy(malloc(len+1),line,len+1));
        }
        state++;
    }
    free(line);
    fclose(fp);
    return 1;
}
