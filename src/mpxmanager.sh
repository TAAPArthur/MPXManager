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
#%    --send, -s                    send argument(s) to running instance
#%    -h, --help                    Print this help
#%    -v, --version                 Print script information
#%
#%Examples:
#%    ${SCRIPT_NAME}              #start the window manager
#%    ${SCRIPT_NAME} VAR=VALUE             #set VAR to VALUE
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
    $path/$fileName $@
}
setVar(){
    file="/tmp/mpxmanager-fifo"
    [ ! -e "$file" ] && mkfifo "$file"
    echo $@ >>"$file"
}
displayVersion(){
    echo "0.9.2"
}

fileName=$(echo "mpxmanager-"$(sed "s/ /-/g" -- < <(uname -sm)))
path=$HOME/.mpxmanager
optspec=":vhs-:"

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
                    setVar $@
                    ;;
                send)
                    shift
                    setVar $@
                    ;;
                recompile)
                    mkdir -p $path
                    [ -f $path/config.c ] || cp /usr/share/mpxmanager/sample-config.c $path/config.c
                    shift
                    gcc $path/config.c -o $path/$fileName -lmpxmanager "$@"
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
                *)
                    run $@
                    ;;
            esac;;

        s)
            setVar $@
            ;;
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
