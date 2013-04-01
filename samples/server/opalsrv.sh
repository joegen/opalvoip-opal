#!/bin/sh

NAME=opalsrv
DIR=$HOME/.opalsrv
EXE=`ls ./obj*[^d]/$NAME`

case $1 in
  start )
    OP="--daemon"
  ;;

  stop )
    OP="--kill"
  ;;

  status )
    OP="--status"
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

if [ ! -x $EXE ]; then
  echo "No executable found"
  exit 1
fi

if [ ! -d $DIR ]; then
  mkdir $DIR || exit $?
fi

if [ -n "$OPALDIR" -a -z "$PTLIBPLUGINDIR" ]; then
  export PTLIBPLUGINDIR=`ls -d $OPALDIR/lib*/plugins`
fi

$EXE --pid-file $DIR/${NAME}.pid \
     --ini-file $DIR/${NAME}.ini \
     --log-file $DIR/${NAME}.log \
     $OP
