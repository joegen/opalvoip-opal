#!/bin/sh

NAME=opalsrv
DIR=$HOME/.opalsrv
PROG=`ls ./obj*[^d]/$NAME`
ARGS="--pid-file $DIR/${NAME}.pid --ini-file $DIR/${NAME}.ini --log-file $DIR/${NAME}.log"

case $1 in
  start )
    COMMAND="$PROG $ARGS --daemon"
  ;;

  stop )
    COMMAND+="$PROG $ARGS --kill"
  ;;

  status )
    COMMAND+="$PROG $ARGS --status"
  ;;

  debug )
    PROG=`ls ./obj*d/$NAME`
    COMMAND="gdb -tui --args $PROG --execute $ARGS --execute"
  ;;

  log )
    less $DIR/${NAME}.log
    exit 0
  ;;

  *)
    echo "usage: $0 start | stop | status"
    exit 1
  ;;
esac

if [ ! -x $PROG ]; then
  echo "No executable found"
  exit 1
fi

if [ ! -d $DIR ]; then
  mkdir $DIR || exit $?
fi

if [ -n "$OPALDIR" -a -z "$PTLIBPLUGINDIR" ]; then
  export PTLIBPLUGINDIR=`ls -d $OPALDIR/lib*/plugins`
fi

$COMMAND

