Building and installing H.264
=============================

This codec is based on FFMPEG and x264.

Please follow the instructions in ../common/ReadMe.txt to make sure the
FFMPEG library is installed for your platform.


For Linux
---------

Then you need to get an x264 snapshot from:

    ftp://ftp.videolan.org/pub/videolan/x264/snapshots/

and do the usual:

    tar xf last_x264.tar.bz2
    cd x246-snaphot*
    ./configure
    make
    sudo make install

Note: make sure yasm is installed.

Then proceed back to OPAL and redo the configure as described in
../common/ReadMe.txt.


For Windows
-----------

Compiling x264 on Windows is a major pain, go get the precompiled version from

    http://www.h323plus.org/source/download/ffmpeg_x264.zip

and copy the contents to your C:\PTLib_Plugins directory. The plug-in itself
should compile out of the box, but will not work without the above.


                                   _o0o_
