Building OpenPhone
==================

Last updated, 5 Apr 2013

OpenPhone support Windows, Linux and Mac OS X.
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

