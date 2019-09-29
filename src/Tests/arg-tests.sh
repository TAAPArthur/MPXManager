set -e

./mpxmanager-debug --set="CRASH_ON_ERRORS=1" --set="quit" &>/dev/null
./mpxmanager-debug --set="CRASH_ON_ERRORS=2" --set="quit" &>/dev/null
./mpxmanager-debug --set="CRASH_ON_ERRORS=2" --set="dump-options" --set="quit" 2>&1 |grep -iq "CRASH_ON_ERRORS\s*=\s*2"
./mpxmanager-debug --set="CRASH_ON_ERRORS=1" --set="dump-options" --set="quit" 2>&1 |grep -iq "CRASH_ON_ERRORS\s*=\s*1"
# testing new syntax
./mpxmanager-debug --set "CRASH_ON_ERRORS=2" dump-options --quit 2>&1 |grep -iq "CRASH_ON_ERRORS\s*=\s*2"


if  ./mpxmanager-debug --fake-set="CRASH_ON_ERRORS=1" --set="quit" &>/dev/null; then exit 1;fi
if  ./mpxmanager-debug --set="BAD_OPTION=1" --set="quit" &>/dev/null; then exit 1;fi
if  ./mpxmanager-debug --set="BAD_OPTION" --set="quit" &>/dev/null; then exit 1;fi

./mpxmanager-debug --mode="xmousecontrol" --set="quit" &>/dev/null
./mpxmanager-debug --mode="mpx" --set="quit" &>/dev/null

./mpxmanager-debug --set="log-level=3" --set="CRASH_ON_ERRORS=3" &>/dev/null &
sleep 1s
./mpxmanager-debug --send="dump-options" 2>&1 |grep -iq "CRASH_ON_ERRORS\s*=\s*3"
./mpxmanager-debug --send="quit" --set="quit" &>/dev/null
wait

./mpxmanager-debug --set=log-level=3 &>/dev/null &
sleep 1s
./mpxmanager-debug --send=restart
sleep 1s
./mpxmanager-debug --send="quit" --set="quit" &>/dev/null
wait

./mpxmanager-debug --clear-startup-method  --enable-inter-client-communication --set=log-level=3 &>/dev/null &
sleep 1s
./mpxmanager-debug --send="quit" --set="quit" &>/dev/null
wait

./mpxmanager-debug --clear-startup-method  --enable-inter-client-communication --set=log-level=3 &>/dev/null &
sleep 1s
./mpxmanager-debug --send="dump" 2>&1 | grep -iq "Dumping:"
./mpxmanager-debug --send="dump=0" 2>&1 | grep -iq "Dumping:"
./mpxmanager-debug --send="dump=test" 2>&1 | grep -iq "Dumping:"
./mpxmanager-debug --send="quit" --set="quit" &>/dev/null
wait
