#include "boundfunction.h"
#include "mywm-structs.h"
#include "user-events.h"
#include "util/arraylist.h"
#include "util/debug.h"
#include "util/logger.h"
#include <stdlib.h>

/// Holds batch events
typedef struct {
    /// how many times the event has been trigged
    int counter;
    /// the list of events to trigger when counter is non zero
    ArrayList list;
} BatchEventList ;

/// Holds an Arraylist of rules that will be applied in response to various conditions
ArrayList eventRules[NUMBER_OF_MPX_EVENTS];
BatchEventList batchEventRules[NUMBER_OF_MPX_EVENTS];

ArrayList* getEventList(int type, bool batch) {
    return batch ? &batchEventRules[type].list : &eventRules[type];
}

void _addEvent(ArrayList* arr, const BoundFunction func) {
    BoundFunction* p = malloc(sizeof(BoundFunction));
    *p = func;
    addElement(arr, p);
    for(int i = arr->size - 2; i >= 0; i--) {
        if(func.priority < ((BoundFunction*)getElement(arr, i))->priority) {
            swapElements(arr, i, arr, i + 1);
        }
    }
}
void addEvent(UserEvent type, const BoundFunction func) {
    _addEvent(getEventList(type, 0), func);
}
void addBatchEvent(UserEvent type, const BoundFunction func) {
    _addEvent(getEventList(type, 1), func);
}

void clearAllRules() {
    for(int i = 0; i < NUMBER_OF_MPX_EVENTS; i++) {
        FOR_EACH(BoundFunction*, f, &eventRules[i]) {
            free(f);
        }
        FOR_EACH(BoundFunction*, f, &batchEventRules[i].list) {
            free(f);
        }
        clearArray(&eventRules[i]);
        clearArray(&batchEventRules[i].list);
    }
}

static bool applyRules(ArrayList* rules, void* p) {
    INFO("Attempting to apply %d rules", rules->size);
    FOR_EACH(BoundFunction*, func, rules) {
        DEBUG("Running func: %s %p", func->name, p);
        pushContext(func->name);
        int abort = 0;
        if(func->intFunc)
            abort = !func->func.intFunc(p, func->arg) && func->abort;
        else
            func->func.func(p, func->arg);
        popContext();
        if(abort) {
            INFO("Rules aborted early due to: %s", func->name);
            return 0;
        }
    }
    return 1;
}
void incrementBatchEventRuleCounter(UserEvent i) {
    assert(i < LEN(batchEventRules));
    batchEventRules[i].counter++;
}
int getNumberOfEventsTriggerSinceLastIdle(UserEvent type) {
    return batchEventRules[type].counter;
}
void applyBatchEventRules(void) {
    pushContext("BATCH");
    for(UserEvent i = 0; i < NUMBER_OF_MPX_EVENTS; i++)
        if(getNumberOfEventsTriggerSinceLastIdle(i)) {
            pushContext(eventTypeToString(i));
            INFO("Event occurred: %d", getNumberOfEventsTriggerSinceLastIdle(i));
            if(batchEventRules[i].list.size)
                applyRules(&batchEventRules[i].list, NULL);
            popContext();
            batchEventRules[i].counter = 0;
        }
    popContext();
}
bool applyEventRules(UserEvent type, void* p) {
    incrementBatchEventRuleCounter(type);
    pushContext(eventTypeToString(type));
    bool result = applyRules(&eventRules[type], p);
    popContext();
    return result;
}
