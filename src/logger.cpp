/**
 * @file logger.cpp
 *
 * @copybrief logger.h
 */
#include "logger.h"
#include "masters.h"
#include "monitors.h"
#include "slaves.h"
#include "windows.h"
#include "workspaces.h"
#include <execinfo.h>

#ifndef INIT_LOG_LEVEL
    #define INIT_LOG_LEVEL LOG_LEVEL_INFO
#endif

static int LOG_LEVEL = INIT_LOG_LEVEL;

int getLogLevel() {
    return LOG_LEVEL;
}
void setLogLevel(uint32_t level) {
    LOG_LEVEL = level;
}
void printSummary(void) {
    LOG(LOG_LEVEL_INFO, "Summary:\n");
    std::cout << "Slaves: "    << getAllSlaves() << "\n";
    std::cout << "Masters: "    << getAllMasters() << "\n";
    std::cout << "Monitors: "   << getAllMonitors() << "\n";
    std::cout << "Workspaces: " << getAllWorkspaces() << "\n";
    std::cout << "Windows: "    << getAllWindows() << "\n";
}
void dumpWindow(WindowMask filterMask) {
    for(WindowInfo* winInfo : getAllWindows()) {
        if(!filterMask ||  winInfo->hasMask(filterMask))
            std::cout << winInfo << "\n";
    }
}
void dumpWindow(std::string match) {
    for(WindowInfo* winInfo : getAllWindows()) {
        if(winInfo->matches(match))
            std::cout << winInfo << "\n";
    }
}

void printStackTrace(void) {
    void* array[32];
    // get void*'s for all entries on the stack
    size_t size = backtrace(array, 32);
    // print out all the frames to stderr
    backtrace_symbols_fd(array, size, LOG_FD);
}
