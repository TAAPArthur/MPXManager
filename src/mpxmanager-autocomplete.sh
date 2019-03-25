#!/bin/bash
_mpxmanagerAutocomplete ()   #  By convention, the function name
{                 #+ starts with an underscore.
    local cur
    # Pointer to current completion word.
    # By convention, it's named "cur" but this isn't strictly necessary.

    COMPREPLY=()   # Array variable storing the possible completions.
    cur=${COMP_WORDS[COMP_CWORD]}
    firstArg=${COMP_WORDS[1]}

    sendOptions="--list --restart --dump --dump-all --list --quit --restart --sum --AUTO_FOCUS_NEW_WINDOW_TIMEOUT --CLONE_REFRESH_RATE --CRASH_ON_ERRORS --DEFAULT_BORDER_COLOR --DEFAULT_BORDER_WIDTH --DEFAULT_FOCUS_BORDER_COLOR --DEFAULT_UNFOCUS_BORDER_COLOR --DEFAULT_WORKSPACE_INDEX --IGNORE_KEY_REPEAT --IGNORE_MASK --IGNORE_SEND_EVENT --IGNORE_SUBWINDOWS --KILL_TIMEOUT --LOAD_SAVED_STATE --LOG_LEVEL --MASTER_INFO_PATH --NUMBER_OF_WORKSPACES --POLL_COUNT --POLL_INTERVAL --SHELL"
    if [[ "$firstArg" == "--send" || "$firstArg" == "-s" ]]; then
        COMPREPLY=( $( compgen -W "$sendOptions --" -- $cur ) )
        return 0;
    else
        COMPREPLY=( $( compgen -W "-h --help -v --version -s --send --recompile --name --path $sendOptions --" -- $cur ) )
    fi
    return 0
}
complete -F _mpxmanagerAutocomplete mpxmanager
