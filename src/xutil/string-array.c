#include <string.h>
#include <stdlib.h>


#include "string-array.h"

void addString(StringJoiner* array, const char* str) {
    const int requiredSize = array->usedBufferSize + (strlen(str)) + 1;
    if(array->bufferSize < requiredSize) {
        if(array->bufferSize)
            array->bufferSize *= 2;
        else array->bufferSize = 64;
        array->bufferSize += requiredSize;
        array->__buffer = realloc(array->__buffer, sizeof(void*)*array->bufferSize);
    }
    strcpy(&array->__buffer[array->usedBufferSize], str);
    array->usedBufferSize += strlen(str) + 1;
}
const char* getBuffer(StringJoiner* array) {
    return array->__buffer;
}
void freeBuffer(StringJoiner* array) {
    free(array->__buffer);
}

bool isInBuffer(StringJoiner* array, const char* str) {
    int offset = 0;
    while(offset < array->usedBufferSize) {
        const char* element = getBuffer(array) + offset;
        int len = strlen(element);
        if(strcmp(str, element) == 0)
            return 1;
        offset += len + 1;
    }
    return 0;
}
