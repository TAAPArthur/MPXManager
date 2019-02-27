set -e

./mpxmanager --CRASH_ON_ERRORS=1 --quit &>/dev/null
./mpxmanager --CRASH_ON_ERRORS=2 --quit &>/dev/null
./mpxmanager --CRASH_ON_ERRORS=2 --dump-options --quit 2>&1 |grep -iq "CRASH_ON_ERRORS=2"
./mpxmanager --CRASH_ON_ERRORS=1 --dump-options --quit 2>&1 |grep -iq "CRASH_ON_ERRORS=1"
./mpxmanager -c2 --quit &>/dev/null
if  ./mpxmanager --BAD_OPTION=1 --quit &>/dev/null; then exit 1;fi 
if  ./mpxmanager --BAD_OPTION --quit &>/dev/null; then exit 1;fi 

