#include <execinfo.h>

#include <unistd.h>

#include "logger.h"
#include "globals.h"
#include "system.h"
#include "xsession.h"

static LogLevel LOG_LEVEL = LOG_LEVEL_INFO;

LogLevel getLogLevel() {
    return LOG_LEVEL;
}
void setLogLevel(LogLevel level) {
    LOG_LEVEL = level;
}

void printStackTrace(void) {
    void* array[32];
    // get void*'s for all entries on the stack
    size_t size = backtrace(array, 32);
    // print out all the frames to stderr
    backtrace_symbols_fd(array, size, STDOUT_FILENO);
}

ArrayList<std::string> contexts;
void pushContext(std::string context) {
    contexts.add(context);
}
void popContext() {
    assert(contexts.size());
    contexts.pop();
}
std::string getContextString() {
    std::string result;
    result = std::to_string(RESTART_COUNTER) + "|" + std::to_string(getCurrentSequenceNumber()) + "|" + std::to_string(
            getLastDetectedEventSequenceNumber()) + "|" + std::to_string(getIdleCount());
    for(auto s : contexts)
        result += "[" + s + "]";
    return result + " ";
}
