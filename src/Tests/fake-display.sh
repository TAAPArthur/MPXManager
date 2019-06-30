i=100
find_free_servernum() {

    while [ -f /tmp/.X$i-lock ]; do
        i=$(($i + 1))
    done
    echo $i
}


workingDisplay1=$(find_free_servernum) 
Xvnc :$workingDisplay1 &>/dev/null &
pid1=$!
trap "pkill $pid1" EXIT
export DISPLAY=:$workingDisplay1
$@
retCode=$?
exit $retCode
