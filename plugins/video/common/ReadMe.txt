Building and installing FFmpeg on Linux
=======================================

If you have a package for you favourite distribution, use that, the configure
script should find it. If not, and this is likely, as you need the development
version, not just the ffmpeg binary program, which i sthe most common, then
you need to get the tar ball and compile.


The following is known to work, download the following:

    http://www.ffmpeg.org/releases/ffmpeg-0.6.1.tar.bz2

and do the usual:

    tar xf ffmpeg-0.6.1.tar.bz2
    cd ffmpeg-0.6.1
    ./configure --enable-shared
    make
    sudo make install

As the above installs by default into /usr/local, it is advisable to add:

  export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig

into your .profile.


Go to the OPAL directory and re-execute ./configure and make.



Building and installing FFmpeg on Windows
=========================================

Most importantly, don't try to compile FFMpeg for Windows unless you enjoy
pain.

Seriously. Many have tried and failed.

The best way to use FFMpeg on Windows is to use a precompiled shared library
version from the Unoffical FFmpeg Win32 Build site:

    http://ffmpeg.arrozcru.org/autobuilds/

As a rule use the latest snapshot, but there are never any guarantees. All
I can say is at time of writing the following worked:

    ffmpeg-r26400-swscale-r32676-mingw32-shared.7z
    ffmpeg-r26400-swscale-r32676-mingw32-shared-dev.7z

The first archive contains the DLL's, copy th e following:

    avcodec-*.dll
    avutil-*.dll
    avcore-*.dll

to the C:\PTLib_Plugins directory. Or whereever you have installed the
OPAL plug-ins.

The second archive is unpacked into:

        ...\opal\plugins\video\common\ffmpeg

You can then compile the H.263-1998 and MPEG4 projects in the OPAL plugins
solution. The H.264 takes a little more effort, see it's ReadMe.txt file.


If you really need to make patches to the code, so need to build from source,
start at http://ffmpeg.arrozcru.org/wiki/ and keep working at it, hard ...



                                   _o0o_
