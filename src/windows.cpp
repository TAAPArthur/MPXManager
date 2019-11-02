#include <assert.h>

#include "boundfunction.h"
#include "ext.h"
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
    stream << " Title '" << winInfo.getTitle() << "'" ;
    stream << " Class '" << winInfo.getClassName() << "' '" << winInfo.getInstanceName() << "'";
    stream << " Type '" << winInfo.getTypeName() << "'";
    stream << " Masks: " << winInfo.getMask();
    if(winInfo.getWorkspace())
        stream << " Workspace:" << winInfo.getWorkspaceIndex() ;
    if(winInfo.isNotManageable())
        stream << " Not Manageable";
    if(winInfo.isOverrideRedirectWindow())
        stream << " OverrideRedirect";
    if(winInfo.isInputOnly())
        stream << " InputOnly";
    if(winInfo.isDock())
        stream << " Dock";
    if(winInfo.getTilingOverrideMask())
        stream << " Tiling override: " << winInfo.getTilingOverrideMask() << winInfo.getTilingOverride();
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
        destIndex = getActiveMaster()->getWorkspaceIndex();
    }
    removeFromWorkspace();
    ::getWorkspace(destIndex)->getWindowStack().add(this);
    applyEventRules(WindowWorkspaceMove, this);
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
void WindowInfo::setDockProperties(int* properties, int numberofProperties) {
    memset(getDockProperties(1), 0, 12 * sizeof(int));
    memcpy(getDockProperties(1), properties, numberofProperties * sizeof(int));
}
int* WindowInfo::getDockProperties(bool createNew) {
    static Index<int[12]> key;
    return *get(key, this, createNew);
}
