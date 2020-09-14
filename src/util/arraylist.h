/**
 * @file arraylist.h
 * @brief ArrayList implementation
 */
#ifndef ARRAY_LIST_H
#define ARRAY_LIST_H

#include <stddef.h>
#include <stdint.h>


#define __VAR_CAT_HELPER(x, y) x##y
#define __VAR_CAT(x, y) __VAR_CAT_HELPER(x, y)

#define FOR_EACH(TYPE, VAR, ARR) int __VAR_CAT(__i, __LINE__) = 0;\
    for(TYPE VAR = (ARR)->size?getElement(ARR, __VAR_CAT(__i, __LINE__)):NULL; __VAR_CAT(__i, __LINE__) < (ARR)->size; VAR=getElement(ARR, ++__VAR_CAT(__i, __LINE__)))
#define FOR_EACH_R(TYPE, VAR, ARR) int __VAR_CAT(__i, __LINE__) = (ARR)->size;\
    for(TYPE VAR = (ARR)->size?getElement(ARR, __VAR_CAT(__i, __LINE__)-1):NULL; --__VAR_CAT(__i, __LINE__) >=0 && (VAR=getElement(ARR, __VAR_CAT(__i, __LINE__)));)
typedef struct ArrayList {
    void** __arr;
    int size;
    int maxSize;
} ArrayList;

void* getElement(const ArrayList* array, int index);
static inline void* getHead(const ArrayList* array) { return getElement(array, 0);}
void addElement(ArrayList* array, void* p);
int getIndex(const ArrayList* array, const void* p, size_t size);
void* findElement(const ArrayList* array, const void* p, size_t size);
void* removeElement(ArrayList* array, const void* p, size_t size);
/**
 * Remove the index-th element form list
 * @param index
 * @return the element that was removed
 */
void* removeIndex(ArrayList* array, uint32_t index);
static inline void push(ArrayList* array, void* p) {addElement(array, p);}
static inline void* pop(ArrayList* array) {return removeIndex(array, array->size - 1);}
static inline void* peek(ArrayList* array) {return getElement(array, array->size - 1);}
/**
 * Removes the index-th element and re-inserts it at the end of the list
 * @param index
 */
static inline void shiftToEnd(ArrayList* array, uint32_t index) { addElement(array, removeIndex(array, index)); }
/**
 * Moves the index-th element to position endingPos.
 * @param index a valid position in the array >= endingPos
 * @param endingPos
 */
void shiftToPos(ArrayList* array, uint32_t index, int endingPos);
void addElementAt(ArrayList* array, void* value, uint32_t index);
/**
 * Removes the index-th element and re-inserts it at the head of the list
 * @param index
 */
static inline void shiftToHead(ArrayList* array, uint32_t index) { shiftToPos(array, index, 0);};
/**
 * Swaps the element at index1 with index2
 * @param index1
 * @param index2
 */
void swapElements(ArrayList* array1, uint32_t index1, ArrayList* array2,  uint32_t index2);
void clearArray(ArrayList* array);

/**
 * Returns the element at index current + delta if the array list where to wrap around
 * For example:
 * getNextIndex(0,-1) returns size-1
 * getNextIndex(size-1,1) returns 0
 * @param current
 * @param delta a number between [-array.size, array.size]
 */
static inline uint32_t getNextIndex(const ArrayList* array, int current, int delta) {
    return (current + delta + array->size) % array->size;
}

#endif
