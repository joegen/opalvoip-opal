Building OpenPhone
==================

Last updated, 11 August 2006


OpenPhone is based onw wxWidgets so as well as all the usual PWLib and
OPAL stuff, you also have to get wxWidgets installed and compiled before
you can get OpenPhone built.

You can get wxWidgets from http://www.wxwidgets.org


Before we start, the assumption is (since you are reading this) that you
already have PWLib and OPAL installed, AND BUILT.

Note: you will need to make sure PWlib is built with the "Expat XML"
configuration option enabled. If you get errors like "unresolved external
symbol _XML_ParserFree referenced" then this is the problem. The expat
library can be found the "external/expat" of the OpenH323 CVS.


For Windows:
------------
  1.  Download the installer for wxWidgets, eg wxMSW-2.6.3-Setup-1.exe or
      wxMSW-2.8.3-Setup.exe

  2.  Run the installer.

  3.  Set the environment variable WXDIR to the directory that you installed
      wxWidgets into in step 2. Also set WXVER to 26 or 28 depending on the
      version of wxWidgets you installed. Be sure to restart DevStudio.

  4.  Open the DevStudio workspace %WXDIR%\build\msw\wx.dsw
      If you are using Visual Studio .NET 2003 or Visual C++ Express 2005, you
      will be asked whether to convert project files. Let it do the conversion
      and use the converted projects.

  5.  Build the Release and Debug versions of everything. The safest way is to
      simply select "Debug", go "Build Solution", then select "Release" and go
      "Build Solution" again.

  6.  Open the DevStudio workspace %WXDIR%\utils\wxrc\wxrc.dsw, let it convert
      the projects as before.

  7.  Build the release version. Note: when using VC Express 2005, you may get
      a large number of undefined symbols when linking. To fix this, add the
      following libraries to the linker command line:

                 user32.lib ole32.lib advapi32.lib shell32.lib

  8.  Copy the %WXDIR%\utils\wxrc\vc_msw\wxrc.exe file to %WXDIR%\bin\wxrc.exe

  9.  wxWidgets is now ready to use.

You should now be able to open %OPALDIR%/opal_samples.sln and build OpenPhone. 



For Linux:
----------
  1.  Download the tar file, eg wxGTK-2.6.3.tar.gz

  2.  Unpack it somewhere, you don't need to be root (yet)

  3.  cd wxGTK-2.6.3

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

