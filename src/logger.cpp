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


__attribute__((constructor)) static void set_autoflush() {
    std::cout << std::unitbuf;
}
int getLogLevel() {
    return LOG_LEVEL;
}
void setLogLevel(uint32_t level) {
    LOG_LEVEL = level;
}
void printSummary(void) {
    std::cout << "Summary:\n";
    std::cout << "Slaves: "    << getAllSlaves() << "\n";
    std::cout << "Masters: "    << getAllMasters() << "\n";
    std::cout << "Monitors: "   << getAllMonitors() << "\n";
    std::cout << "Workspaces: " << getAllWorkspaces() << "\n";
    dumpWindow();
}
void dumpWindow(WindowMask filterMask) {
    std::cout << "Dumping:\n";
    for(WindowInfo* winInfo : getAllWindows()) {
        if(!filterMask ||  winInfo->hasMask(filterMask))
            std::cout << *winInfo << "\n";
    }
}
void dumpWindow(std::string match) {
    std::cout << "Dumping:\n";
    for(WindowInfo* winInfo : getAllWindows()) {
        if(winInfo->getClassName() == match || winInfo->getInstanceName() == match)
            std::cout << *winInfo << "\n";
    }
}
void dumpWindowStack() {
    std::cout << "Dumping Window Stack:\n";
    for(WindowInfo* winInfo : getActiveWorkspace()->getWindowStack())
        std::cout << *winInfo << "\n";
}

void printStackTrace(void) {
    void* array[32];
    // get void*'s for all entries on the stack
    size_t size = backtrace(array, 32);
    // print out all the frames to stderr
    backtrace_symbols_fd(array, size, LOG_FD);
}
