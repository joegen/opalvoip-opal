Building OpenPhone
==================

Last updated, 4 July 2013

An FAQ is at the end of this file.

OpenPhone is the fully featured example of everything that OPAL does
(SIP, H.323, H.239, Send FAX , GateKeeper, Start IM, Presence, Recording
and Send Audio File, etc)

A precompiled Windows installer is available on Source Forge.

OpenPhone is based on wxWidgets, so as well as all the usual PTLib and
OPAL stuff, you also have to get wxWidgets installed and compiled before
you can get OpenPhone built. Though, many Linux distributions will have it
as a system isntallable package.

You can get wxWidgets from http://www.wxwidgets.org

Before we start, the assumption is (since you are reading this) that you
already have PWLib and OPAL installed, AND BUILT.


For Windows:
------------
  1.  Download the installer for wxWidgets, eg wxMSW-2.8.4-Setup.exe, or later

  2.  Run the installer.

  3.  Set the environment variable WXDIR to the directory that you installed
      wxWidgets into in step 2. Also set WXVER to 26 or 28 depending on the
      version of wxWidgets you installed. Be sure to restart DevStudio.

  4.  Open the DevStudio workspace %WXDIR%\build\msw\wx.dsw
      If you are using Visual Studio .NET 2003 or Visual C++ Express 2005, you
      will be asked whether to convert project files. Let it do the conversion
      and use the converted projects.

  5.  Build the "Unicode Release" and "Unicode Debug" versions of everything.
      The safest way is to simply select "Unicode Debug" in the tool bar drop
      down list, go "Build Solution", then do the same for "Unicode Release".

  6.  Open the DevStudio workspace %WXDIR%\utils\wxrc\wxrc.dsw, if missing then
      %WXDIR%\utils\wxrc\wxrc.dsp will do. Let it convert the projects as before.

  7.  Build the "Unicode Release" version. Note: when using VC Express 2005, you may get
      a large number of undefined symbols when linking. To fix this, add the
      following libraries to the linker command line:

                 user32.lib ole32.lib advapi32.lib shell32.lib

  8.  Copy the %WXDIR%\utils\wxrc\vc_mswu\wxrc.exe file to %WXDIR%\bin\wxrc.exe.
      You may need to create the %WXDIR%\bin directory.

  9.  wxWidgets is now ready to use.

You should now be able to open %OPALDIR%/opal_samples_2003.sln or
%OPALDIR%/opal_samples_2005.sln and build OpenPhone.



For Linux:
----------
If a yum or apt-get package is available for your distribution, then you can
skip this section.

  1.  Download the tar file, eg wxGTK-2.8.4.tar.gz, or later.

  2.  Unpack it somewhere, you don't need to be root (yet)

  3.  cd wxGTK-2.8.?

  4.  Build wxWidgets. You can read the instructions in README-GTX.txt,
      or, if you are too lazy and feel lucky, follow the salient bits
      below (taken fromREADME-GTX.txt) which worked for me:

  5.  mkdir build_gtk

  6.  cd build_gtk

  7.  ../configure --with-gtk=2

  8.  make

  9.  "su" and enter root password>

  11. make install

  12. ldconfig

  13. wxWidgets is now ready to use.

You should now be able to go to $OPALDIR/samples/openphone and do "make opt".


For Mac OS-X
------------

  1.  Use mac ports to install wxWidgets. For Lion and Mountain Lion this is
      very straightforward as the 64 bit version 2.9 of wxWidgets is \
      supported.

      Alternatively, download stable version at:
         http://sourceforge.net/projects/wxwindows/files/2.8.12/wxMac-2.8.12.tar.gz/download?use_mirror=iweb
	    and follow instructions at:
         http://stackoverflow.com/questions/5341189/how-to-use-wxwidgets-in-mac

      For 64bit support download V2.9.x
         http://hivelocity.dl.sourceforge.net/project/wxwindows/2.9.4/wxWidgets-2.9.4.tar.bz2
      or try latest at:
         http://sourceforge.net/projects/wxwindows/files/latest/download

  2.  Build PTLib/OPAL as per OPAL VoIP web site, with one additional caveat.

      If you are using 64 bit Snow Leopard (OS X 10.6) or other 64bit OS X,
      you must configure with the --enable-force32 option as wxWidgets only
      supports 32 bit OS-X at the time of writing.

  	  Version 2.9 or later of wxWidgets supports 64bit on OS X.
        See: http://wiki.wxwidgets.org/Development:_wxMac#Building_under_10.6_Snow_Leopard
        and http://stackoverflow.com/questions/8675304/cant-compile-wxwidget-application-for-macosx

You should now be able to go to $OPALDIR/samples/openphone and do "make".



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

 