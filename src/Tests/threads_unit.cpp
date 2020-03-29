#include <errno.h>
#include <signal.h>
#include <pthread.h>

#include "../threads.h"
#include "test-mpx-helper.h"
#include "tester.h"

static int size = 100000;
static volatile int counter = 0;
void incrementCounter() {
    for(int i = 0; i < size; i++) {
        lock();
        counter++;
        unlock();
    }
}
MPX_TEST("run_in_new_thread", {
    spawnThread(incrementCounter);
    waitForAllThreadsToExit();
    assert(counter == size);
});
// make volatile
int getSize(ArrayList<long>* list) {return list->size();}
MPX_TEST("thread_lock_unlock", {
    spawnThread(incrementCounter);
    spawnThread(incrementCounter);
    waitForAllThreadsToExit();
    assertEquals(counter, size * 2);
});
MPX_TEST("safe_shutdown", {
    spawnThread([]{
        while(!isShuttingDown())msleep(100);
    });
    requestShutdown();
    waitForAllThreadsToExit();
});
static ThreadSignaler signaler;
static ThreadSignaler signaler2;
MPX_TEST("thread_signal_wakeup_sync", {
    signaler.signal();
    signaler.justWait();
});
MPX_TEST("thread_signal_wakeup_sync_reuse", {
    signaler.signal();
    signaler.sleepOrWait(1);
    signaler.sleepOrWait(1);
});
MPX_TEST_ITER("thread_signal_wakeup_async", 2, {
    if(_i)
        signaler.signal();
    spawnThread([]{signaler.justWait();});
    if(!_i) {
        msleep(100);
        signaler.signal();
    }
    waitForAllThreadsToExit();
});

MPX_TEST("thread_signal", {
    const int size = 5;
    int num = 0;
    spawnThread([&]{
        for(int i = 1; i < size; i++) {
            num += i;
            signaler.signal();
            signaler2.justWait();
        }
    });
    spawnThread([&]{
        for(int i = 1; i < size; i++) {
            signaler.justWait();
            num *= i;
            signaler2.signal();
        }
    });
    waitForAllThreadsToExit();
    assertEquals(num, 124);
});
