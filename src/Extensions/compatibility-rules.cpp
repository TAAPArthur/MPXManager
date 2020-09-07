#include "../bindings.h"
#include "../logger.h"
#include "../monitors.h"
#include "../mywm-structs.h"
#include "../user-events.h"
#include "../windows.h"
#include "compatibility-rules.h"

void autoDetectDockPosition(WindowInfo* winInfo) {
    if(winInfo->isDock() && !winInfo->getDockProperties()) {
        const Rect& rect = winInfo->getGeometry();
        int properties[12] = {0};
        for(Monitor* monitor : getAllMonitors()) {
            const Rect& bounds = monitor->getBase();
            if(bounds.contains(rect) && !bounds.containsProper(rect)) {
                bool dockHor = rect.y == bounds.y || rect.y + rect.height == bounds.y + bounds.height;
                int type = !dockHor ?
                    rect.x == 0 ? DOCK_LEFT : DOCK_RIGHT :
                    rect.y == 0 ? DOCK_TOP : DOCK_BOTTOM;
                int thickness = rect[dockHor + 2];
                properties[type] = thickness;
                if(rect[!dockHor] != bounds[!dockHor] && rect[!dockHor + 2] != bounds[!dockHor + 2]) {
                    properties[4 + 2 * type] = rect[!dockHor];
                    properties[4 + 2 * type + 1] = rect[!dockHor] + rect[!dockHor + 2];
                }
                winInfo->setDockProperties(properties, LEN(properties));
                INFO(*winInfo);
                applyEventRules(SCREEN_CHANGE);
                return;
            }
        }
    }
}
void addAutoDetectDockPosition(bool remove) {
    getEventRules(WINDOW_MOVE).add(DEFAULT_EVENT(autoDetectDockPosition, LOWER_PRIORITY), remove);
}
