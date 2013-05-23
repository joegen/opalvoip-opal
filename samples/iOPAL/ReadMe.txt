iOS Sample
==========

Building OPAL for iOS, as with any iOS application, requires installation of
XCode. And yes, you must have a Mac to build for iOS.

Follow the instructions from the Wiki oin building for Mac OS-X, available at
  http://www.opalvoip.org/wiki/index.php?n=Main.BuildingPTLibMacOSX

Then to build the iOS you (currently) must use the PTLIBDIR/OPALDIR
environment variables. The following procedure should work:

  export PTLIBDIR=wherever/ptlib
  cd $PTLIBDIR
  make OS=iPhoneSimulator all
  export OPALDIR=wherever/opal
  cd $OPALDIR
  make OS=iPhoneSimulator all

Then you can fire up XCode on .../opal/samples/iOPAL/iOPAL.xcodeproj and build
the sample for the iOS simulator.

To build for an iOS device, use OS=iPhoneOS in the above make lines.

Note: you cannot run anything on Apple hardware without being a developer,
paying Apple $99 and getting a whole bunch of software certificates etc. This
is an extremeley involved process which I am not going to get into here!
