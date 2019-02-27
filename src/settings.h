
/**
 * @file config.h
 * @brief user config file
 */


#ifndef SETTINGS_H_
#define SETTINGS_H_

/**
 * Load default settings
 */
void loadNormalSettings(void);

/**
 * Prints names of workspaces that are visible or have windows.
 * The layout of each workspace is also printed
 * The title of the active window is printed and the active workspace is marked
 */
void defaultPrintFunction(void);

#endif
