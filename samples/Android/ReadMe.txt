Andoid Sample
=============

Building OPAL for Android is currently only supported in a Windows environment
and requires installation of several systems:

  Visual Studio 2010 or later, Express edition is OK.
  Android SDK   http://developer.android.com/sdk
  Android NDK   http://developer.android.com/tools/sdk/ndk
  vs-android    http://code.google.com/p/vs-android
  Java JDK      http://www.oracle.com/technetwork/java/javase/downloads

Then I would advise following the instructions on building for Windows first,
using the Wiki at http://www.opalvoip.org/wiki to make sure anything I did not
mention above is installed.

Then simply use the opal_samples_2010.sln solution, select Android and build.


Note, if you wish to use this on physical hardware, you will need to provide
a certificate and key. THis is done using the command:

  keytool -genkey -v -alias OpalSample -keyalg RSA -validity 10000 -keystore OpalSample.keystore

and answering all the questions about who you are and who you work for. Note,
I used a Linux box to generate the key store as I don't know where you can
get a Windows version of keytool ...
