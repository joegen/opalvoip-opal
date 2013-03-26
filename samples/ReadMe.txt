OPAL Sample Application Summary:
=============================

c_api
Sample of "c" API.
usage: c_api { listen | call | mute | hold | transfer | consult | register | subscribe | record | play } [ A-party [ B-party | file ] ]


callgen
Load tester.


codectest
Codec (plug in) testing code. 
note: ./codectest --list will not work, but ./codectest --list test will list
plugins. also see: opalcodecinfo


dotnet
Simple reference to OpalDotNet.dll.


externrtp
External RTP demonstration for OPAL


faxopal
Send/receive faxes. Support for SIP and H.323 with T38, G.711 support.


h224test
OPAL application source file for testing H.224 sessions


ivropal
OPAL application source file for Interactive Voice Response using VXML.
Executes the VXML script on each call, and optional when call outgoing call is
made.

             
jester
A test program of the OPAL jitter buffer.


lidtest
OPAL Line Interface Device test environment


mfc
Sample using MFC with OPAL. Very old, and unsupported due to MFC not being
available in 


mobileopal
Sample Windows Mobile application with SIP and H.323 support. Very old,
and unsupported as Microsoft no longer use this operating system.


opalcodecinfo
Display plugin information, media formats, codec plugins, plugin list, info
about a media format.


opalecho
Audio Echo server.  Default web interface is http://localhost:3246


opalgw
SIP/H.323 gateway server. Default web interface is http://localhost:1719.


opalmcu
Multpoint Conference Unit.


openphone
Full GUI tetst application. Uses wxWidgets for portable GUI so works on
Windows, Linux and Mac OS-X.


playrtp
Play back PCAP (Wireshark) file RTP.


regtest
Test application for mass SIP registrations.


simple
Command line test application. Somewhay mis-named as it is not really very
simple any more.


sipim
SIP Instant Message client.


testpresent
Test application for presence (SIMPLE protocol) functionality.

                                   --oOo--
