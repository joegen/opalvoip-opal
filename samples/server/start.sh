#!/bin/sh

export LD_LIBRARY_PATH=/usr/local/lib
cd /opt/opalsrv/bin
ulimit -c unlimited
./opalsrv --ini-file /opt/opalsrv/etc/opalsrv.ini --console --daemon
