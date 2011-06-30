#!/bin/bash

CURDIR=`pwd`

FAXOPAL=`ls $CURDIR/obj_*/faxopal | head -1`
if [ -z "$FAXOPAL" ]; then
  echo "faxopal not built."
  exit 1
fi

RESULT_DIR=$CURDIR/results
if [ -d $RESULT_DIR ]; then
  rm -f $RESULT_DIR/*
else
  mkdir $RESULT_DIR || exit $?
fi

HOST=`/sbin/ifconfig eth0 2>/dev/null | grep 'inet addr' | sed -e 's/ *inet addr://' -e 's/ *Bcast.*$//'`
if [ -z "$HOST" ]; then
  HOST=`hostname`
fi

function test_fax()
{
  echo -n "Performing test $1 $2 -> $3 ... "

  if [ "$1" = "sip" ]; then
    DEST_URL="sip:$HOST:15060"
  else
    DEST_URL="h323:$HOST:11720"
  fi

  if [ "$2" = "g711" ]; then
    TX_ARG="-a"
  fi

  if [ "$3" = "g711" ]; then
    RX_ARG="-a"
  fi

  if [ "$4" = "slow" ]; then
    TX_ARG="-F $TX_ARG"
    RX_ARG="-F $RX_ARG"
  fi

  $FAXOPAL $RX_ARG -ttttto$RESULT_DIR/${1}_rx.log --sip udp$\*:15060 --h323 tcp$\*:11720 --timeout 2:30 $RESULT_DIR/${1}_rx.tif 2>&1 > $RESULT_DIR/${1}_rx.out &
  sleep 2
  $FAXOPAL $TX_ARG -ttttto$RESULT_DIR/${1}_tx.log --sip udp$\*:25060 --h323 tcp$\*:21720 $CURDIR/F06_200.tif $DEST_URL 2>&1 > $RESULT_DIR/${1}_tx.out &
  wait
  if grep -q success $RESULT_DIR/${1}_rx.out && grep -q success $RESULT_DIR/${1}_tx.out; then
    echo "successful."
  else
    echo "failed."
  fi
}

if [ $# -ge 3 ]; then
  test_fax $*
elif [ $# = 0 ]; then
  test_fax sip t38  t38
  test_fax sip g711 t38
  test_fax sip t38  g711
  test_fax sip g711 g711

  test_fax h323 t38  t38  fast
  test_fax h323 g711 t38  fast
  test_fax h323 t38  g711 fast
  test_fax h323 g711 g711 fast

  test_fax h323 t38  t38  slow
  test_fax h323 g711 t38  slow
  test_fax h323 t38  g711 slow
  test_fax h323 g711 g711 slow
else
  echo "usage: $0 { sip | h323 } { t38 | g711 } { t38 | g711 } [ slow | fast ]"
fi

