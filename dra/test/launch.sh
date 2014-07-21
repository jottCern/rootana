#!/bin/bash


DIR=`dirname $0`
DRADIR=`readlink -f $DIR/..`

${DRADIR}/dra_master $* &
MASTER_PID=$!
echo Master pid: $MASTER_PID

sleep 1

for i in `seq 0 10`; do
   ${DRADIR}/dra_worker 127.0.0.1 5473 &
   WORKERS="$WORKERS $!"
done

#echo "Waiting for all to finish ..."

for pid in $MASTER_PID $WORKERS; do
  wait $pid
  [ $? -gt 0 ] && { echo "NOTE: exit code of pid $pid was non-zero"; }
done

