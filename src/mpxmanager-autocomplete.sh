#!/bin/bash
_mpxmanagerAutocomplete ()   #  By convention, the function name
{                 #+ starts with an underscore.
    local cur
    # Pointer to current completion word.
    # By convention, it's named "cur" but this isn't strictly necessary.

    COMPREPLY=()   # Array variable storing the possible completions.
    cur=${COMP_WORDS[COMP_CWORD]}
    firstArg=${COMP_WORDS[1]}

    sendOptions="--list-options --restart --dump --dump-filter --dump-all --quit --restart --sum --log-level --auto_focus_new_window_timeout --run_as_wm --sync_focus --clone_refresh_rate --crash_on_errors --default_border_color --default_border_width --default_focus_border_color --default_unfocus_border_color --default_workspace_index --ignore_key_repeat --ignore_mask --ignore_send_event --ignore_subwindows --kill_timeout --load_saved_state --master_info_path --default_number_of_workspaces --poll_count --poll_interval --shell"
    if [[ "$firstArg" == "--set" ||"$firstArg" == "--send" || "$firstArg" == "-s" ]]; then
        COMPREPLY=( $( compgen -W "$sendOptions --" -- $cur ) )
        return 0;
    else
        COMPREPLY=( $( compgen -W "-h --help -v --version -s --set --send --recompile --name --path " -- $cur ) )
    fi
    return 0
}
complete -F _mpxmanagerAutocomplete mpxmanager
