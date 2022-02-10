/**
 * @file boundfunction.h
 * @brief FunctionWrapper and Rules
 */
#ifndef MPX_BOUND_FUNCTION_H
#define MPX_BOUND_FUNCTION_H

#include <assert.h>
#include <stdint.h>

#include "user-events.h"
#include "mywm-structs.h"

typedef int8_t FunctionPriority;
/// @{
/// Controls when a binding will be executed in the queue for its event type.
/// [-128, 127] where higher priorities (lower numbers) are executed first
#define HIGHEST_PRIORITY (-128)
#define HIGHER_PRIORITY  (-64)
#define HIGH_PRIORITY    (-32)
#define NORMAL_PRIORITY  (  0)
#define LOW_PRIORITY     ( 32)
#define LOWER_PRIORITY   ( 64)
#define LOWEST_PRIORITY  (127)
/// @}

typedef struct BoundFunction {
    union {
        void(*func)();
        bool(*intFunc)();
    } func;
    const char* name;
    bool intFunc;
    FunctionPriority priority;
    char abort;
    Arg arg;
} BoundFunction;
/// @{
/// Creates a BoundFunction with a name based on F with a preset priority
/// @param F the function to call
/// @param args
#define __EVENT(F, N, T, P...) ((const BoundFunction){F, N, T, P})
#define DEFAULT_EVENT(F, P...) __EVENT({.func = F}, "_" #F, 0, P)
#define FILTER_EVENT(F, P...) __EVENT({.intFunc = F}, "_" #F, 1, P)
#define USER_EVENT(F, P...) __EVENT({.func = F}, #F, 0, P)
#define USER_FILTER_EVENT(F, P...) __EVENT({.intFunc = F}, #F, 1, P)
/// @}
void addEvent(UserEvent type, const BoundFunction func);
void addBatchEvent(UserEvent type, const BoundFunction func);

/**
 * Deletes all normal and batch event rules
 */
void clearAllRules();

/**
 * Increment the counter for a batch rules.
 * All batch rules with a non zero counter will be trigged
 * on the next call to incrementBatchEventRuleCounter()
 *
 * @param i the event type
 */
void incrementBatchEventRuleCounter(UserEvent i);
/**
 * Returns the number of times an event has been triggered since the last corresponding batch event call;
 *
 * @param type
 *
 * @return
 */
int getNumberOfEventsTriggerSinceLastIdle(UserEvent type);

/**
 * For every event rule that has been triggered since the last call
 * incrementBatchEventRuleCounter(), trigger the corresponding batch rules
 *
 */
void applyBatchEventRules(void);
/**
 *
 * Apply the list of rules designated by the type
 *
 * @param type
 * @param arg
 *
 * @return the result
 */
bool applyEventRules(UserEvent type, void* p);
#endif
