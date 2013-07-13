#!/bin/sh

SRC_DIR=`dirname $0`
SRC_DIR=`cd $SRC_DIR ; pwd`

NAME=opalsrv
INFO_DIR=$HOME/.opalsrv
ARGS="--pid-file $INFO_DIR/${NAME}.pid --ini-file $INFO_DIR/${NAME}.ini --log-file $INFO_DIR/${NAME}.log"

PROG=`ls $SRC_DIR/obj*[^d]/$NAME | head -1`
if [ ! -x $PROG ]; then
  echo "No executable found"
  exit 1
fi

case $1 in
  start )
    COMMAND="$PROG $ARGS --daemon"
  ;;

  stop )
    COMMAND="$PROG $ARGS --kill"
  ;;

  status )
    COMMAND="$PROG $ARGS --status"
  ;;

  debug )
    PROG=`ls $SRC_DIR/obj*d/$NAME`
    COMMAND="gdb -tui --args $PROG --execute $ARGS --execute"
  ;;

  log )
    less $INFO_DIR/${NAME}.log
    exit 0
  ;;

  *)
    echo "usage: $0 start | stop | status"
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

