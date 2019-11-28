#include "user-events.h"
#include "boundfunction.h"
#include "logger.h"
#include "xsession.h"


/// Holds batch events
struct BatchEventList {
    /// how many times the event has been trigged
    int counter;
    /// the list of events to trigger when counter is non zero
    UniqueArrayList<BoundFunction*> list;
} ;

/// Holds an Arraylist of rules that will be applied in response to various conditions
static UniqueArrayList<BoundFunction*> eventRules[NUMBER_OF_MPX_EVENTS];
static BatchEventList batchEventRules[NUMBER_OF_MPX_EVENTS];

ArrayList<BoundFunction*>& getEventRules(int i) {
    assert(i < LEN(eventRules));
    return eventRules[i];
}

ArrayList<BoundFunction*>& getBatchEventRules(int i) {
    assert(i < LEN(batchEventRules));
    return batchEventRules[i].list;
}
void incrementBatchEventRuleCounter(int i) {
    assert(i < LEN(batchEventRules));
    batchEventRules[i].counter++;
}
static int applyRules(ArrayList<BoundFunction*>& rules, WindowInfo* winInfo, Master* m = NULL) {
    for(BoundFunction* func : rules) {
        logger.debug() << "Running func (" << (m ? m->getID() : 0) << ", " << (winInfo ? winInfo->getID() : 0) << "):" <<
            func->getName() << std::endl;
        pushContext(func->getName());
        if(!func->execute(winInfo, m)) {
            popContext();
            logger.info() << "Rules aborted early due to: " << func->getName() << std::endl;
            return 0;
        }
        popContext();
    }
    return 1;
}
int getNumberOfEventsTriggerSinceLastIdle(int type) {
    return batchEventRules[type].counter;
}
void applyBatchEventRules(void) {
    for(int i = 0; i < NUMBER_OF_BATCHABLE_EVENTS; i++)
        if(getNumberOfEventsTriggerSinceLastIdle(i)) {
            LOG(LOG_LEVEL_INFO, "Applying Batch rules %d (count:%d) %s number of rules: %d\n", i, batchEventRules[i].counter,
                eventTypeToString(i), getBatchEventRules(i).size());
            pushContext(eventTypeToString(i));
            applyRules(getBatchEventRules(i), NULL);
            popContext();
            batchEventRules[i].counter = 0;
        }
}
int applyEventRules(int type, WindowInfo* winInfo, Master* m) {
    LOG(LOG_LEVEL_INFO, "Event detected %d %s number of rules: %d parameters WindowID: %d MasterID: %d\n",
        type, eventTypeToString(type), getEventRules(type).size(), winInfo ? winInfo->getID() : 0, m ? m->getID() : 0);
    incrementBatchEventRuleCounter(type);
    pushContext(eventTypeToString(type));
    int result = applyRules(getEventRules(type), winInfo, m);
    popContext();
    return result;
}
bool BoundFunction::execute(WindowInfo* winInfo, Master* master)const {
    return shouldPassThrough(passThrough, call(winInfo, master));
}
int BoundFunction::call(WindowInfo* winInfo, Master* master)const {
    LOG(LOG_LEVEL_TRACE, "Calling func with %p %p\n", winInfo, master);
    return func ? func->call(winInfo, master) : 1;
}
bool BoundFunction::operator==(const BoundFunction& boundFunction)const {
    assert(name != "" || boundFunction.name != "");
    return name == boundFunction.name;
}
std::ostream& operator<<(std::ostream& stream, const BoundFunction& boundFunction) {
    return stream << boundFunction.getName();
}
void deleteAllRules() {
    for(int i = 0; i < NUMBER_OF_BATCHABLE_EVENTS; i++)
        getBatchEventRules(i).deleteElements();
    for(int i = 0; i < NUMBER_OF_MPX_EVENTS; i++)
        getEventRules(i).deleteElements();
}
