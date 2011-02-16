Building and installing H.264
=============================

This codec is based on FFMPEG and x264.

Note, due to the GPL license for the x264 codec, it cannot be linked to
directly. So, there is a special program that runs and the plig in talks
to it via a socket. Stupid, but .....


For Linux, the configure should find FFMPEG and point the compilation to it.
You just need to make sure it is installed, using yum or apt-get or
whatever your distribution needs.


For Windows, please follow the instructions in ../common/ReadMe.txt to get
FFMPEG installed.

Then you need to get an x264 snapshot from:

	ftp://ftp.videolan.org/pub/videolan/x264/snapshots/

and unpack it into the x264 sub-directory.