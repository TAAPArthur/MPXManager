/**
 * @file layouts.h
 *
 * @brief Layout functions
 */

#ifndef LAYOUTS_H_
#define LAYOUTS_H_
#include "mywm-structs.h"


enum {FULL,GRID,LAST_LAYOUT};

extern Layout DEFAULT_LAYOUTS[];
extern int NUMBER_OF_DEFAULT_LAYOUTS;

void full(Monitor*m,Node*windowStack,int*args);
void grid(Monitor*m,Node*windowStack,int*args);
void masterPane(Monitor*m,Node*windowStack,int*args);

#endif /* LAYOUTS_H_ */
