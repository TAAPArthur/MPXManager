#!/bin/bash
_mpxmanagerAutocomplete ()   #  By convention, the function name
{                 #+ starts with an underscore.
    local cur
    # Pointer to current completion word.
    # By convention, it's named "cur" but this isn't strictly necessary.

    COMPREPLY=()   # Array variable storing the possible completions.
    cur=${COMP_WORDS[COMP_CWORD]}
    firstArg=${COMP_WORDS[1]}

    sendOptions="--dump --dump-all --dump-filter --dump-win --list-options --log-level --quit --restart --sum --auto_focus_new_window_timeout --clone_refresh_rate --crash_on_errors --default_border_color --default_border_width --default_focus_border_color --default_mode --default_mod_mask --default_number_of_hidden_workspaces --default_number_of_workspaces --default_unfocus_border_color --default_workspace_index --event_period --ignore_key_repeat --ignore_mask --ignore_send_event --ignore_subwindows --kill_timeout --ld_preload_injection --load_saved_state --master_info_path --poll_count --poll_interval --run_as_wm --run_as_wm --shell --steal_wm_selection --sync_focus --sync_focus --sync_window_masks"
    if [[ "$firstArg" == "--set" ||"$firstArg" == "--send" || "$firstArg" == "-s" ]]; then
        COMPREPLY=( $( compgen -W "$sendOptions --" -- $cur ) )
        return 0;
    else
        COMPREPLY=( $( compgen -W "-h --help -v --version -s --set --send --recompile --restart --force-restart --name --path " -- $cur ) )
    fi
    return 0
}
complete -F _mpxmanagerAutocomplete mpxmanager
