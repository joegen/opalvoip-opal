#!/bin/sh

set -e

if [ `whoami` != root ]; then
   echo "$0 must be run as root!"
   exit 1
fi

case `uname -sm` in
   Linux*x86_64 )
     PLATFORM=linux64
     EXT=so
     LINKEXT=so.4
   ;;
   Linux*x86 )
     PLATFORM=linux32
     EXT=so
     LINKEXT=so.4
   ;;
   Darwin*x86_64 )
     PLATFORM=osx64
     EXT=dylib
     LINKEXT=4.dylib
   ;;
   Darwin*x86 )
     PLATFORM=osx32
     EXT=dylib
     LINKEXT=4.dylib
   ;;
   * )
     echo Unknown platform `uname -sm`
     exit 2
   ;;
esac

LIBBASE=libopenh264
LIBFILE=${LIBBASE}-1.8.0-${PLATFORM}.4.$EXT
LIBDIR=${1-/usr/local/lib}

if [ \! -d $LIBDIR ]; then
   mkdir -p $LIBDIR
fi

cd $LIBDIR
rm -f ${LIBBASE}*
curl http://ciscobinary.openh264.org/$LIBFILE.bz2 | bunzip2 > $LIBFILE
ln -s $LIBFILE ${LIBBASE}.$LINKEXT
ln -s $LIBFILE ${LIBBASE}.$EXT
ls -l $LIBDIR/${LIBBASE}*

