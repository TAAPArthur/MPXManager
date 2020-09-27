#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "boundfunction.h"
#include "globals.h"
#include "util/logger.h"
#include "masters.h"
#include "user-events.h"
#include "windows.h"
#include "workspaces.h"


///list of all windows
static ArrayList windows;
const ArrayList* getAllWindows(void) {
    return &windows;
}

WindowInfo* newWindowInfo(WindowID id, WindowID parent) {
    WindowInfo* winInfo = malloc(sizeof(WindowInfo));
    WindowInfo temp = {.id = id, .parent = parent};
    memmove(winInfo, &temp, sizeof(WindowInfo));
    addElement(&windows, winInfo);
    return winInfo;
}

void freeWindowInfo(WindowInfo* winInfo) {
    FOR_EACH(Master*, master, getAllMasters()) {
        removeWindowFromFocusStack(master, winInfo->id);
    }
    removeFromWorkspace(winInfo);
    removeElement(&windows, winInfo, sizeof(WindowID));
    free(winInfo);
}

bool isNotInInvisibleWorkspace(WindowInfo* winInfo) {
    if(getWorkspaceIndexOfWindow(winInfo) == NO_WORKSPACE)
        return 1;
    return isWorkspaceVisible(getWorkspaceOfWindow(winInfo));
}

void removeFromWorkspace(WindowInfo* winInfo) {
    Workspace* w = getWorkspaceOfWindow(winInfo);
    if(w) {
        applyEventRules(WORKSPACE_WINDOW_REMOVE, winInfo);
        removeIndex(getWorkspaceWindowStack(w), getIndex(getWorkspaceWindowStack(w), &winInfo->id, sizeof(WindowID)));
    }
}

void moveToWorkspace(WindowInfo* winInfo, WorkspaceID destIndex) {
    assert(destIndex < getNumberOfWorkspaces() || destIndex == NO_WORKSPACE);
    if(destIndex == NO_WORKSPACE) {
        addMask(winInfo, STICKY_MASK);
        if(!getWorkspaceOfWindow(winInfo))
            return;
        destIndex = getActiveWorkspaceIndex();
    }
    if(destIndex != getWorkspaceIndexOfWindow(winInfo)) {
        DEBUG("Moving %d to workspace %d from %d", winInfo->id, destIndex, getWorkspaceIndexOfWindow(winInfo));
        removeFromWorkspace(winInfo);
        addElement(&getWorkspace(destIndex)->windows, winInfo);
        applyEventRules(WORKSPACE_WINDOW_ADD, winInfo);
    }
}

const DockProperties* getDockProperties(WindowInfo* winInfo) {
    return winInfo->dockProperties.thickness ? &winInfo->dockProperties : NULL;
}
void setDockProperties(WindowInfo* winInfo, int* properties, bool partial) {
    if(properties) {
        for(int i = 0; i < 4; i++) {
            if(properties[i]) {
                winInfo->dockProperties = (DockProperties) {(DockType)i, (uint16_t)properties[i], 0, 0};
                if(partial) {
                    winInfo->dockProperties.start = properties[4 + i * 2];
                    winInfo->dockProperties.end = properties[4 + i * 2 + 1];
                }
                return;
            }
        }
    }
    winInfo->dockProperties.thickness = 0;
}

WindowMask getEffectiveMask(const WindowInfo* winInfo) {
    Workspace* workspace = getWorkspaceOfWindow(winInfo);
    WindowMask mask = winInfo->mask;
    if(workspace) {
        mask |= workspace->mask;
    }
    return mask;
}

WindowMask getMasksToSync(WindowInfo* winInfo) {
    return hasMask(winInfo, SYNC_ALL_MASKS) ? (WindowMask) ~EXTERNAL_MASKS : MASKS_TO_SYNC;
}
void saveAllWindowMasks() {
    FOR_EACH(WindowInfo*, winInfo, getAllWindows()) {
        winInfo->savedMask = winInfo->mask;
    }
}
