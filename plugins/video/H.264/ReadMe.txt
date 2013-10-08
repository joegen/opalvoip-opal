Building and installing H.264
=============================

This codec is based on FFMPEG and x264.

Please follow the instructions in ../common/ReadMe.txt to make sure the
FFMPEG library is installed for your platform.


For Linux
---------

As a rule, it should be available as a package. If not, or you think you
need then latest version, then you need to get an x264 snapshot from:

    ftp://ftp.videolan.org/pub/videolan/x264/snapshots/

and do the usual:

    tar xf last_x264.tar.bz2
    cd x246*
    ./configure
    make
    sudo make install

Note: make sure yasm is installed beforehand.

Then proceed back to OPAL and redo the configure as described in
../common/ReadMe.txt.


For Windows
-----------

Compiling x264 on Windows is a major pain, go get the precompiled version from

    http://www.opalvoip.org/bin/libav_DLLs.zip

and copy the contents to your C:\PTLib_Plugins directory. The plug-in itself
should compile out of the box, but the helper application will not. The
plug in DLL will also not work without the application.

If you wish to use a licensed version of x264, check out:

    http://www.x264licensing.com/

to get a license, then you will need to copmpile it. Make sure you do so
in the opal/samples/video/H.264/x264 directory. Anywhere else and you will
have to do things with include and library paths.

Good luck!

                                   _o0o_
