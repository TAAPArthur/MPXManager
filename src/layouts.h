/**
 * @file layouts.h
 *
 * @brief Layout functions
 */

#ifndef LAYOUTS_H_
#define LAYOUTS_H_


#define CONFIG_LEN 6

enum {FULL,GRID,LAST_LAYOUT};

extern Layout DEFAULT_LAYOUTS[];
extern int NUMBER_OF_DEFAULT_LAYOUTS;

#define LAYOUT_LIMIT_INDEX  0
#define LAYOUT_BORDER_INDEX 1

/**
 *
 * @param windowStack the stack of windows to check from
 * @return the number of windows that can be tiled
 */
int getNumberOfWindowsToTile(Node*windowStack,int limit);


void full(Monitor*m,Node*windowStack,int*args);
void grid(Monitor*m,Node*windowStack,int*args);
void masterPane(Monitor*m,Node*windowStack,int*args);

#endif /* LAYOUTS_H_ */
