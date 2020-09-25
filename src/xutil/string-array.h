#ifndef MPX_XUTIL_STRING_
#define MPX_XUTIL_STRING_

typedef struct StringJoiner {
    char* __buffer;
    int bufferSize;
    int usedBufferSize;
} StringJoiner ;


void addString(StringJoiner* array, const char* str);

const char* getBuffer(StringJoiner* array);
void freeBuffer(StringJoiner* array);
#endif
