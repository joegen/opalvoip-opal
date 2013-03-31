#!/bin/sh

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

  *)
    echo "usage: $0 start | stop | status"
    exit 1
  ;;
esac

NAME=opalsrv
DIR=$HOME/.opalsrv
EXE=`ls ./obj*[^d]/$NAME`

if [ ! -x $EXE ]; then
  echo "No executable found"
  exit 1
fi

if [ ! -d $DIR ]; then
  mkdir $DIR || exit $?
fi

$EXE --pid-file $DIR/${NAME}.pid \
     --ini-file $DIR/${NAME}.ini \
     --log-file $DIR/${NAME}.log \
     $OP
