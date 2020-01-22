
#ifndef MPX_THREADS
#define MPX_THREADS

#include <functional>
#include "time.h"
#include "globals.h"

/**
 * execute code in an atomic manner
 * @param code
 */
#define ATOMIC(code...) do {lock();code;unlock();}while(0)

void registerWakeupFunction(void(*func)());

/**
 * Locks/unlocks the global mutex
 *
 * It is not safe to modify most structs from multiple threads so the main event loop lock/unlocks a
 * global mutex. Any addition thread that runs alongside the main thread of if in general, there
 * is a race, lock/unlock should be used
 */
void lock(void);
///copydoc lock(void)
void unlock(void);
struct _CondMutex;
/**
 * Abstracts the wait and notify thread semantics
 * This class is used to coordinate actions between to threads that have some shared state in an attempt to
 * prevent busy sleeps
 */
class ThreadSignaler {
    _CondMutex* condMutex;
    volatile bool ready = 0;
public:
    ThreadSignaler();
    /**
     * Runs func under a lock and then notifies one or all waiting threads
     *
     * @param func
     * @param all determines where a single thread or all threads are notified
     */
    void signal(std::function<void()> func, bool all = 0);
    /**
     * Notifies threads that are blocked in justWait()
     * @param all
     */
    void signal(bool all = 0) {signal([&] {this->ready = 1;}, all);}
    /**
     * Blocks until the thread is signaled and func returns true
     *
     * @param  func
     * @param noResetSignal if set this->ready will be cleared; Only matters if waiting on ready
     */
    void justWait(std::function<bool()> func, bool noResetSignal = 0);
    /**
     * Blocks until signal(bool) has been called
     */
    void justWait() {justWait([&] {return this->ready;});}
    /**
     * Either sleeps delay ms or blocks until signaled
     *
     * @param condition whether to sleep or block
     * @param delay the time to sleep if condition is true
     */
    void sleepOrWait(uint32_t condition, long delay = 100) {
        if(condition)
            msleep(delay);
        else
            justWait();
    }
};
/**
 * Runs method in a new thread
 * @param func the method to run
 */
void spawnThread(std::function<void()>func, const char* name);
/**
 * @return the number of threads that have not terminated and been cleaned up
 */
int getNumberOfThreads(void);

/**
 * Joins all threads started via runInNewThread
 */
void waitForAllThreadsToExit(void);
#endif
