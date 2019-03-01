#include <assert.h>

#include <xcb/xcb.h>

#include "ewmh.h"

#include "globals.h"
#include "mywm-util.h"
#include "wmfunctions.h"

WindowInfo* cloneWindow(WindowInfo* winInfo);

void updateClone(WindowInfo* winInfo, WindowInfo* dest);
void updateAllClones(WindowInfo* winInfo);
int getCloneOrigin(WindowInfo* winInfo);

void* autoUpdateClones();
void addCloneRules(void);
