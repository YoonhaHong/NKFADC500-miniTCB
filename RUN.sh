#!/usr/bin/env bash

export TCBSTART=./bin/minitcb_start 
export FADCEXE=./bin/nkfadc500_run
export RUNNO=`cat RUNNUMBER`
export NEVT=${1:-0}

$TCBSTART #192.168.0.2
$FADCEXE $RUNNO $NEVT

echo "///////////////////"
echo "RUNNUMBER: ${RUNNO}"
echo "///////////////////"

if [ $NEVT -ne 0 ]; then
	source ./STOP.sh
fi
