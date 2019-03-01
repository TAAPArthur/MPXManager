#ifndef STATE_H
#define STATE_H

/**
 * Marks that the state may have possibility changed
 */
void markState(void);
/**
 * Unmarks that the state may have possibility changed
 */
void unmarkState(void);
/**
 * Calls onChange (if not NULL) for every (currently visible) workspace changed since
 * this function was last called. A workspace if considered to have changed if
 * <ul>
 * <li> the bounding box of the active monitor has changed </li>
 * <li> the number of interactable windows has changed </li>
 * <li> the stacking order of the interactable windows has changed </li>
 * <li> the user mask for any interactable window has changed </li>
 * </ul>
 * @param onChange the callback function to be trigger for every workspace
 * that has changed
 * @return 1 iff the state has actually changed
 */
int updateState(void(*onChange)(int));

#endif
