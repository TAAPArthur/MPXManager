#include "logger.h"
#include "masters.h"
#include "monitors.h"
#include "slaves.h"
#include "windows.h"
#include "workspaces.h"
#include "xsession.h"
#include <execinfo.h>

static int LOG_LEVEL = LOG_LEVEL_INFO;


int getLogLevel() {
    return LOG_LEVEL;
}
void setLogLevel(uint32_t level) {
    LOG_LEVEL = level;
}
void printSummary(void) {
    std::cout << "Summary:" << std::endl;
    std::cout << "Slaves: " << getAllSlaves() << std::endl;
    std::cout << "Masters: " << getAllMasters() << std::endl;
    std::cout << "Monitors: " << getAllMonitors() << std::endl;
    std::cout << "Workspaces: " << getAllWorkspaces() << std::endl;
    dumpWindow();
}
void dumpWindow(WindowMask filterMask) {
    std::cout << "Dumping:" << std::endl;
    for(WindowInfo* winInfo : getAllWindows()) {
        if(!filterMask || winInfo->hasMask(filterMask))
            std::cout << *winInfo << std::endl;
    }
}
void dumpMaster(Master* master) {
    if(!master)
        master = getActiveMaster();
    std::cout << "Dumping:" << *master << std::endl;
    std::cout << master->getSlaves() << std::endl;
    for(WindowInfo* winInfo : master->getWindowStack()) {
        std::cout << *winInfo << std::endl;
    }
}
void dumpWindow(std::string match) {
    std::cout << "Dumping:" << std::endl;
    for(WindowInfo* winInfo : getAllWindows()) {
        if(winInfo->getClassName() == match || winInfo->getInstanceName() == match)
            std::cout << *winInfo << std::endl;
    }
}
void dumpSingleWindow(WindowID win) {
    WindowInfo* winInfo = getWindowInfo(win);
    if(!winInfo) {
        std::cout << "No window with id " << win << std::endl;
        return;
    }
    std::cout << "Dumping:" << std::endl;
    std::cout << *winInfo << std::endl;
}
void dumpWindowStack() {
    std::cout << "Dumping Window Stack:" << std::endl;
    for(WindowInfo* winInfo : getActiveWorkspace()->getWindowStack())
        std::cout << *winInfo << std::endl;
}

Logger logger;
void printStackTrace(void) {
    void* array[32];
    // get void*'s for all entries on the stack
    size_t size = backtrace(array, 32);
    // print out all the frames to stderr
    backtrace_symbols_fd(array, size, LOG_FD);
}

ArrayList<std::string> contexts;
void pushContext(std::string context) {
    contexts.add(context);
}
void popContext() {
    contexts.pop();
}
std::string getContextString() {
    std::string result = std::to_string(getCurrentSequenceNumber()) + "|" + std::to_string(
            getLastDetectedEventSequenceNumber()) + "|" + std::to_string(getIdleCount());
    for(auto s : contexts)
        result += "[" + s + "]";
    return result + " ";
}
