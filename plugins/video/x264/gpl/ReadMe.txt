Compiling under Windows
=======================

Tricky.

First install Cygwin from http://www.cygwin.com

Next get yasm from http://yasm.tortall.net/Download.html, not the Cygwin
version, make sure you rename it to yasm.exe and put it in you path.

Download x264 from http://www.videolan.org/developers/x264.html

Unpack it into this directory (something/opal/plugins/video/H.264/gpl/x264),
or into somthing/external/x264, where "something" is the same in both cases.
That is, "external" is at the same directory as "opal".

Run bash from Cygwin, cd to the x264 directory.

Execute "./configure --enabel-shared --enable-pic" and then "make".

Now open opal/plugins/plugins_XXXX.sln (DevStudeio 2008 or 2010) and you
should be able to compile the helper application project.

At least it is possible.

For me.

At this time.

Good luck.

-----------------------
