#include <assert.h>
#include <string.h>

#include "boundfunction.h"
#include "ext.h"
#include "logger.h"
#include "masters.h"
#include "user-events.h"
#include "windows.h"
#include "workspaces.h"


///list of all windows
static ArrayList<WindowInfo*> windows;
ArrayList<WindowInfo*>& getAllWindows(void) {
    return windows;
}

std::ostream& operator<<(std::ostream& stream, const WindowInfo& winInfo) {
    stream << "{ WindowInfo: ID " << winInfo.getID() << (winInfo.isTileable() ? "*" : "") <<
        (!winInfo.isMappable() ? "?" : "");
    stream << " Parent: " << winInfo.getParent();
    if(winInfo.getID() != winInfo.getEffectiveID())
        stream << " EffectiveID: " << winInfo.getEffectiveID();
    if(winInfo.getGroup())
        stream << " Group: " << winInfo.getGroup();
    if(winInfo.getTransientFor())
        stream << " Transient For: " << winInfo.getTransientFor();
    if(winInfo.getType()) {
        stream << " Title '" << winInfo.getTitle() << "'" ;
        stream << " Class '" << winInfo.getClassName() << "' '" << winInfo.getInstanceName() << "'";
        stream << " Type " << (winInfo.isImplicitType() ? "?" : "") << "'" << winInfo.getTypeName() << "'";
        stream << " Role '" << winInfo.getRole() << "'";
    }
    stream << " Masks: " << winInfo.getMask();
    if(winInfo.getWorkspace())
        stream << " Workspace:" << winInfo.getWorkspaceIndex() ;
    if(winInfo.isNotManageable())
        stream << " NotManageable";
    if(winInfo.isInputOnly())
        stream << " InputOnly";
    if(winInfo.isDock())
        stream << " Dock";
    if(winInfo.isOverrideRedirectWindow())
        stream << " OverrideRedirect";
    else if(winInfo.getTilingOverrideMask())
        stream << " TilingOverride: (" << (int)winInfo.getTilingOverrideMask() << ")" << winInfo.getTilingOverride();
    stream << " Geometry " << winInfo.getGeometry();
    return stream << "}";
}

bool WindowInfo::isNotInInvisibleWorkspace()const {
    if(getWorkspaceIndex() == NO_WORKSPACE)
        return 1;
    return getWorkspace()->isVisible();
}
WindowMask WindowInfo::hasPartOfMask(WindowMask mask)const {
    Workspace* w = getWorkspace();
    WindowMask winMask = getMask();
    if(w && !((winMask | mask) & IGNORE_WORKSPACE_MASKS_MASK))
        winMask |= w->getMask();
    return (winMask & mask);
}

void WindowInfo::removeFromWorkspace() {
    if(getWorkspace())
        getWorkspace()->getWindowStack().removeElement(this);
}

void WindowInfo::moveToWorkspace(WorkspaceID destIndex) {
    assert(destIndex < getNumberOfWorkspaces() || destIndex == NO_WORKSPACE);
    if(destIndex == NO_WORKSPACE) {
        addMask(STICKY_MASK);
        if(!getWorkspace())
            return;
        destIndex = getActiveMaster()->getWorkspaceIndex();
    }
    if(destIndex != getWorkspaceIndex()) {
        LOG(LOG_LEVEL_DEBUG, "Moving %d to workspace %d from %d", getID(), destIndex, getWorkspaceIndex());
        removeFromWorkspace();
        ::getWorkspace(destIndex)->getWindowStack().add(this);
        applyEventRules(WINDOW_WORKSPACE_CHANGE, {.winInfo = this, .workspace = getWorkspace()});
    }
}
void WindowInfo::swapWorkspaceWith(WindowInfo* winInfo) {
    Workspace* w1 = getWorkspace();
    Workspace* w2 = winInfo->getWorkspace();
    int index1 = w1 ? w1->getWindowStack().indexOf(this) : -1;
    int index2 = w2 ? w2->getWindowStack().indexOf(winInfo) : -1;
    if(index1 != -1)
        w1->getWindowStack()[index1] = winInfo;
    if(index2 != -1)
        w2->getWindowStack()[index2] = this;
    applyEventRules(WINDOW_WORKSPACE_CHANGE, {.winInfo = winInfo, .workspace = w1});
    applyEventRules(WINDOW_WORKSPACE_CHANGE, {.winInfo = this, .workspace = getWorkspace()});
}

WorkspaceID WindowInfo::getWorkspaceIndex()const {
    Workspace* w = getWorkspace();
    return w ? w->getID() : NO_WORKSPACE;
}
Workspace* WindowInfo::getWorkspace(void)const {
    for(Workspace* w : getAllWorkspaces())
        if(w->getWindowStack().find(getID()))
            return w;
    return NULL;
}

WindowInfo::~WindowInfo() {
    for(Master* master : getAllMasters())
        master->removeWindowFromFocusStack(getID());
    removeFromWorkspace();
    removeID(this);
}
void WindowInfo::setDockProperties(int* properties, int numberOfProperties) {
    memset(getDockProperties(1), 0, 12 * sizeof(int));
    memcpy(getDockProperties(1), properties, numberOfProperties * sizeof(int));
}
int* WindowInfo::getDockProperties(bool createNew) {
    static Index<int[12]> key;
    return *get(key, this, createNew);
}
