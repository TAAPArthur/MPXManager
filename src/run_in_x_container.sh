#!/bin/sh -e
# Basically a stripped down version of xvfb-run

export DISPLAY="${FAKE_DISPLAY_NUM:-:12}"
Xvfb "${DISPLAY}" -screen 0 640x480x24 -nolisten tcp >/dev/null 2>&1 &
pid=$!
trap 'kill -9 $pid' 0
sleep 2
"$@"
