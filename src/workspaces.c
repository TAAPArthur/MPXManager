#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>
#include "globals.h"
#include "mywm-structs.h"
#include "user-events.h"
#include "boundfunction.h"
#include "workspaces.h"
#include "windows.h"
#include "monitors.h"
#include "util/arraylist.h"


///list of all workspaces
ArrayList workspaces;
const ArrayList* getAllWorkspaces() {
    return &workspaces;
}

__DEFINE_GET_X_BY_NAME(Workspace)

void addWorkspaces(int num) {
    for(int i = 0; i < num; i++) {
        Workspace* workspace = malloc(sizeof(Workspace));
        *workspace = (Workspace) {.id = getAllWorkspaces()->size};
        sprintf(workspace->name, "%d", workspace->id + 1);
        addElement(&workspaces, workspace);
    }
}
void freeWorkspace(Workspace* workspace) {
    FOR_EACH_R(WindowInfo*, winInfo, getWorkspaceWindowStack(workspace)) {
        moveToWorkspace(winInfo, getNumberOfWorkspaces() - 1);
    }
    clearArray(&workspace->windows);
    clearArray(&workspace->layouts);
    free(workspace);
}
void removeWorkspaces(int num) {
    for(int i = 0; i < num && getNumberOfWorkspaces() > 1; i++)
        freeWorkspace(pop(&workspaces));
}
uint32_t getNumberOfWorkspaces() {
    return getAllWorkspaces()->size;
}

Monitor* getMonitor(Workspace* workspace) {
    return workspace->monitor;
}
void setMonitor(Workspace* workspace, Monitor* m) {
    if(m != workspace->monitor) {
        workspace->monitor = m;
        applyEventRules(MONITOR_WORKSPACE_CHANGE, workspace);
    }
};

bool isWorkspaceVisible(Workspace* workspace) {
    return getMonitor(workspace) && isMonitorActive(getMonitor(workspace));
}

ArrayList* getWorkspaceWindowStack(Workspace* workspace) {
    return &workspace->windows;
}

Workspace* getWorkspaceOfWindow(const WindowInfo* winInfo) {
    FOR_EACH(Workspace*, w, getAllWorkspaces()) {
        if(findElement(getWorkspaceWindowStack(w), winInfo, sizeof(WindowID)))
            return w;
    }
    return NULL;
}

WorkspaceID getWorkspaceIndexOfWindow(const WindowInfo* winInfo) {
    Workspace* w = getWorkspaceOfWindow(winInfo);
    return w ? w->id : NO_WORKSPACE;
}
void markActiveWorkspaceDirty() {
    getActiveWorkspace()->dirty = 1;
}

void markWorkspaceOfWindowDirty(WindowInfo* winInfo) {
    getWorkspaceOfWindow(winInfo)->dirty = 1;
}

void addLayout(Workspace* workspace, Layout* layout) {
    addElement(&workspace->layouts, layout);
}

void setLayout(Workspace* workspace, Layout* layout) { workspace->activeLayout = layout; }
Layout* getLayout(Workspace* workspace) {return workspace->activeLayout;}
void setLayoutOffset(Workspace* workspace, int offset) {
    workspace->layoutOffset = offset;
}
void cycleLayouts(Workspace* workspace, int dir) {
    if(!workspace->layouts.size)
        return;
    int pos = getNextIndex(&workspace->layouts, getLayoutOffset(workspace), dir);
    setLayout(workspace, getElement(&workspace->layouts, pos));
    setLayoutOffset(workspace, pos);
}
bool toggleLayout(Workspace* workspace, Layout* layout) {
    if(layout != getLayout(workspace)) {
        setLayout(workspace, layout);
    }
    else {
        cycleLayouts(workspace, 0);
        if(layout == getLayout(workspace))
            return 0;
    }
    return 1;
}

uint32_t getLayoutOffset(Workspace* workspace) {
    return workspace->layoutOffset;
}

bool hasWindowWithMask(Workspace* workspace, WindowMask mask) {
    FOR_EACH(WindowInfo*, winInfo, getWorkspaceWindowStack(workspace)) {
        if(hasMask(winInfo, mask))
            return 1;
    }
    return 0;
}

void swapMonitors(WorkspaceID index1, WorkspaceID index2) {
    Monitor* monitor1 = getMonitor(getWorkspace(index1));
    Monitor* monitor2 = getMonitor(getWorkspace(index2));
    if(monitor2)
        setMonitor(getWorkspace(index1), monitor2);
    setMonitor(getWorkspace(index2), monitor1);
    if(!monitor2)
        setMonitor(getWorkspace(index1), monitor2);
}
void setWorkspaceName(WorkspaceID id, const char* name) {
    strcpy(getWorkspace(id)->name, name);
}

