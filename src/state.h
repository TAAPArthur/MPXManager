#ifndef STATE_H
#define STATE_H

/**
 * Marks that the state may have possibility changed
 */
void markState(void);
/**
 *
 * @return 1 iff the state has actually changed
 */
int updateState();

#endif
