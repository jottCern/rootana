#!/bin/bash

DRA=../dra/dra_local
OUT_BASE=/nfs/dust/cms/user/ottjoc/omega-out/ntuple-latest/


function do_exec(){
    echo -n $* ...
    [ -f $OUT_PATH/done ] && { echo "already completed"; return; }
    [ -d $OUT_PATH ] || mkdir -p $OUT_PATH;
    echo "running now:"
    eval $*
    [ $? -gt 0 ] && { echo "Error executing $*"; exit 1; }
    touch $OUT_PATH/done
}

# flavor splitting:
for fl in bbvis bbother cc other; do
  OUT_PATH=${OUT_BASE}/z${fl}
  do_exec $DRA cfg/split_dy-${fl}.cfg
done

# ttbar / ttbar + bbbar splitting:
OUT_PATH=${OUT_BASE}/ttbar2b
do_exec $DRA cfg/split_ttbar-2b.cfg

OUT_PATH=${OUT_BASE}/ttbar4b
do_exec $DRA cfg/split_ttbar-4b.cfg

# make main reco plots:
OUT_PATH=${OUT_BASE}/recoplots
do_exec $DRA cfg/recoplots.cfg

# split me:
#OUT_PATH=${OUT_BASE}/me
#do_exec $DRA cfg/select_me.cfg

# plot me:
#OUT_PATH=${OUT_BASE}/plot_me
#do_exec $DRA cfg/plot_me.cfg
