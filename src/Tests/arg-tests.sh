set -e

./mpxmanager --set="CRASH_ON_ERRORS=1" --set="quit" &>/dev/null
./mpxmanager --set="CRASH_ON_ERRORS=2" --set="quit" &>/dev/null
./mpxmanager --set="CRASH_ON_ERRORS=2" --set="list-options" --set="quit" 2>&1 |grep -iq "CRASH_ON_ERRORS=2"
./mpxmanager --set="CRASH_ON_ERRORS=1" --set="list-options" --set="quit" 2>&1 |grep -iq "CRASH_ON_ERRORS=1"
if  ./mpxmanager --fake-set="CRASH_ON_ERRORS=1" --set="quit" &>/dev/null; then exit 1;fi
if  ./mpxmanager --set="BAD_OPTION=1" --set="quit" &>/dev/null; then exit 1;fi
if  ./mpxmanager --set="BAD_OPTION" --set="quit" &>/dev/null; then exit 1;fi

./mpxmanager --mode="xmousecontrol" --set="quit" &>/dev/null
./mpxmanager --mode="mpx" --set="quit" &>/dev/null
./mpxmanager --clear-startup-method --enable-inter-client-communication --set=log-level=3 &>/dev/null &
sleep 3s
./mpxmanager --send="quit" --set="quit" &>/dev/null
wait
