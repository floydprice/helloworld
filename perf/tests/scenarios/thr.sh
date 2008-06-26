#!/bin/sh

if [ $# -ne 2 ]; then
    echo "Usage: thr.sh [tcp | zmq] [local | remote]"
    exit 1
fi

if [ $2 != "local" -a $2 != "remote" ]; then
    echo "Usage: thr.sh [tcp | zmq ] [local | remote]"
    exit 1
fi

if [ $1 != "zmq" -a $1 != "tcp" ]; then
    echo "Usage: thr.sh [tcp | zmq ] [local | remote]"
    exit 1
fi

##############################################################################

GL_IP="62.176.172.203"
GL_PORT=5555

REC_IP="10.0.0.1"
REC_PORT=5672

MSG_SIZE_START=1
MSG_SIZE_STEPS=10

THREADS=1
RUNS=3

TEST_TIME=5000

LOCAL_THR_BIN="./local_thr"
REMOTE_THR_BIN="./remote_thr"

##############################################################################

SYS_LAT_DEN=100
SYS_SLOPE=0.55
SYS_OFF=110
SYS_BREAK=1024

SYS_SLOPE_BIG=0.89
SYS_OFF_BIG=0

if [ $2 = "local" ]; then
    echo "running local (receiver)"    
    while [ $RUNS -gt 0 ]; do
        for i in `seq 0 $MSG_SIZE_STEPS`;
        do
            let MSG_SIZE=2**$i

            if [ $MSG_SIZE -lt $SYS_BREAK ]; then
                MSG_COUNT=`echo "($TEST_TIME * 100000 ) / ($SYS_SLOPE * $MSG_SIZE + $SYS_OFF)" | bc`
            else
                MSG_COUNT=`echo "($TEST_TIME * 100000 ) / ($SYS_SLOPE_BIG * $MSG_SIZE + $SYS_OFF_BIG)" | bc`
            fi

            if [ $1 = "zmq" ]; then
                $LOCAL_THR_BIN $GL_IP $GL_PORT $REC_IP $REC_PORT $MSG_SIZE $MSG_COUNT $THREADS
            else
                echo $LOCAL_THR_BIN $REC_IP $REC_PORT $MSG_SIZE $MSG_COUNT $THREADS
                $LOCAL_THR_BIN $REC_IP $REC_PORT $MSG_SIZE $MSG_COUNT $THREADS
                let REC_PORT=REC_PORT+THREADS
            fi
        done
        let RUNS=RUNS-1 
    done
else
    echo "running remote (sender)"
    while [ $RUNS -gt 0 ]; do
        for i in `seq 0 $MSG_SIZE_STEPS`;
        do
            let MSG_SIZE=2**$i

            if [ $MSG_SIZE -lt $SYS_BREAK ]; then
                MSG_COUNT=`echo "($TEST_TIME * 100000 ) / ($SYS_SLOPE * $MSG_SIZE + $SYS_OFF)" | bc`
            else
                MSG_COUNT=`echo "($TEST_TIME * 100000 ) / ($SYS_SLOPE_BIG * $MSG_SIZE + $SYS_OFF_BIG)" | bc`
            fi

            if [ $1 = "zmq" ]; then
                $REMOTE_THR_BIN $GL_IP $GL_PORT $MSG_SIZE $MSG_COUNT $THREADS
            else
                echo $REMOTE_THR_BIN $REC_IP $REC_PORT $MSG_SIZE $MSG_COUNT $THREADS
                $REMOTE_THR_BIN $REC_IP $REC_PORT $MSG_SIZE $MSG_COUNT $THREADS                
                let REC_PORT=REC_PORT+THREADS
            fi
            sleep 1
        done
        let RUNS=RUNS-1 
    done
    echo
fi
