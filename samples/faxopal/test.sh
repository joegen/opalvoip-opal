#!/bin/bash

CURDIR=`pwd`

FAXOPAL=`ls $CURDIR/obj_*/faxopal | head -1`
if [ -z "$FAXOPAL" ]; then
  echo "faxopal not built."
  exit 1
fi

RESULT_DIR=$CURDIR/results
if [ ! -d $RESULT_DIR ]; then
  mkdir $RESULT_DIR || exit $?
fi

HOST=`/sbin/ifconfig eth0 2>/dev/null | grep 'inet addr' | sed -e 's/ *inet addr://' -e 's/ *Bcast.*$//'`
if [ -z "$HOST" ]; then
  HOST=`hostname`
fi

function test_fax()
{
  echo -n "Performing test $1 ... "
  $FAXOPAL $4 -ttttto$RESULT_DIR/rx_$1.log --sip udp$\*:15060 --h323 tcp$\*:11720 --timeout 2:30 $RESULT_DIR/rx_$1.tif 2>&1 > $RESULT_DIR/rx_$1.out &
  sleep 2
  $FAXOPAL $3 -ttttto$RESULT_DIR/tx_$1.log --sip udp$\*:25060 --h323 tcp$\*:21720 $CURDIR/F06_200.tif $2 2>&1 > $RESULT_DIR/tx_$1.out &
  wait
  if grep -q success $RESULT_DIR/rx_$1.out && grep -q success $RESULT_DIR/tx_$1.out; then
    echo "successful."
  else
    echo "failed."
  fi
}

test_fax h323_fast_t38_t38   "h323:$HOST:11720" ""   ""
test_fax h323_fast_g711_t38  "h323:$HOST:11720" "-a" ""
test_fax h323_fast_t38_g711  "h323:$HOST:11720" ""   "-a"
test_fax h323_fast_g711_g711 "h323:$HOST:11720" "-a" "-a"

test_fax h323_slow_t38_t38   "h323:$HOST:11720" "-F"    "-F"
test_fax h323_slow_g711_t38  "h323:$HOST:11720" "-F -a" "-F"
test_fax h323_slow_t38_g711  "h323:$HOST:11720" "-F"    "-F -a"
test_fax h323_slow_g711_g711 "h323:$HOST:11720" "-F -a" "-F -a"

test_fax sip_t38_t38   "sip:$HOST:15060" ""   ""
test_fax sip_g711_t38  "sip:$HOST:15060" "-a" ""
test_fax sip_t38_g711  "sip:$HOST:15060" ""   "-a"
test_fax sip_g711_g711 "sip:$HOST:15060" "-a" "-a"

