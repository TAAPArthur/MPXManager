#include "../settings.h"
#include "../Extensions/compatibility-rules.h"
#include "../Extensions/containers.h"
#include "../Extensions/ewmh.h"
#include "../Extensions/extra-rules.h"
#include "../Extensions/mpx.h"
#include "../Extensions/session.h"
#include "../bindings.h"
#include "../boundfunction.h"
#include "../functions.h"
#include "../globals.h"
#include "../globals.h"
#include "../layouts.h"
#include "../settings.h"
#include "../system.h"
#include "../util/logger.h"
#include "../windows.h"
#include "../wm-rules.h"
#include "../wmfunctions.h"
#include "../xutil/window-properties.h"


void loadSettings(void) {
    addSuggestedRules();
    loadNormalSettings();
    if(getenv("ALLOW_MPX_EXT")) {
        addInsertWindowsAtPositionRule(BEFORE_FOCUSED);
        // Extensions
        addAutoDetectDockPosition();
        addAutoMPXRules();
        addEWMHRules();
        addFloatRule();
        addMoveNonTileableWindowsToWorkspaceBounds();
        addPrintStatusRule();
        addResumeContainerRules();
        addResumeCustomStateRules();
        addStickyPrimaryMonitorRule();
    }
}
