#!/bin/bash
#================================================================
# HEADER
#================================================================
#%Usage: ${SCRIPT_NAME} [ACTION]
#%MPX aware window manager
#%
#%
#%Action:
#%    --recompile                   Recompile with program. Needed for changes to config to be loaded
#%    --restart                     Restart the program
#%    --name                        Name of executable
#%    --path                        Path to the config file
#%    --send=<name=value>, -s       send argument(s) to running instance (can be called multiple times)
#%    --set=<name=value>            set argument(s) for new instances (can be called multiple times)
#%    --mode, -m                    run with a predefined settings. Valid options are: xmousecontrol
#%    -h, --help                    Print this help
#%    --quit                        Exists a running instance
#%    -v, --version                 Print script information
#%
#%Modes:
#%Each mode is a non-WM feature
#%      xmousecontrol enables control of the mouse with the keyboard
#%      mpx-start   loads mpx config file, creates masters and attaches slaves to the appropiate masters
#%      mpx-stop    removes all non-default masters
#%      mpx-restart mpx-stop and mpx-start
#%      mpx-resstore   Like mpx-start but does not create any new masters
#%      mpx-split   takes all slaves that are sending events and add them to a newly created master. To make the timeout longer, change the value of POLL_COUNT/POLL_INTERVAL via --set
#%
#%Examples:
#%    ${SCRIPT_NAME}              #start the window manager
#%    ${SCRIPT_NAME} --set="VAR=VALUE"             #start the window manager and set VAR to VALUE
#%    ${SCRIPT_NAME} --send="VAR=VALUE"             #set VAR to VALUE for a running instance
#%
#================================================================
#- IMPLEMENTATION
#-    version         ${SCRIPT_NAME} (taaparthur.no-ip.org) 0.9.1
#-    author          Arthur Williams
#-    license         MIT
#================================================================
# END_OF_HEADER
#================================================================
#MAN generated with help2man -No mpxmanager.1 ./mpxmanager.sh

set -e

displayHelp(){
    SCRIPT_HEADSIZE=$(head -200 ${0} |grep -n "^# END_OF_HEADER" | cut -f1 -d:)
    SCRIPT_NAME="$(basename ${0})"
    head -${SCRIPT_HEADSIZE:-99} ${0} | grep -e "^#[%+]" | sed -e "s/^#[%+-]//g" -e "s/\${SCRIPT_NAME}/${SCRIPT_NAME}/g" ;
}

run(){
    exec $path/$fileName $@
}
displayVersion(){
    echo "0.9.6"
}

fileName=$(echo "mpxmanager-"$(sed "s/ /-/g" -- < <(uname -sm)))
path=$HOME/.mpxmanager
optspec=":vhs-:"

LIBS="-lpthread -lm -lX11 -lXi -lxcb -lxcb-xinput -lxcb-xtest -lxcb-ewmh -lxcb-icccm -lxcb-randr -lX11-xcb"

getopts "$optspec" optchar || run
    case "${optchar}" in
        -)
            case "${OPTARG}" in
                force-restart)
                    pid=$(pgrep $fileName)
                    kill $pid;
                    while kill -0 $pid; do
                        sleep 1
                    done
                    shift
                    run $@
                    ;;
                restart)
                    run "--send=restart"
                    ;;
                recompile)
                    mkdir -p $path
                    [ -f $path/config.c ] || cp /usr/share/mpxmanager/sample-config.c $path/config.c
                    shift
                    gcc $path/config.c -o $path/$fileName -lmpxmanager $LIBS "$@"
                    ;;
                help)
                    displayHelp
                    ;;
                version)
                    displayVersion
                    ;;
                name)
                    echo $fileName
                    ;;
                path)
                    echo $path
                    ;;
                quit)
                    run "--send=quit"
                    ;;
                *)
                    run $@
                    ;;
            esac;;
        v)
            displayVersion
            ;;
        h)
            displayHelp
            ;;
        *)
            run $@
            ;;
    esac
