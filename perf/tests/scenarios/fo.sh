#!/bin/sh

if [ $# -lt 2 ]; then
    echo "Usage: fo.sh [local number_of_subscribers | remote subs_id]"
    exit 1
fi

if [ $1 != "local" -a $1 != "remote" ]; then
    echo "Usage: fo.sh [local number_of_subscribers |remote subs_id]"
    exit 1
fi

##############################################################################

GL_IP="62.176.172.203"
GL_PORT=5555

REC_IP="10.0.0.2"
REC_PORT=5672

MSG_SIZE_START=1
MSG_SIZE_STEPS=10

RUNS=5

TEST_TIME=5000

LOCAL_BIN="./local_fo"
REMOTE_BIN="./remote_fo"

##############################################################################

SYS_LAT_DEN=100
SYS_SLOPE=0.55
SYS_OFF=110
SYS_BREAK=1024

SYS_SLOPE_BIG=0.89
SYS_OFF_BIG=0

if [ $1 = "local" ]; then
    echo "running local (sender)"    
    while [ $RUNS -gt 0 ]; do
        for i in `seq 0 $MSG_SIZE_STEPS`;
        do
            let MSG_SIZE=2**$i

            if [ $MSG_SIZE -lt $SYS_BREAK ]; then
                MSG_COUNT=`echo "($TEST_TIME * 100000 ) / ($SYS_SLOPE * $MSG_SIZE + $SYS_OFF)" | bc`
            else
                MSG_COUNT=`echo "($TEST_TIME * 100000 ) / ($SYS_SLOPE_BIG * $MSG_SIZE + $SYS_OFF_BIG)" | bc`
            fi

            $LOCAL_BIN $GL_IP $GL_PORT $REC_IP $REC_PORT $MSG_SIZE $MSG_COUNT $2
        done
        let RUNS=RUNS-1 
    done
else
    echo "running remote (receiver)"
    while [ $RUNS -gt 0 ]; do
        for i in `seq 0 $MSG_SIZE_STEPS`;
        do
            let MSG_SIZE=2**$i

            if [ $MSG_SIZE -lt $SYS_BREAK ]; then
                MSG_COUNT=`echo "($TEST_TIME * 100000 ) / ($SYS_SLOPE * $MSG_SIZE + $SYS_OFF)" | bc`
            else
                MSG_COUNT=`echo "($TEST_TIME * 100000 ) / ($SYS_SLOPE_BIG * $MSG_SIZE + $SYS_OFF_BIG)" | bc`
            fi

            $REMOTE_BIN $GL_IP $GL_PORT $MSG_SIZE $MSG_COUNT $2
            sleep 1
        done
        let RUNS=RUNS-1 
    done
    echo
fi
