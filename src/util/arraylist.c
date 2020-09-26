#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "arraylist.h"
void* getElement(const ArrayList* array, int index) {
    return array->__arr[index];
}

void addElement(ArrayList* array, void* p) {
    if(array->size == array->maxSize) {
        if(array->maxSize)
            array->maxSize *= 2;
        else array->maxSize = 16;
        array->__arr = realloc(array->__arr, sizeof(void*)*array->maxSize);
    }
    array->__arr[array->size++] = p;
}

int getIndex(const ArrayList* array, const void* p, size_t size) {
    for(int i = 0; i < array->size; i++) {
        if(memcmp(p, array->__arr[i], size) == 0)
            return i;
    }
    return -1;
}
void* findElement(const ArrayList* array, const void* p, size_t size) {
    int index  = getIndex(array, p, size);
    return index == -1 ? NULL : array->__arr[index];
}

void* removeElement(ArrayList* array, const void* p, size_t size) {
    int index = getIndex(array, p, size);
    return index != -1 ? removeIndex(array, index) : NULL;
}
void* removeIndex(ArrayList* array, uint32_t index) {
    void* value = getElement(array, index);
    for(uint32_t i = index; i < array->size - 1; i++)
        array->__arr[i] = array->__arr[i + 1];
    array->size--;
    return value;
}
void shiftToPos(ArrayList* array, uint32_t index, int endingPos) {
    assert(index >= endingPos);
    void* value = getElement(array, index);
    for(int i = index; i > endingPos; i--)
        array->__arr[i] = array->__arr[i - 1];
    array->__arr[endingPos] = value;
}
void addElementAt(ArrayList* array, void* value, uint32_t index) {
    addElement(array, value);
    shiftToPos(array, array->size - 1, index);
}
void swapElements(ArrayList* array1, uint32_t index1, ArrayList* array2,  uint32_t index2) {
    assert(index1 < array1->size && index2 < array2->size);
    void* temp = array1->__arr[index1];
    array1->__arr[index1] = array2->__arr[index2];
    array2->__arr[index2] = temp;
}
void clearArray(ArrayList* array) {
    free(array->__arr);
    array->__arr = NULL;
    array->size = 0;
    array->maxSize = 0;
}
