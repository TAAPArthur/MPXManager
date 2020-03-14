#include "boundfunction.h"
#include "logger.h"
#include "user-events.h"
#include "xsession.h"

struct EventList: UniqueArrayList<BoundFunction*> {
    void add(BoundFunction* value) override {
        push_back(value);
        sort();
    }
};

/// Holds batch events
struct BatchEventList {
    /// how many times the event has been trigged
    int counter;
    /// the list of events to trigger when counter is non zero
    EventList list;
} ;

/// Holds an Arraylist of rules that will be applied in response to various conditions
static EventList eventRules[NUMBER_OF_MPX_EVENTS];
static BatchEventList batchEventRules[NUMBER_OF_MPX_EVENTS];

void clearAllRules() {
    for(int i = 0; i < NUMBER_OF_MPX_EVENTS; i++) {
        getEventRules(i).deleteElements();
        getBatchEventRules(i).deleteElements();
    }
}
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
static int applyRules(ArrayList<BoundFunction*>& rules, const BoundFunctionArg& arg = {}) {
    for(BoundFunction* func : rules) {
        DEBUG("Running func: " << func->getName());
        pushContext(func->getName());
        if(!func->execute(arg)) {
            popContext();
            INFO("Rules aborted early due to: " << func->getName());
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
            LOG(LOG_LEVEL_INFO, "Applying Batch rules %d (count:%d) %s number of rules: %d", i, batchEventRules[i].counter,
                eventTypeToString(i), getBatchEventRules(i).size());
            pushContext("BATCH_" + std::string(eventTypeToString(i)));
            applyRules(getBatchEventRules(i));
            popContext();
            batchEventRules[i].counter = 0;
        }
}
int applyEventRules(int type, const BoundFunctionArg& arg) {
    LOG(LOG_LEVEL_INFO, "Event detected %d %s number of rules: %d parameters",
        type, eventTypeToString(type), getEventRules(type).size());
    incrementBatchEventRuleCounter(type);
    pushContext(eventTypeToString(type));
    int result = applyRules(getEventRules(type), arg);
    popContext();
    return result;
}
bool BoundFunction::execute(const BoundFunctionArg& arg)const {
    return shouldPassThrough(passThrough, call(arg));
}
int BoundFunction::call(const BoundFunctionArg& arg)const {
    return func ? func->call(arg) : 1;
}
bool BoundFunction::operator==(const BoundFunction& boundFunction)const {
    return boundFunction.name != "" && name == boundFunction.name;
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
