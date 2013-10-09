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

Compiling x264 on Windows is a pain, so go get the precompiled version from

    http://www.opalvoip.org/bin/libav_DLLs.zip

and copy the contents to your C:\PTLib_Plugins directory. The plug-in itself
should compile out of the box, but the helper application will not. The
plug in DLL will also not work without the application, but it is included
in the ZIP file above, so you do not have to compile it. It changes rarely.


If you wish to use a licensed version of x264, check out:

    http://www.x264licensing.com/

to get a license, then you will need to compile it. Make sure you do so
in the opal/samples/video/H.264/x264 directory. Anywhere else, and you will
have to do interesting things with include and library paths.


Some hints on compiling x264 on Windows. I used cygwin, with the mingw32
compiler, yasm and make installed. Then used from the cygwin shell:

  ./configure --enable-shared --disable-cli --enable-win32thread \
              --host=i686-pc-mingw32 --cross-prefix=i686-pc-mingw32-

Finally, you might need to tweak the x264dll.c file as the first DllMain
argument is wrong, it should be HINSTANCE not HANDLE.


Good luck!

                                   _o0o_
