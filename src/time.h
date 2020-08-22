/**
 * @file time.h
 * time related methods
 */
#ifndef MPXMANAGER_TIME_H_
#define MPXMANAGER_TIME_H_
#include <time.h>
#include <sys/time.h>

/**
 * Sleep of mil milliseconds
 * @param ms number of milliseconds to sleep
 */
static inline void msleep(int ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    while(nanosleep(&ts, &ts));
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
