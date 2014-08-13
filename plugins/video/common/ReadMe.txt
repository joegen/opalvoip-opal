Introduction
============

This document describes how to compile OPAL plugin codecs that require 3rd
party codec library, FFmpeg or libav.

Note: libav is a fork of the original FFmpeg, it is becoming more popular
      as it is seems better oriented to being a library, rather than support
      for an application, which isnhow FFmpeg started in the first place.

It would have been nice to have just used the precompiled binaries provided
by the libav project web sites. However, as that includes x264 encoder, it
means the binaries are GPL, thus we are unable to use them with MPL used by
OPAL. Things are hard enough without lawyers involved. :-(



Unix
----

As a rule, on Linux systems, it will either be a system library (yum, apt-get
or Mac "port") and the configure will find it.

Alternatively, you can do a download the tarball from http://www.libav.org
and do the traditional "./configure; make; sudo make install" sequence
to create the libraries. Again, the OPAL ./configure should then find them
in, typically, /usr/local.

It shouldn't matter if you are using FFmpeg or libav, either should work. Of
course, there are never any guarantees!


Windows
-------

There is the easy way to get ffmpeg/libav on Windows; and there is the hard way.

Please, choose the easy way. Pretty please. And if you don't, then, really,
do not come crying to us!


1) Easy way: get precompiled binaries from:

   http://ffmpeg.zeranoe.com/builds/

and get the "Dev" 7z file. Drop its contents into opal/plugins/video/common,
making sure you rename it to ffmpeg-win32-dev or ffmpeg-win64-dev. Then you
need to get the "Shared" 7z file and extract the avcodec-XX.dll,
avutil-XX.dll and swresample-X.dll files from it, placing them in the same
directory as the compiled plug in, typically "C:\Program Files\PTLib Plug Ins"


2) The hard way. Building it from source.

Now, these are just some brief notes to get you started. It is not easy to do
this, it is quite likely to go wrong in some mysterious way. Don't ask us if
it does.

You will need to install Cygwin from http://www.cygwin.org.

Then download libav from http://www.libav.org

Now even though you are on Windows, everything is Unix-like via Cygwin. So
fire up a cygwin shell and un-tar the libav tarball, then use the following
configure line:

   ./configure --target-os=mingw32 --arch=i686 --cross-prefix=i686-pc-mingw32- \
               --enable-cross-compile --enable-shared --disable-static \
               --disable-avdevice --disable-avformat --disable-avfilter \
               --disable-avresample --disable-zlib --disable-network \
               --disable-doc --disable-programs

then make. If you're lucky you get some DLLs in:
   .../libavcodec/avcodec-54.dll
   .../libavutil/avutil-52.dll

Copy these wherever you are putting the plug ins, typically C:\PTLib_Plugins.



Then, get x264 from http://www.videolan.org/developers/x264.html, un-tar it
to either .../external/x264 where ... is the directory opal is in, or put
it into .../opal/plugins/video/H.264/gpl/x264. Then use the configure line:

   ./configure --host=i686-pc-mingw32 --cross-prefix=i686-pc-mingw32- \
               --enable-win32thread --enable-shared --disable-cli

and make. There should now be a libx264-XXX.dll in the top directory. Copy
that to wherever you are putting the plug ins, typically C:\PTLib_Plugins.

After that, the H.263, H.264 and MPEG4 plug ins should build using the Visual
Studio solution. Make sure you also build the "H.264 (x264)) Helper" project
and copy the x264plugin_helper.exe as well.


Good luck!


                                   _o0o_
