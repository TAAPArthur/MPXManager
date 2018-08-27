#include <assert.h>

#include <xcb/xcb.h>

#include "ewmh.h"

#include "globals.h"
#include "mywm-util.h"
#include "wmfunctions.h"

xcb_gcontext_t create_graphics_context();
WindowInfo* createCloneWindow(WindowInfo*winInfo);

void updateClone(WindowInfo*winInfo,WindowInfo* dest);
void updateAllClones(WindowInfo*winInfo);
int getCloneOrigin(WindowInfo*winInfo);

void addCloneRules();
