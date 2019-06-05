/**
 * @file spawn.h
 * Helper methods to spawn external programs
 */
#ifndef MPX_SPAWN_H
#define MPX_SPAWN_H

/**
 * Set environment vars such to help old clients know which master device to use
 */
void setClientMasterEnvVar(void);
/**
 * Forks and runs command in SHELL
 * @param command
 * @return the pid of the new process
 */
int spawn(char* command);

/**
 * Like spawn but the child's stdin refers to our stdout
 * @return the pid of the new process
 */
int spawnPipe(char* command);
/**
 * Waits for the child process given by pid to terminate
 * @param pid the child
 * @return the pid of the terminated child or -1 on error
 * @see waitpid()
 */
int waitForChild(int pid);
/**
 * Resets the status pipe
 */
void resetPipe();

#endif
