
void addAutoMPXRules(void);
/**
 * Attaches all slaves to their propper masters
 */
void restoreMPX(void);
/**
 * Creates master devices according to saved masterInfo.
 * Note that a master device won't be created if one with the same name already exists
 * loadMasterInfo should be called prior. 
 */
void startMPX(void);
/**
 * Removes all masters devices except the default
 */
void stopMPX(void);
/**
 * Saves the current master/slave configurations for later use
 */
int saveMasterInfo(void);
/**
 * loads saved master/slave configuration
 */
int loadMasterInfo(void);
/**
 * Creates a new master and moves all active slaves (slaves that are firing events) to the newly
 * created master
 * This method adds a Idle event that will serve as a callback to stop moving active slavse
 * @return the id of the newly created master
 */
int splitMaster(void);
/**
 * Removes all the event rules created by splitMaster()
 */
void endSplitMaster(void);
