#!/usr/bin/env bash

export TCBINIT=./bin/minitcb_setup
export RUNNO=`cat RUNNUMBER`

#$TCBINIT -r $RUNNO -i 192.168.0.2
#$TCBINIT -r $RUNNO -i 192.168.0.2 -t 1 > log_${RUNNO}.txt
$TCBINIT -r $RUNNO -i 192.168.0.2 > log_${RUNNO}.txt; cat log_${RUNNO}.txt

echo "/////////////////"
echo "RUNNUMBER: $RUNNO"
echo "/////////////////"
