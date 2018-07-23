/**
 * TODO
 * XUtil Test (finish) //30min F
 * functions (non-x11)//30min F
 * * Workspaces //2hr F
 * X11 tests S
     * Create testing environment
     * functions (X11)
     * wmfunctions
     * mywm // as a unit not c file
 * EWMH //2hr S
 * ICCM //2hr S
     * Floating
     * Docks
     * Sticky
 * Monitors //2hr
     * Basic (finish) F
     * Virtual monitors
     * new monitor detection
 * updating window property //30min S
 * * window tagging //1h
 * Options //1hr S
     * Focus follows mouse
     * raise on focus
         * keyboard
         * mouse
         * general
     * new window focus
         * lasat active master
         * last focus master
         * None
     * Tag OnCreate vs on change

 * Debugging/Logging
 * Doc
     * Refactor
     * Comments
     * ReadMe
     * Install
 *recompile
 * cleanup
 * Optimization/Benchmarking
 ** * Scripting //2hr
 * Extra Tiling modes
     * Tmux
 * * Patch mode
 */


#include <X11/extensions/XInput2.h>
#include <X11/extensions/XI.h>
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>

#include <getopt.h>
#include <stdio.h>

#include "event-loop.h"
#include "config.h"
#include "defaults.h"
#include "mywm-util.h"

int enterEventLoop=1;
int runAsWindowManager=1;
int numberOfWorkspaces=10;

void handler(int sig) {
  void *array[32];
  size_t size;

  // get void*'s for all entries on the stack
  size = backtrace(array, LEN(array));

  // print out all the frames to stderr
  fprintf(stderr, "Error: signal %d:\n", sig);
  backtrace_symbols_fd(array, size, STDERR_FILENO);
  exit(1);
}

void parseAgrs(int argc, char * argv[]){
    int             c;
    const char    * short_opt = "pn";
    struct option   long_opt[] =
    {
      {"help",          no_argument,       NULL, 'h'},
      {"patch",          no_argument, NULL, 'p'},
      {"no-event-loop",          no_argument, NULL, 'n'},
      {NULL,            0,                 NULL, 0  }
    };

    while((c = getopt_long(argc, argv, short_opt, long_opt, NULL)) != -1){
      switch(c)
      {
         case -1:       /* no more arguments */
         case 0:        /* long options toggles */
         break;

         case 'n':
             enterEventLoop=0;
             break;
         case 'p':
             runAsWindowManager=0;
             break;
         case 'h':
             break;
      }
    }
}

int main(int argc, char * argv[]){
    signal(SIGSEGV, handler);
    signal(SIGABRT, handler);
    //signal(SIGINT, quit);

    parseAgrs(argc,argv);
    setLogLevel(LOG_LEVEL_TRACE);
    //TODO make var
    createContext(numberOfWorkspaces);
    onStartup();
    loadSettings();
    connectToXserver();
    if(enterEventLoop)
        runEventLoop(NULL);
}
