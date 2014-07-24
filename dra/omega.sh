# read configuration variable override; see below for their meaning and defaults
[ -r $HOME/.omega.rc ] && { source $HOME/.omega.rc; }

# The location of where to create the "session-$SESSIONID" sub-directories. These directories
# are used for some files used internally in this script (master host, port, pid; environment export for screen;
# shell script for the workers). It is also used to redirect stdout and stderr from the workers batch jobs.
OMEGA_WORKDIR_BASE=${OMEGA_WORKDIR_BASE:-$HOME/.omega}

# The command-line arguments for qsub. Those specified here are used in addition to the standard ones which:
# * set the job name to "omega-$SESSIONID" (qsub argument -N)
# * redirect stdout, stderr to the session directory (-e, -o)
# * export the current environment (-V)
# The arguments provided here should set values for the soft and hard memory and time limits. Make sure
# that the distance between soft and hard limit let the job finish processing the current batch of events and
# close the output file (a few seconds should usually be enough for that).
OMEGA_QSUB_ARGS=${OMEGA_QSUB_ARGS:--l h_vmem=2G,s_vmem=1.8G,h_rt=2:00:00,s_rt=1:55:00}

# A directory accessible to both master and workers used to place the dra_master, dra_worker, and lib/*
# binaries to be used by the master and the workers in the current session.
#
# Such copying of files to a "safe location" is useful when re-compiling code while there is an
# ongoing session in the background: Re-compiling code could otherwise lead to workers using
# different versions of the code, creating chaos.
#
# Note, however, that the configuration file(s) are *not* copied (as includes are hard to handle in that case).
# So you should *not* edit any configuration file used by the workers until all workers are done configuring.
#
# If OMEGA_COPY_FILES is left empty, files are *not* copied and the dra_worker and other binaries are used
# from the directory where this script resides.
#
# If the path is not absolute, it is interpreted relative to the current session directory.
# In case of an absolute path, you can use the variable $OMEGA_CURRENT_SESSION which will expand to the name
# of the current session.
# All parts of the path but the last must exist; the last should not exist and will be created.
OMEGA_COPY_FILES=files

# If set to a non-empty value, be more verbose about commands, etc.:
#OMEGA_VERBOSE=1

# minimum and maximum tcp port number to try as listening socket for the master:
OMEGA_MASTER_PORT_MIN=${OMEGA_MASTER_PORT_MIN:-7000}
OMEGA_MASTER_PORT_MAX=${OMEGA_MASTER_PORT_MAX:-8000}

# (end config)

THIS_SCRIPT_DIR=`readlink -f \`dirname $0\``
[ $? -gt 0 ] && { echo "Error: could not determine the directory of this script"; return 1; }

[ -d $OMEGA_WORKDIR_BASE ] || mkdir $OMEGA_WORKDIR_BASE || { echo "Could not create $OMEGA_WORKDIR_BASE"; return 1; }

# submit the given number of workers via qsub.
function omega_submit_workers() {
  [ -z "$1" ] && { echo "missing argument: how many jobs do you want to submit?"; return 1; }
  [[ "$1" =~ "^[0-9]+$" ]] || { echo "argument is not a number!"; return 1; }
  local scriptfile=$OMEGA_CURRENT_WORKDIR/worker.sh
  local host=`cat $OMEGA_CURRENT_WORKDIR/host`
  local port=`cat $OMEGA_CURRENT_WORKDIR/master.port`
  local pid=`cat $OMEGA_CURRENT_WORKDIR/master.pid`
  kill -s 0 $pid || { echo "Master not running any more. Not submitting workers."; return 1;  }
  if [ ! -r $scriptfile ]; then
    cat << "EOF" > $scriptfile
#!/bin/bash
# This script is executed on the worker hosts
echo Worker script starting on `hostname` at `date`
$OMEGA_DRA_DIR/dra_worker HH PP
echo Worker script stopping at `date`; dra_worker exit code=$?
EOF
    
    sed -i s/HH/$host/ $scriptfile
    sed -i s/PP/$port/ $scriptfile
  fi

  # submit as a task job, which should be much faster than doing qsub in a loop:
  local qsub_cmd="qsub -t 1:$1 -N omega-$OMEGA_CURRENT_SESSION -V -e $OMEGA_CURRENT_WORKDIR -o $OMEGA_CURRENT_WORKDIR $OMEGA_QSUB_ARGS $scriptfile"
  [ -n "$OMEGA_VERBOSE" ] && { echo "Worker submission qsub command: $qsub_cmd"; }
  eval $qsub_cmd
  #for i in `seq 1 $NWORKERS`; do
  #  eval $qsub_cmd || { echo "Error exeucting '$qsub_cmd'"; return 1; }
  #done
}

