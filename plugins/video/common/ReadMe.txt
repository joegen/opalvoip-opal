Introduction
============

This document describes how to compile plugin codecs that require FFMPEG on both Opal 
on Windows and Linux.



Credits
=======

Many thanks to the H323Plus team for making thes H.264/ffmpeg binaries available.
The original link is:

   http://www.h323plus.org/source/download/ffmpeg_x264.zip


Many thanks to the FFMPEG autobuild maintainers for the sources 
The source used to create these above binaries is available from:

    http://www.opalvoip.org/bin/ffmpeg-r26400-swscale-r32676.tar.bz2

The original link is:

    http://ffmpeg.arrozcru.org/autobuilds/ffmpeg/sources/ffmpeg-r26400-swscale-r32676.tar.bz2



Building and installing FFmpeg on Linux
=======================================

IMPORTANT NOTE: Some Opal video codecs need more than just ffmpeg to compile.
                For instance, H.264 needs the x264 library. See the codec-specific
                ReadMe.txt files for more information.

If your favourite distribution has a devel package for fmpeg, your best approach 
is to use it and allow the Opal plugin configure to find it.

If it does not find it, then make sure you have the development package installed and
not just the binary program.

If you are not able to get a package for your distro (unlikely), then you need to get the tar ball 
and compile it yourself

The following download is known to work:

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

There is one easy way to get ffmpeg on Windows; there is one not-so-easy way, 
and there is one very hard way.

Please choose the easy way. Pretty please. 



1) Easy way: get precompiled ffmpeg binaries from:

    http://www.opalvoip.org/bin/ffmpeg_x264.zip

Add the following directory to the include path for the compiler
for the "H.264 (x264) Video Codec" project, or whatever codec project you are compiling

     ...\opal\plugins\video\common\ffmpeg

This directory is already populated with header files that are known to work with the above binaries.



2) Not so easy way: If you require a particualr version of FFMPEG, you can use a precompiled shared 
library version  from the Unoffical FFmpeg Win32 Build site:

    http://ffmpeg.arrozcru.org/autobuilds/

As a rule use the latest snapshot, but there are never any guarantees. All
I can say is at time of writing the following worked:

    ffmpeg-r26400-swscale-r32676-mingw32-shared.7z

The first archive contains the DLL's, copy th e following:

    avcodec-*.dll
    avutil-*.dll
    avcore-*.dll

to the C:\PTLib_Plugins directory. Or where ever you have installed the
OPAL plug-ins.


Finally, to build the plug ins you need some header files. These are available
from the Unoffical FFmpeg Win32 Build site:

    http://ffmpeg.arrozcru.org/autobuilds/

with archive:

    ffmpeg-r26400-swscale-r32676-mingw32-shared-dev.7z

This archive is unpacked into the following directory, replacing it's current contents 

        ...\opal\plugins\video\common\ffmpeg


Add the following directory to the include path for the compiler
for the "H.264 (x264) Video Codec" project, or whatever codec project you are compiling



3) Very hard way - compile FFMPEG for Windows yourself.

If you really need to make patches to the code, so need to build from source,
start at http://ffmpeg.arrozcru.org/wiki/ and keep working at it, hard ...

Good luck!


                                   _o0o_
