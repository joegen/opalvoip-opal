#!/bin/sh

SRC_DIR=`dirname $0`
SRC_DIR=`cd $SRC_DIR ; pwd`

NAME=opalsrv
INFO_DIR=$HOME/.opalsrv
ARGS="--pid-file $INFO_DIR/${NAME}.pid --ini-file $INFO_DIR/${NAME}.ini --log-file $INFO_DIR/${NAME}.log"

PROG=`ls $SRC_DIR/obj*[!d]/$NAME | head -1`
if [ ! -x $PROG ]; then
  echo "No executable found"
  exit 1
fi

cd $INFO_DIR

case $1 in
  start )
	cp $SRC_DIR/webrtc_test.html $INFO_DIR
    COMMAND="$PROG $ARGS --daemon"
  ;;

  stop )
    COMMAND="$PROG $ARGS --kill"
  ;;

  restart )
    COMMAND="$PROG $ARGS --kill"
    COMMAND="$PROG $ARGS --daemon"
  ;;

  status )
    COMMAND="$PROG $ARGS --status"
  ;;

  debug )
    PROG=`ls $SRC_DIR/obj*d/$NAME`
    GDB=`which gdb`
    if [ -z "$GDB" ]; then
      GDB=`which ggdb`
      if [ -z "$GDB" ]; then
        echo "Could not find GDB."
        exit 1
      fi
    fi
    COMMAND="$GDB -tui --args $PROG --execute $ARGS --execute"
  ;;

  log )
    less $INFO_DIR/${NAME}.log
    exit 0
  ;;

  tail )
    tail -f $INFO_DIR/${NAME}.log
    exit 0
  ;;

  *)
    echo "usage: $0 start | stop | status | log | tail | debug"
    exit 1
  ;;
esac

if [ ! -d $INFO_DIR ]; then
  mkdir $INFO_DIR || exit $?
fi

if [ -n "$OPALDIR" -a -z "$PTLIBPLUGINDIR" ]; then
  export PTLIBPLUGINDIR=`ls -d $OPALDIR/lib*/plugins`
fi

$COMMAND