function omega_cancel_workers() {
  [ -n "$OMEGA_CURRENT_SESSION" ] || { echo "Error: OMEGA_CURRENT_SESSION is not set; most likely you execute this command outside of the session's screen?"; }
  echo qdel omega-$OMEGA_CURRENT_SESSION
  qdel omega-$OMEGA_CURRENT_SESSION
}

# list all current sessions, based on the "session-" sub-directories in $OMEGA_WORKDIR_BASE
function omega_list_sessions() {
  [ -n "$OMEGA_VERBOSE" ] && echo "Using OMEGA_WORKDIR_BASE=$OMEGA_WORKDIR_BASE"
  strstatus=("alive" "workdir error" "dead" "unknown, as started on another host")
  for f in $OMEGA_WORKDIR_BASE/session-*; do
     local SESSIONID=${f#*/session-}
     _omega_session_status $SESSIONID
     local stat=$strstatus[$(($? + 1))]
     echo "$SESSIONID ($stat)"
  done
}

# create and populate the directory according to OMEGA_COPY_FILES and set the
# OMEGA_DRA_DIR variable to the location of dra_worker and dra_master.
function _omega_setup_dra_dir(){
  if [ -z "${OMEGA_COPY_FILES}" ]; then
     OMEGA_DRA_DIR=$THIS_SCRIPT_DIR
     return
  elif [[ ${OMEGA_COPY_FILES} == /* ]]; then
     # an absolute path. Expand it:
     eval OMEGA_DRA_DIR=${OMEGA_COPY_FILES}
  else
     OMEGA_DRA_DIR=$OMEGA_CURRENT_WORKDIR/$OMEGA_COPY_FILES
  fi
  if [ -d $OMEGA_DRA_DIR ]; then
    echo -e "Warning: directory for binaries, '$OMEGA_DIR_DIR', already exists.\n This should be avoided, as it can lead to problems if outdated binaries in lib/ are used.";
  else
    mkdir $OMEGA_DRA_DIR || { echo "Error creating directory for OMEGA_COPY_FILES '$OMEGA_DRA_DIR'"; return 1; }
  fi
  # now copy the files there:
  mkdir $OMEGA_DRA_DIR/bin $OMEGA_DRA_DIR/lib || { echo "Error creating dirs in $OMEGA_DRA_DIR"; return 1; }
  { cp $THIS_SCRIPT_DIR/dra_{worker,master} $OMEGA_DRA_DIR/bin/ && cp $THIS_SCRIPT_DIR/../lib/* $OMEGA_DRA_DIR/lib/; } || { echo "Error copying files to $OMEGA_DRA_DIR"; return 1; }
  OMEGA_DRA_DIR=$OMEGA_DRA_DIR/bin
}

# no arguments; all info is in the variables
function _omega_start_master() {
  _omega_find_master_port || { echo "Did not find a free TCP port in the given range ($OMEGA_MASTER_PORT_MIN, $OMEGA_MASTER_PORT_MAX)"; return 1; }
  [ -n "${OMEGA_VERBOSE}" ] && echo Using master port $OMEGA_MASTER_PORT
  echo $OMEGA_MASTER_PORT > $OMEGA_CURRENT_WORKDIR/master.port
  $OMEGA_DRA_DIR/dra_master -f $OMEGA_CURRENT_WORKDIR/master.pid -p $OMEGA_MASTER_PORT $OMEGA_CURRENT_CFGFILE
}

# returns whether a screen session with this name exists
function _omega_screen_exists() {
  screen -ls | grep $1 > /dev/null
}

# tests for a screen on that session and on existence of the config dir
# returns:
# 0: screen session exists on current host
# 1: workdir does not exist or is inconsistent
# 2: screen session does not exist on current host although it was setup on the current host
# 3: unknown status, as screen was started on another host
function _omega_session_status() {
  if _omega_screen_exists omega-$1; then
      return 0
  else
      [ -r $OMEGA_WORKDIR_BASE/session-$1/host ] || { return 1; }
      local host=`cat $OMEGA_WORKDIR_BASE/session-$1/host`
      if [ "$host" = `hostname -f` ]; then
          return 2
      else
          return 3
      fi
  fi
}

function _omega_screen_control() {
   for i in `seq 1 20`; do
       sleep 0.2 || sleep 1
       _omega_screen_exists ${OMEGA_CURRENT_SESSION} && break;
   done
   _omega_screen_exists ${OMEGA_CURRENT_SESSION} || return 1;
   screen -S omega-${OMEGA_CURRENT_SESSION} -p 0 -X stuff "source $OMEGA_CURRENT_WORKDIR/screenenv
_omega_start_master $OMEGA_CURRENT_CFGFILE
"
   screen -S omega-${OMEGA_CURRENT_SESSION} -p 1 -X stuff "source $OMEGA_CURRENT_WORKDIR/screenenv
omega_submit_workers 10"
}

# usage: omega_create_session <session name>  <cfg file>
function omega_create_session() {
   [ -z "$1" ] && { echo "Error: no session name given"; return 1; }
   [ -r "$2" ] || { echo "Error: config file '$2' not readable"; return 1; }
   [ -d $OMEGA_WORKDIR_BASE/session-$1 ] && { echo -e "Error: A session directory for that session name already exists ($OMEGA_WORKDIR_BASE/session-$1).\n  Use 'omega_cleanup_session $1' to remove it."; return 1; }
   OMEGA_CURRENT_SESSION=$1
   OMEGA_CURRENT_WORKDIR=$OMEGA_WORKDIR_BASE/session-$1
   OMEGA_CURRENT_CFGFILE=$2
   mkdir $OMEGA_CURRENT_WORKDIR || { echo "Error: could not create workdir $OMEGA_CURRENT_WORKDIR"; return 1; }
   [ -n "${OMEGA_VERBOSE}" ] && { echo "Created workdir $OMEGA_CURRENT_WORKDIR"; }
   hostname -f > $OMEGA_CURRENT_WORKDIR/host
   export -p > $OMEGA_CURRENT_WORKDIR/screenenv
   # export some OMEGA variables to make the omega-functions work in the screen:
   export OMEGA_MASTER_PORT_MIN OMEGA_MASTER_PORT_MAX OMEGA_QSUB_ARGS OMEGA_WORKDIR_BASE OMEGA_CURRENT_CFGFILE OMEGA_CURRENT_SESSION OMEGA_CURRENT_WORKDIR OMEGA_DRA_DIR
   export -f >> $OMEGA_CURRENT_WORKDIR/screenenv
   _omega_setup_dra_dir || return 1;
   # start the configuring process:
   (_omega_screen_control &)
   screen -c $THIS_SCRIPT_DIR/screenrc -S omega-${OMEGA_CURRENT_SESSION}
}

function omega_cleanup_session() {
  [ -n "$1" ] || { echo "Error: specify the session you want to remove"; return 1; }
  workdir=$OMEGA_WORKDIR_BASE/session-$1
  [ -d $workdir ] || { echo "Error: Session workdir not found '$workdir'"; return 1; }
  _omega_session_status $1
  local st=$?
  if [ $st -eq 1 ] || [ $st -eq 2 ]; then
     rm -rf $workdir
     echo "Data for session '$1' removed"
  else
     if [ $st -eq 0 ]; then
         echo "Error: session '$1' still running! Re-attach to it first (screen -r omega-$SESSION) and terminate screen session"
     else
         echo -e "Error: session '$1' was started form another host (`cat $OMEGA_WORKDIR_BASE/session-$1/host`); please remove it from there\n (if you are sure the session is not running, you can 'rm -rf $workdir')"
     fi
     return 1
  fi
}

function _omega_find_master_port() {
   netstat_out=$(netstat -tnl)
   OMEGA_MASTER_PORT=-1
   for OMEGA_MASTER_PORT in `seq $OMEGA_MASTER_PORT_MIN $OMEGA_MASTER_PORT_MAX`; do
       echo $netstat_out | grep :$OMEGA_MASTER_PORT > /dev/null
       if [ $? -eq 1 ]; then
           return 0
       fi
   done
   OMEGA_MASTER_PORT=-1
   return 1
}
