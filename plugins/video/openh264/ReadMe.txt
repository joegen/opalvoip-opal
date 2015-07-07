Cisco Systems OpenH264
======================

The plugin should compile without any additional files, to run, however, you
need to obtain the binary package.

Go to https://github.com/cisco/openh264/blob/master/RELEASES to find out where
the binary DLL/so/dylib file is kept. Once you have it rename it to

Windows 32 : openh264_x86.dll
Windows 64 : openh264_x64.dll
Linux      : libopenh264.so
OS-X       : libopenh264.dylib

For Windows, simply put it in the same directory as the plug in DLL/so/dylib
file. For example "C:\Program Files\PTLib Plug Ins". It is required at run
time only, and the plug in can be built without it.

For Linux and OS-X, you should put these into the library path, e.g. /usr/lib.
Directories such as /usr/local/lib or /opt/local/lib can work provided ld.conf
is configured, or LD_LIBRARY_PATH environment variable is set. Note, in this
case, the binary must be put in place before "./configure" is run or the plug
in will not be built.

Note the currently supported version is 1.4.0.

More details on the codec, it's licensing and patent indemnity is available
at http://www.openh264.org.


                                   _o0o_
