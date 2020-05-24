/**
 * @file
 * @brief RingBuffer implementation
 */
#ifndef RING_BUFFER_H
#define RING_BUFFER_H
#include <assert.h>
#include <stddef.h>
#include <stdint.h>

/**
 * This class is intended to have a 1 read thread and one thread write.
 * Multiple threads trying to read or trying to write will cause problems
 *
 * @tparam T the type to hold
 * @tparam I the size of the buffer
 */
template < class T, int I = 1 << 10 >
struct RingBuffer {
    T eventBuffer[I];
    /// next index to read from
    uint16_t bufferIndexRead = 0;
    /// next index to write to
    uint16_t bufferIndexWrite = 0;
    volatile uint16_t size = 0;
    uint16_t getSize() const {return size;}
    uint16_t getMaxSize() const {return I;}
    bool isFull() const {return getSize() == getMaxSize();}
    bool isEmpty() const {return !getSize();}
    T pop() {
        if(isEmpty())
            return NULL;
        auto element = eventBuffer[bufferIndexRead++ % getMaxSize()];
        size--;
        return element;
    }
    T peek() {
        return eventBuffer[bufferIndexRead % getMaxSize()];
    }
    T peekEnd() {
        return eventBuffer[(getMaxSize() + bufferIndexWrite - 1) % getMaxSize()];
    }
    bool push(T event) {
        if(!isFull() && event) {
            eventBuffer[bufferIndexWrite++ % getMaxSize()] = event;
            size++;
        }
        return event && !isFull();
    }
};
#endif
