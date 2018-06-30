/**
 * @file event-loop.h
 *
 * @brief Handles geting and processing XEvents
 */

#ifndef EVENT_LOOP_H_
#define EVENT_LOOP_H_
#include "mywm-structs.h"

void *runEventLoop(void*context);
void onGenericEvent();
#endif /* EVENT_LOOP_H_ */
