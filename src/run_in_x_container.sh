#!/bin/sh -e
# Basically a stripped down version of xvfb-run

export DISPLAY="${FAKE_DISPLAY_NUM:-:12}"
Xvfb "${DISPLAY}" >/dev/null 2>&1 &
pid=$!
trap 'kill -9 $pid' 0
sleep 2
"$@"
