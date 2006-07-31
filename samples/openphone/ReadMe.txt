Building OpenPhone
==================


OpenPhone is based onw wxWidgets so as well as all the usual PWLib and
OPAL stuff, you also have to get wxWidgets installed and compiled before
you can get OpenPhone built.

You can get wxWidgets from http://www.wxwidgets.org


Before we start, teh assumption is (since you are reading this) that you
already have PWLib and OPAL installed, AND BUILT.


For windows:
  1.  Download the installer, eg wxMSW-2.6.3-Setup-1.exe
  2.  Run the installer
  3.  Set the environment variable WXDIR to the directory that you installed
      wxWidgets into in step 2.
  4.  Open the DevStudio workspace %WXDIR%\build\msw\wx.dsw
      If you are using .NET, it will ask to convert it, let it and use the
      converted files.
  5.  Build everything, to be sure.
  6.  Open the DevStudio workspace %WXDIR%\utils\wxrc\wxrc.dsw
  7.  Build the release version.
  8.  Copy the %WXDIR%\utils\wxrc\vc_msw\wxrc.exe file to %WXDIR%\bin\wxrc.exe
  9.  wxWidgets is now ready to use.

You should now be able to open %OPALDIR%/opal_samples.sln and build OpenPhone



For Linux:

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
  9.  su
  10. <type root password>
  11. make install
  12. ldconfig
  13. wxWidgets is now ready to use.

You should now be able to go to $OPALDIR/samples/openphone and do "make opt".
