/*
 * window-properties.h
 *
 *  Created on: Jul 28, 2018
 *      Author: arthur
 */

#ifndef WINDOW_PROPERTIES_H_
#define WINDOW_PROPERTIES_H_

#include "mywm-util-private.h"

typedef enum{REMOVE,ADD,TOGGLE}MaskAction;

void loadWindowProperties(WindowInfo *winInfo);
void loadWindowHints(WindowInfo *winInfo);

/**
 *
 * @param winInfo
 * @return 1 iff external resize requests should be granted
 */
int isExternallyResizable(WindowInfo* winInfo);
/**
 *
 * @param winInfo
 * @return 1 iff external move requests should be granted
 */
int isExternallyMoveable(WindowInfo* winInfo);

/**
 *Removes,Adds or toggles the given mask for the given window
 * @param winInfo
 * @param mask
 * @param action an instance of MaskAction
 */
void updateState(WindowInfo*winInfo,int mask,MaskAction action);

/**
 * Adds the states give by mask to the window
 * @param winInfo
 * @param mask
 */
void addState(WindowInfo*winInfo,int mask);
/**
 * Removes the states give by mask from the window
 * @param winInfo
 * @param mask
 */
void removeState(WindowInfo*winInfo,int mask);


/**
 * Determines if a window should be tiled given its mapState and masks
 * @param winInfo
 * @return true if the window should be tiled
 */
int isTileable(WindowInfo* winInfo);
/**
 *
 * @param windowStack the stack of windows to check from
 * @return the number of windows that can be tiled
 */
int getNumberOfTileableWindows(Node*windowStack);
/**
 * Returns the max dims allowed for this window based on its mask
 * @param m the monitor the window is one
 * @param winInfo   the window
 * @param height    whether to check the height(1) or the width(0)
 * @return the max dimension or 0 if the window has not relevant mask
 */
int getMaxDimensionForWindow(Monitor*m,WindowInfo*winInfo,int height);

/**
 *
 * @param winInfo the window whose map state to query
 * @return the window map state either UNMAPPED(0) or MAPPED(1)
 */
int isMappable(WindowInfo* winInfo);
/**
 * Sets the mapped state to UNMAPPED(0) or MAPPED(1)
 * @param win the window whose state is being modified
 * @param state the new state
 * @return  true if the state actuall changed
 */
void updateMapState(int id,int state);
/**
 *
 * @param winInfo
 * @return true if the window can receive focus
 */
int isActivatable(WindowInfo* winInfo);
int isWindowVisible(WindowInfo* winInfo);
/**
 * Returns the know visibily state of the window.
 * This will can only be remolty accurate when listening for Visibilty of events
 * @param winInfo
 * @param v
 */
void setVisible(WindowInfo* winInfo,int v);
/**
 * Sets the visibility state for the given window
 * @param mask
 */
void setDefaultWindowMasks(int mask);
int getSavedWorkspaceIndex(xcb_window_t win);
//Modifes window masks that determines special propertie of window
/**
 *
 * @param info
 * @return The mask for the given window
 */
int getWindowMask(WindowInfo*info);


#endif /* WINDOW_PROPERTIES_H_ */
