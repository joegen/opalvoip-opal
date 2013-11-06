Building OpenPhone
==================

Last updated, 5 November 2013

An FAQ is at the end of this file.

OpenPhone is the fully featured example of everything that OPAL does
(SIP, H.323, H.239, Send FAX , GateKeeper, Start IM, Presence, Recording
and Send Audio File, etc)

A precompiled Windows installer for OpenPhone is available on Source Forge.

OpenPhone is based on wxWidgets, so as well as all the usual PTLib and
OPAL stuff, you also have to get wxWidgets installed before you can get
OpenPhone built.

You can get wxWidgets from http://www.wxwidgets.org

Before we start, the assumption is (since you are reading this) that you
already have PTLib and OPAL installed, AND BUILT.


For Windows:
------------

Note, that the standard Visual Studio projects will assume you are using
wxWidgets 3.0.

It is recommended that you use the pre-built wxWidgets system. This is
relatively straightforward, download the headers, DLL's, librarys and PDB
files (four archives) and unpack them somewhere. Youo should, of course,
check the exWidgets site for any changes to this method.

After downloading and unpacking wxWidgets, set the environment variable
WXDIR to the directory that you installed wxWidgets. Make sure you restart
Visual Studio if it was running.

If you want to build wxWidgets yourself, it should still work. If not, please
tell us.


For Linux:
----------

A yum or apt-get package for wxWidgets should be available. This might be
wxWidgets 2.8 or 2.9, and OpenPhone should still compile against these
versions.

After installing you should now be able to go to $OPALDIR/samples/openphone
and do "make opt".


For Mac OS-X
------------

This is similar to Linux, but you use "macports" to get a wxWidgets
distribution. While it should work on "Snow Leopard" or later, "Lion" is
recommended.

After installing you should now be able to go to $OPALDIR/samples/openphone
and do "make opt".


FAQ Open Phone
==============

Disclaimer:
   The follow is edit version of email from opalvoip-devel@lists.sourceforge.net
   mailing list. Since Opal is always being improved some of this information
   might be out of date.


Q. What OS does Open Phone run on?

A. OpenPhone does run on Linux, OS X, and Windows XP, Vista, 7 and 8.
   
   Only a Windows installer on Source Forge. But that is only because doing
   binary distros for Linux is a huge job, there are so many!

   An installer for OS-X is in the pipeline, just as soon as we figure out how!


Q. Only getting Audio no video?

A. At the other end point, ff Codec is using HD, you can easily end up with no
   codecs that can do size offered. Check trace log for:

   H.263    Resolution range (1280x720-1920x1080) outside all possible fixed sizes.

   Solution: At the other end point, try not having a minimum video size set
             to HD720 Codec.


Q. Having Audio Delay issue in Open Phone?
 
A. Try test option accessible via OpenPhone options dialog, that tests the
   sound card for it's usability in telephony (Added July 13th, 2013). 

   What we have found Direct Sound "clumps" audio frames. You should get 20ms
   of audio in/out every 20ms. And you certainly average that, but getting
   4x20ms audio every 80ms is not really useful, it breaks the jitter buffer
   operation.
     
   Interestingly, the old Windows Multi-media API is much better, getting
   within a millisecond of the 20ms time.  However, these tests where on
   Windows 8 Pro. I remember when Vista came out, that you needed at least
   120ms of buffers or the audio got broken up. Now, on Win8 it works quite
   nicely with only 60ms of buffers. I suspect Microsoft may have done some
   improvements there. I would be very interested in what Win7 does, but I
   don't have a copy.


Q. Does TLS/SRTP work with Open Phone for SIP and H.323? (As of May 8th, 2013)

A. From  IMTC SuperOp 2013: The big theme of the event was security. Everyone
   had TLS/SRTP capability, and wanted to test it. I tested OPAL with around a
   dozen vendors and failed on only one. I am willing to bet that that was
   their problem. And it was not any of the "big" names either, so of perhaps
   lower importance.


Q Does H.264 work with Open Phone for SIP and H.323? (As of May 8th, 2013)

A. From  IMTC SuperOp 2013: We were successfully interoperating at HD720
   resolution in both directions. I did not test "high" profile, only
   "baseline", nor was HD1080 resolution. The only issue in this areas was
   that the Windows FFMPEG decoder we are using produced artefacts for some
   vendors. When run on Mac OS-X the problems disappeared. I am guessing this
   is because the DLL we use to decode is now quite old and needs an update.
   Note there were no issues with transmitted video.

   A piece of useful information from other attendees there, was a pointer to
   http://www.libav.org, which is  fork of FFMPEG and looks to much more
   library/portability oriented. These guys do nightly builds for Windows in
   both 32bit and 64bit variants. Hopefully, all the numerous compile issues
   we have had will go away at last if we just require this library. Some
   work yet to done there.


Q. Does Secondary video channel, a.k.a "presentation video" or "content role
   video" or H.239 video, with Open Phone for SIP and H.323? (As of May 8th, 2013)

A. From  IMTC SuperOp 2013: For both H.323 and SIP, if it was offered to us,
   we accepted and displayed the second video stream correctly. However, we
   were unable open a transmit secondary video. For SIP this was because we
   do not yet support BFCP (Binary Floor Control Protocol) so cannot obtain
   the "chair" allowing us to send.

   For H.323/H.239 a quick hack was tried to get the chair (or token in H.329
   language) and this was partially successful. The one MCU I had time to try
   would now allow the transmission of the video, however, it did not display
   it. Wireshark logs showed all was being sent correctly. Unfortunately, the
   vendor and I could not determine why before we had to move on to the next
   test.


Q. On Linux what video cameras work?

A. Depends on Video drivers and kernel. Check log file for issues. Level 5 may
   be needed. example: Logitech B990 start working with 3.7.1 kernel on Fedora box.


Q. Missing codecs for Open Phone?

A. Check plugins are installed and Check the directories in PTLIBPLUGINDIR
   environment variable, or if not set, then check the default directories
   are ".:/usr/lib/ptlib:/usr/lib/pwlib" and INSTALLDIR/lib/opal-3.10.6

 