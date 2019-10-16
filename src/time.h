/**
 * @file time.h
 * time related methods
 */
#ifndef MPXMANAGER_TIME_H_
#define MPXMANAGER_TIME_H_
#include <chrono>
#include <thread>
#include <sys/time.h>

/**
 * Sleep of mil milliseconds
 * @param mil number of milliseconds to sleep
 */
static inline void msleep(int mil) {
    std::this_thread::sleep_for(std::chrono::milliseconds(mil));
}

/**
 * Returns a monotonically increasing number that servers the time in ms.
 * @return the current time (ms)
 */
static inline unsigned int getTime() {
    struct timeval start;
    gettimeofday(&start, NULL);
    return start.tv_sec * 1000 + start.tv_usec * 1e-3;
}
#endif
