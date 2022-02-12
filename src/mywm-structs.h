/**
 * @file mywm-structs.h
 * @brief contains global struct definitions
 */
#ifndef MYWM_STRUCTS_H
#define MYWM_STRUCTS_H

#include "util/arraylist.h"
#include <stdint.h>

#define LEN(X) (sizeof(X) / sizeof((X)[0]))
#define MAX_NAME_LEN 255

#define MAX(A,B) (A>=B?A:B)
#define MIN(A,B) (A<=B?A:B)

#define MIN_NAME_LEN(X) (MAX_NAME_LEN-1<X?MAX_NAME_LEN-1:X)
/// typeof WindowInfo::id
typedef unsigned int WindowID;
/// typeof Master::id
typedef unsigned int MasterID;
/// typeof Slave::id
typedef MasterID SlaveID;
/// typeof Workspace::id
typedef unsigned int WorkspaceID;
/// typeof Monitor::id
typedef unsigned int MonitorID;
/// holds current time in mills
typedef unsigned long TimeStamp;
typedef uint32_t WindowMask;

typedef struct Monitor Monitor;
typedef struct Layout Layout;
typedef struct Master Master;
typedef struct Slave Slave;
typedef struct WindowInfo WindowInfo;
typedef struct Workspace Workspace;


typedef union Arg {
    int i;
    const char* str;
    void* p;
} Arg;


#define __FUNC_CAT_HELPER(x, y, z) x##y##z
#define __FUNC_CAT(x, y, z) __FUNC_CAT_HELPER(x, y, z)
#define __DECLARE_GET_X_BY_NAME(X) \
X* __FUNC_CAT(get,X,ByName)(const char* name)

#define __DEFINE_GET_X_BY_NAME(X) \
X* __FUNC_CAT(get,X,ByName)(const char* name) { \
    const ArrayList* arr =__FUNC_CAT(getAll, X, s)(); \
    for(int i = 0; i < arr->size; i++) { \
        X* p= getElement(arr, i); \
        if(strcmp(p->name, name) == 0) \
            return p; \
    } \
    return NULL; \
}

/**
 * @return a list of all workspaces
 */
const ArrayList* getAllWorkspaces();

/**
 * @return a list of windows
 */
const ArrayList* getAllWindows();

/**
 * Returns a struct with stored metadata on the given window
 * @param win
 * @return pointer to struct with info on the given window
 */
static inline WindowInfo* getWindowInfo(WindowID win) {
    return findElement(getAllWindows(), &win, sizeof(WindowID));
}

__DECLARE_GET_X_BY_NAME(Master);
__DECLARE_GET_X_BY_NAME(Monitor);
__DECLARE_GET_X_BY_NAME(Slave);
__DECLARE_GET_X_BY_NAME(Workspace);
#endif
