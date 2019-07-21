set -e
i=100
find_free_servernum() {
    while [ -f /tmp/.X$i-lock ]; do
        i=$(($i + 1))
    done
    echo $i
}

export DISPLAY=:$(find_free_servernum) 
Xvnc $DISPLAY &>/dev/null &
pid1=$!
trap "kill $pid1" EXIT
sleep 3s
$@
retCode=$?
exit $retCode
