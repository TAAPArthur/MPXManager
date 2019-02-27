/**
 * @file args.h
 * Contains methods to parse command line options
 * The same options can be triggered by writting to our named pipe
 */
#ifndef ARGS_H_
#define ARGS_H_

/**
 * Lists all options with their current value
 */
void listOptions(void);
/**
 * Parse command line arguments starting the the (i=1)th index
 * @param argc the number of args to parse
 * @param argv the list of args. An element by be like "-s", "--long", --long=V", etc
 * @param exitOnUnknownOptions if true will exit(1) if encounter an unkown option
 */
void parseArgs(int argc, char* argv[], int exitOnUnknownOptions);
/**
 * Continually read and parse user arguments to set global vars
 */
void startReadingUserInput(void);
#endif
