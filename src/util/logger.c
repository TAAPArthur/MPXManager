#include <assert.h>
#include <stdio.h>
#include <unistd.h>

#include "../globals.h"
#include "../system.h"
#include "../xevent.h"
#include "arraylist.h"
#include "logger.h"

static LogLevel LOG_LEVEL = 0;

LogLevel getLogLevel() {
    return LOG_LEVEL;
}
void setLogLevel(LogLevel level) {
    LOG_LEVEL = level;
}

ArrayList context;
void pushContext(const char* c) {
    addElement(&context, (char*)c);
}
void popContext() {
    assert(context.size);
    removeIndex(&context, context.size - 1);
}

int getEventQueueSize();
void printContextStr() {
    // TODO
    printf("%d|%04X|%04X|", RESTART_COUNTER, getCurrentSequenceNumber(), getIdleCount());
    for(int i = 0; i < context.size; i++)
        printf("[%s]", (char*)getElement(&context, i));
}
