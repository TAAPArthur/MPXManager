#include "user-events.h"
#include "boundfunction.h"
#include "logger.h"
#include "xsession.h"


/// Holds batch events
struct BatchEventList{
    /// how many times the event has been trigged
    int counter;
    /// the list of events to trigger when counter is non zero
    UniqueArrayList<BoundFunction*> list;
} ;

/// Holds an Arraylist of rules that will be applied in response to various conditions
static UniqueArrayList<BoundFunction*> eventRules[MPX_LAST_EVENT];
static BatchEventList batchEventRules[MPX_LAST_EVENT];

ArrayList<BoundFunction*>& getEventRules(int i) {
    assert(i < MPX_LAST_EVENT);
    return eventRules[i];
}

ArrayList<BoundFunction*>& getBatchEventRules(int i) {
    assert(i < MPX_LAST_EVENT);
    return batchEventRules[i].list;
}
void incrementBatchEventRuleCounter(int i) {
    batchEventRules[i].counter++;
}
static int applyRules(ArrayList<BoundFunction*>& rules, WindowInfo* winInfo, Master* m = NULL) {
    for(BoundFunction* func : rules) {
        LOG_RUN(LOG_LEVEL_DEBUG, std::cout << "Running func: " << func->getName() << "\n");
        if(!func->execute(winInfo, m)) {
            LOG_RUN(LOG_LEVEL_DEBUG, std::cout << "Rules aborted early " << func->getName() << "\n");
            return 0;
        }
    }
    return 1;
}
int getNumberOfEventsTriggerSinceLastIdle(int type) {
    return batchEventRules[type].counter;
}
void applyBatchEventRules(void) {
    for(int i = 0; i < MPX_LAST_EVENT; i++)
        if(getNumberOfEventsTriggerSinceLastIdle(i)) {
            LOG(LOG_LEVEL_DEBUG, "Applying Batch rules %d (count:%d) %s number of rules: %d\n", i, batchEventRules[i].counter,
                eventTypeToString(i), getBatchEventRules(i).size());
            applyRules(getBatchEventRules(i), NULL);
            batchEventRules[i].counter = 0;
        }
}
int applyEventRules(int type, WindowInfo* winInfo, Master* m) {
    LOG(LOG_LEVEL_DEBUG, "Event detected %d %s number of rules: %d parameters WindowID: %d MasterID: %d\n",
        type, eventTypeToString(type), getEventRules(type).size(), winInfo ? winInfo->getID() : 0, m ? m->getID() : 0);
    incrementBatchEventRuleCounter(type);
    return applyRules(getEventRules(type), winInfo, m);
}
int BoundFunction::execute(WindowInfo* winInfo, Master* master)const {
    return shouldPassThrough(passThrough, call(winInfo, master));
}
int BoundFunction::call(WindowInfo* winInfo, Master* master)const {
    LOG(LOG_LEVEL_VERBOSE, "Calling func with %p %p\n", winInfo, master);
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
    for(int i = 0; i < MPX_LAST_EVENT; i++) {
        getEventRules(i).deleteElements();
        getBatchEventRules(i).deleteElements();
    }
}
