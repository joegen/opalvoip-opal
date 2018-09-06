#!/bin/sh

export LD_LIBRARY_PATH=/usr/local/lib
/opt/opalsrv/bin/opalsrv --ini-file /opt/opalsrv/etc/opalsrv.ini --log-file /var/log/opalsrv/opalsrv.log --daemon
