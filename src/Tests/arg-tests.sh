set -ex

./mpxmanager --set="CRASH_ON_ERRORS=1" --set="quit" &>/dev/null
./mpxmanager --set="CRASH_ON_ERRORS=2" --set="quit" &>/dev/null
./mpxmanager --set="CRASH_ON_ERRORS=2" --set="dump-options" --set="quit" 2>&1 |grep -iq "CRASH_ON_ERRORS\s*=\s*2"
./mpxmanager --set="CRASH_ON_ERRORS=1" --set="dump-options" --set="quit" 2>&1 |grep -iq "CRASH_ON_ERRORS\s*=\s*1"
# testing new syntax
./mpxmanager --set "CRASH_ON_ERRORS=2" dump-options --quit 2>&1 |grep -iq "CRASH_ON_ERRORS\s*=\s*2"


if  ./mpxmanager --fake-set="CRASH_ON_ERRORS=1" --set="quit" &>/dev/null; then exit 1;fi
if  ./mpxmanager --set="BAD_OPTION=1" --set="quit" &>/dev/null; then exit 1;fi
if  ./mpxmanager --set="BAD_OPTION" --set="quit" &>/dev/null; then exit 1;fi

./mpxmanager --mode="xmousecontrol" --set="quit" &>/dev/null
./mpxmanager --mode="mpx" --set="quit" &>/dev/null

./mpxmanager --set="log-level=3" --set="CRASH_ON_ERRORS=3" &>/dev/null &
sleep 1s
./mpxmanager --send="dump-options" 2>&1 |grep -iq "CRASH_ON_ERRORS\s*=\s*3"
./mpxmanager --send="quit" --set="quit" &>/dev/null
wait

./mpxmanager --set=log-level=3 &>/dev/null &
sleep 1s
./mpxmanager --send=restart &>/dev/null
sleep 1s
./mpxmanager --send="quit" --set="quit" &>/dev/null
wait

./mpxmanager --clear-startup-method  --enable-inter-client-communication --set=log-level=3 &>/dev/null &
sleep 1s
./mpxmanager --send="quit" --set="quit" &>/dev/null
wait

./mpxmanager --clear-startup-method  --enable-inter-client-communication --set=log-level=3 &>/dev/null &
sleep 1s
./mpxmanager --send="dump" 2>&1 | grep -iq "Dumping:"
./mpxmanager dump 2>&1 | grep -iq "Dumping:"
./mpxmanager --dump 2>&1 | grep -iq "Dumping:"
./mpxmanager --dump=0 2>&1 | grep -iq "Dumping:"
./mpxmanager --send="dump=0" 2>&1 | grep -iq "Dumping:"
./mpxmanager --send="dump=test" 2>&1 | grep -iq "Dumping:"
./mpxmanager --send="quit" --set="quit" &>/dev/null
wait
