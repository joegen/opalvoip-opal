/*
 * channels.cxx
 *
 * H.323 protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2000 Equivalence Pty. Ltd.
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Portions of this code were written with the assisance of funding from
 * Vovida Networks, Inc. http://www.vovida.com.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: channels.cxx,v $
 * Revision 2.46  2007/09/25 09:48:34  rjongbloed
 * Added H.323 support for videoFastUpdatePicture
 *
 * Revision 2.45  2007/08/31 11:36:13  csoutheren
 * Fix usage of GetOtherPartyConnection without PSafePtr
 *
 * Revision 2.44  2007/08/21 00:19:46  rjongbloed
 * Fixed previous check in which mysteriously changed a flag.
 *
 * Revision 2.43  2007/08/17 09:00:40  csoutheren
 * Remove commented-out code
 *
 * Revision 2.42  2007/08/16 03:10:35  rjongbloed
 * Fixed setting of dynamic RTP in OLC on rx channel for slow start, only worked for fast cononect.
 * Also fixed sending dynamic RTP type in OLC for Tx channel.
 *
 * Revision 2.41  2007/08/13 16:19:08  csoutheren
 * Ensure CreateMediaStream is only called *once* for each stream in H.323 calls
 *
 * Revision 2.40  2007/06/28 12:08:26  rjongbloed
 * Simplified mutex strategy to avoid some wierd deadlocks. All locking of access
 *   to an OpalConnection must be via the PSafeObject locks.
 *
 * Revision 2.39  2007/04/04 02:12:00  rjongbloed
 * Reviewed and adjusted PTRACE log levels
 *   Now follows 1=error,2=warn,3=info,4+=debug
 *
 * Revision 2.38  2007/03/29 05:16:49  csoutheren
 * Pass OpalConnection to OpalMediaSream constructor
 * Add ID to OpalMediaStreams so that transcoders can match incoming and outgoing codecs
 *
 * Revision 2.37  2007/03/13 02:17:46  csoutheren
 * Remove warnings/errors when compiling with various turned off
 *
 * Revision 2.36  2007/03/01 03:36:28  csoutheren
 * Use local jitter buffer values rather than getting direct from OpalManager
 *
 * Revision 2.35  2007/01/10 09:16:55  csoutheren
 * Allow compilation with video disabled
 *
 * Revision 2.34  2006/08/10 05:10:30  csoutheren
 * Various H.323 stability patches merged in from DeimosPrePLuginBranch
 *
 * Revision 2.33  2006/07/24 14:03:40  csoutheren
 * Merged in audio and video plugins from CVS branch PluginBranch
 *
 * Revision 2.32.2.1  2006/08/09 12:49:21  csoutheren
 * Improve stablity under heavy H.323 load
 *
 * Revision 2.32  2006/04/10 05:16:09  csoutheren
 * Populate media stream info even when OLCack only contains media control information
 *
 * Revision 2.31  2006/03/29 23:57:52  csoutheren
 * Added patches from Paul Caswell to provide correct operation when using
 * external RTP with H.323
 *
 * Revision 2.30  2006/01/02 15:51:44  dsandras
 * Merged changes from OpenH323 Atlas_devel_2.
 *
 * Revision 2.29  2005/09/04 06:51:52  rjongbloed
 * Added OpalMediaCommand mechanism (via PNotifier) for media streams
 *   and media transcoders to send commands back to remote.
 *
 * Revision 2.28  2005/09/04 06:23:39  rjongbloed
 * Added OpalMediaCommand mechanism (via PNotifier) for media streams
 *   and media transcoders to send commands back to remote.
 *
 * Revision 2.27  2005/08/31 13:19:25  rjongbloed
 * Added mechanism for controlling media (especially codecs) including
 *   changing the OpalMediaFormat option list (eg bit rate) and a completely
 *   new OpalMediaCommand abstraction for things like video fast update.
 *
 * Revision 2.26  2005/07/11 01:52:24  csoutheren
 * Extended AnsweringCall to work for SIP as well as H.323
 * Fixed problems with external RTP connection in H.323
 * Added call to OnClosedMediaStream
 *
 * Revision 2.25  2004/04/07 08:21:01  rjongbloed
 * Changes for new RTTI system.
 *
 * Revision 2.24  2004/02/19 10:47:02  rjongbloed
 * Merged OpenH323 version 1.13.1 changes.
 *
 * Revision 2.23  2003/03/17 10:26:59  robertj
 * Added video support.
 *
 * Revision 2.22  2003/01/07 04:39:53  robertj
 * Updated to OpenH323 v1.11.2
 *
 * Revision 2.21  2002/11/10 11:33:18  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.20  2002/09/04 06:01:47  robertj
 * Updated to OpenH323 v1.9.6
 *
 * Revision 2.19  2002/07/01 04:56:31  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 2.18  2002/04/18 03:41:54  robertj
 * Fixed logical channel so does not delete media stream too early.
 *
 * Revision 2.17  2002/04/15 08:51:42  robertj
 * Fixed correct setting of jitter buffer size in RTP media stream.
 *
 * Revision 2.16  2002/03/22 06:57:49  robertj
 * Updated to OpenH323 version 1.8.2
 *
 * Revision 2.15  2002/02/19 07:45:28  robertj
 * Restructured media bypass functions to fix problems with RFC2833.
 *
 * Revision 2.14  2002/02/13 04:17:53  robertj
 * Fixed return of both data and control if only have one from OLC.
 *
 * Revision 2.13  2002/02/11 09:32:12  robertj
 * Updated to openH323 v1.8.0
 *
 * Revision 2.12  2002/02/11 07:40:02  robertj
 * Added media bypass for streams between compatible protocols.
 *
 * Revision 2.11  2002/01/22 05:24:36  robertj
 * Added enum for illegal payload type value.
 * Update from OpenH323
 *
 * Revision 2.10  2002/01/14 06:35:57  robertj
 * Updated to OpenH323 v1.7.9
 *
 * Revision 2.9  2001/11/13 04:29:47  robertj
 * Changed OpalTransportAddress CreateTransport and CreateListsner functions
 *   to have extra parameter to control local binding of sockets.
 *
 * Revision 2.8  2001/11/02 10:45:19  robertj
 * Updated to OpenH323 v1.7.3
 *
 * Revision 2.7  2001/10/15 04:34:42  robertj
 * Added delayed start of media patch threads.
 *
 * Revision 2.6  2001/10/05 00:22:13  robertj
 * Updated to PWLib 1.2.0 and OpenH323 1.7.0
 *
 * Revision 2.5  2001/10/03 05:56:15  robertj
 * Changes abndwidth management API.
 *
 * Revision 2.4  2001/08/17 08:32:17  robertj
 * Update from OpenH323
 *
 * Revision 2.3  2001/08/13 05:10:39  robertj
 * Updates from OpenH323 v1.6.0 release.
 *
 * Revision 2.2  2001/08/01 05:17:45  robertj
 * Made OpalMediaFormatList class global to help with documentation.
 *
 * Revision 2.1  2001/07/30 01:40:01  robertj
 * Fixed GNU C++ warnings.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.136  2003/02/10 05:36:13  robertj
 * Fixed returning mediaControlChannel address in preference to mediaChannel
 *   address as Cisco's just feed your own address back at you.
 *
 * Revision 1.135  2002/12/18 11:20:49  craigs
 * Fixed problem with T.38 channels SEGVing thanks to Vyacheslav Frolov
 *
 * Revision 1.134  2002/12/17 08:48:09  robertj
 * Set silence suppression mode earlier in codec life so gets correct
 *   value for silenceSuppression in fast start OLC's.
 *
 * Revision 1.133  2002/12/16 08:20:04  robertj
 * Fixed problem where a spurious RTP packet full of zeros could be sent
 *   at the beginning of the transmission, thanks Bruce Fitzsimons
 *
 * Revision 1.132  2002/11/26 02:59:25  robertj
 * Added logging to help find logical channel thread stop failures.
 *
 * Revision 1.131  2002/10/31 00:37:47  robertj
 * Enhanced jitter buffer system so operates dynamically between minimum and
 *   maximum values. Altered API to assure app writers note the change!
 *
 * Revision 1.130  2002/08/05 10:03:47  robertj
 * Cosmetic changes to normalise the usage of pragma interface/implementation.
 *
 * Revision 1.129  2002/06/28 03:34:28  robertj
 * Fixed issues with address translation on gatekeeper RAS channel.
 *
 * Revision 1.128  2002/06/25 08:30:12  robertj
 * Changes to differentiate between stright G.723.1 and G.723.1 Annex A using
 *   the OLC dataType silenceSuppression field so does not send SID frames
 *   to receiver codecs that do not understand them.
 *
 * Revision 1.127  2002/06/24 00:07:31  robertj
 * Fixed bandwidth usage being exactly opposite (adding when it should
 *   be subtracting), thanks Saswat Praharaj.
 *
 * Revision 1.126  2002/05/23 04:53:57  robertj
 * Added function to remove a filter from logical channel.
 *
 * Revision 1.125  2002/05/10 05:47:15  robertj
 * Added session ID to the data logical channel class.
 *
 * Revision 1.124  2002/05/07 23:49:11  robertj
 * Fixed incorrect setting of session ID in data channel OLC, caused an
 *   incorrect optional field to be included, thanks Ulrich Findeisen.
 *
 * Revision 1.123  2002/05/03 00:07:24  robertj
 * Fixed missing setting of isRunning flag in external RTP channels.
 *
 * Revision 1.122  2002/05/02 07:56:27  robertj
 * Added automatic clearing of call if no media (RTP data) is transferred in a
 *   configurable (default 5 minutes) amount of time.
 *
 * Revision 1.121  2002/05/02 06:28:53  robertj
 * Fixed problem with external RTP channels not fast starting.
 *
 * Revision 1.120  2002/04/17 05:56:05  robertj
 * Added trace output of H323Channel::Direction enum.
 *
 * Revision 1.119  2002/02/25 08:42:26  robertj
 * Fixed comments on the real time requirements of the codec.
 *
 * Revision 1.118  2002/02/19 06:15:20  robertj
 * Allowed for RTP filter functions to force output of packet, or prevent it
 *   from being sent overriding the n frames per packet algorithm.
 *
 * Revision 1.117  2002/02/09 04:39:05  robertj
 * Changes to allow T.38 logical channels to use single transport which is
 *   now owned by the OpalT38Protocol object instead of H323Channel.
 *
 * Revision 1.116  2002/02/05 08:13:02  robertj
 * Added ability to not have addresses when external RTP channel created.
 *
 * Revision 1.115  2002/02/04 06:04:19  robertj
 * Fixed correct exit on terminating transmit channel, thanks Norwood Systems.
 *
 * Revision 1.114  2002/01/27 10:49:51  rogerh
 * Catch a division by zero case in a PTRACE()
 *
 * Revision 1.113  2002/01/24 03:33:07  robertj
 * Fixed payload type being incorrect for audio after sending RFC2833 packet.
 *
 * Revision 1.112  2002/01/22 22:48:25  robertj
 * Fixed RFC2833 support (transmitter) requiring large rewrite
 *
 * Revision 1.111  2002/01/22 07:08:26  robertj
 * Added IllegalPayloadType enum as need marker for none set
 *   and MaxPayloadType is a legal value.
 *
 * Revision 1.110  2002/01/22 06:05:03  robertj
 * Added ability for RTP payload type to be overridden at capability level.
 *
 * Revision 1.109  2002/01/17 07:05:03  robertj
 * Added support for RFC2833 embedded DTMF in the RTP stream.
 *
 * Revision 1.108  2002/01/17 00:10:37  robertj
 * Fixed double copy of rtpPayloadType in RTP channel, caused much confusion.
 *
 * Revision 1.107  2002/01/14 05:18:44  robertj
 * Fixed typo on external RTP channel constructor.
 *
 * Revision 1.106  2002/01/10 05:13:54  robertj
 * Added support for external RTP stacks, thanks NuMind Software Systems.
 *
 * Revision 1.105  2002/01/09 06:05:55  robertj
 * Rearranged transmitter timestamp calculation to allow for a codec that has
 *   variable number of timestamp units per call to Read().
 *
 * Revision 1.104  2001/12/22 01:50:47  robertj
 * Fixed bug in data channel (T.38) negotiations, using wrong PDU subclass.
 * Fixed using correct port number in data channel (T.38) negotiations.
 * Improved trace logging.
 *
 * Revision 1.103  2001/11/28 00:09:14  dereks
 * Additional information in PTRACE output.
 *
 * Revision 1.102  2001/11/09 05:39:54  craigs
 * Added initial T.38 support thanks to Adam Lazur
 *
 * Revision 1.101  2001/10/24 00:55:49  robertj
 * Made cosmetic changes to H.245 miscellaneous command function.
 *
 * Revision 1.100  2001/10/23 02:17:16  dereks
 * Initial release of cu30 video codec.
 *
 * Revision 1.99  2001/09/13 08:20:27  robertj
 * Fixed broken back out of rev 1.95, thanks Santiago Garcia Mantinan
 *
 * Revision 1.98  2001/09/11 00:21:23  robertj
 * Fixed missing stack sizes in endpoint for cleaner thread and jitter thread.
 *
 * Revision 1.97  2001/08/28 09:28:28  robertj
 * Backed out change in revision 1.95, not compatible with G.711
 *
 * Revision 1.96  2001/08/16 06:34:42  robertj
 * Plugged memory leak if using trace level 5.
 *
 * Revision 1.95  2001/08/10 01:34:41  robertj
 * Fixed problem with incorrect timestamp if codec returns more than one
 *    frame in read, thanks Lee Kirchhoff.
 *
 * Revision 1.94  2001/08/06 05:36:00  robertj
 * Fixed GNU warnings.
 *
 * Revision 1.93  2001/08/06 03:08:56  robertj
 * Fission of h323.h to h323ep.h & h323con.h, h323.h now just includes files.
 *
 * Revision 1.92  2001/07/24 02:26:44  robertj
 * Added start for handling reverse channels.
 *
 * Revision 1.91  2001/07/17 04:44:31  robertj
 * Partial implementation of T.120 and T.38 logical channels.
 *
 * Revision 1.90  2001/07/12 07:28:41  yurik
 * WinCE fix: Sleep(0) in Main to get system chance to digest
 *
 * Revision 1.89  2001/06/15 07:20:35  robertj
 * Moved OnClosedLogicalChannel() to be after channels threads halted.
 *
 * Revision 1.88  2001/06/02 01:35:32  robertj
 * Added thread names.
 *
 * Revision 1.87  2001/05/31 06:29:48  robertj
 * Changed trace of RTP mismatch so only displays for first n packets then
 *   does not dump messages any more. Was exactly the opposite.
 *
 * Revision 1.86  2001/04/20 02:32:07  robertj
 * Improved logging of bandwith, used more intuitive units.
 *
 * Revision 1.85  2001/04/02 04:12:53  robertj
 * Fixed trace output from packet transmit timing.
 *
 * Revision 1.84  2001/03/23 05:38:30  robertj
 * Added PTRACE_IF to output trace if a conditional is PTrue.
 *
 * Revision 1.83  2001/02/09 05:13:55  craigs
 * Added pragma implementation to (hopefully) reduce the executable image size
 * under Linux
 *
 * Revision 1.82  2001/02/07 05:04:45  robertj
 * Improved codec read analysis debug output.
 *
 * Revision 1.81  2001/02/06 07:40:46  robertj
 * Added debugging for timing of codec read.
 *
 * Revision 1.80  2001/01/25 07:27:16  robertj
 * Major changes to add more flexible OpalMediaFormat class to normalise
 *   all information about media types, especially codecs.
 *
 * Revision 1.79  2000/12/20 00:50:42  robertj
 * Fixed MSVC compatibility issues (No trace).
 *
 * Revision 1.78  2000/12/19 22:33:44  dereks
 * Adjust so that the video channel is used for reading/writing raw video
 * data, which better modularizes the video codec.
 *
 * Revision 1.77  2000/12/17 22:45:36  robertj
 * Set media stream threads to highest unprivileged priority.
 *
 * Revision 1.76  2000/11/24 10:52:50  robertj
 * Modified the ReadFrame/WriteFrame functions to allow for variable length codecs.
 * Added support for G.729 annex B packetisation scheme in RTP.
 * Fixed bug in fast started G.711 codec not working in one direction.
 *
 * Revision 1.75  2000/10/24 00:00:09  robertj
 * Improved memory hogging hash function for logical channels.
 *
 * Revision 1.74  2000/10/19 04:05:01  robertj
 * Added compare function for logical channel numbers, thanks Yuriy Ershov.
 *
 * Revision 1.73  2000/09/23 06:54:44  robertj
 * Prevented call of OnClose call back if channel was never opened.
 *
 * Revision 1.72  2000/09/22 01:35:49  robertj
 * Added support for handling LID's that only do symmetric codecs.
 *
 * Revision 1.71  2000/09/22 00:32:33  craigs
 * Added extra logging
 * Fixed problems with no fastConnect with tunelling
 *
 * Revision 1.70  2000/09/20 01:50:21  craigs
 * Added ability to set jitter buffer on a per-connection basis
 *
 * Revision 1.69  2000/09/14 23:03:45  robertj
 * Increased timeout on asserting because of driver lockup
 *
 * Revision 1.68  2000/08/31 08:15:40  robertj
 * Added support for dynamic RTP payload types in H.245 OpenLogicalChannel negotiations.
 *
 * Revision 1.67  2000/08/30 06:33:01  craigs
 * Add fix to ignore small runs of consectuive mismatched payload types
 *
 * Revision 1.66  2000/08/25 01:10:28  robertj
 * Added assert if various thrads ever fail to terminate.
 *
 * Revision 1.65  2000/08/21 02:50:28  robertj
 * Fixed race condition if close call just as slow start media channels are opening.
 *
 * Revision 1.64  2000/07/14 14:04:49  robertj
 * Clarified a debug message.
 *
 * Revision 1.63  2000/07/14 12:47:36  robertj
 * Added clarification to some logging messags.
 *
 * Revision 1.62  2000/07/13 16:05:47  robertj
 * Removed time critical priority as it can totally slag a Win98 system.
 * Fixed trace message displaying mismatched codecs in RTP packet around the wrong way.
 *
 * Revision 1.61  2000/07/11 11:15:52  robertj
 * Fixed bug when terminating RTP receiver and not also terminating transmitter.
 *
 * Revision 1.60  2000/06/23 02:04:01  robertj
 * Increased the priority of the media channels, only relevent for Win32 at this time.
 *
 * Revision 1.59  2000/06/15 01:46:15  robertj
 * Added channel pause (aka mute) functions.
 *
 * Revision 1.58  2000/05/18 12:10:50  robertj
 * Removed all Sleep() calls in codec as timing innacuracies make it unusable. All
 *    codec implementations must thus have timing built into them, usually using I/O.
 *
 * Revision 1.57  2000/05/11 23:54:25  craigs
 * Fixed the Windows fix with another Linux fix. But it worked OK on the Alpha!
 *
 * Revision 1.56  2000/05/11 09:56:46  robertj
 * Win32 compatibility and addition of some extra debugging on codec timing.
 *
 * Revision 1.55  2000/05/11 02:27:18  robertj
 * Added "fail safe" timer sleep on codec writes when on output of jitter buffer.
 *
 * Revision 1.54  2000/05/04 11:52:34  robertj
 * Added Packets Too Late statistics, requiring major rearrangement of jitter
 *    buffer code, not also changes semantics of codec Write() function slightly.
 *
 * Revision 1.53  2000/05/02 04:32:26  robertj
 * Fixed copyright notice comment.
 *
 * Revision 1.52  2000/05/01 01:01:49  robertj
 * Added flag for what to do with out of orer packets (use if jitter, don't if not).
 *
 * Revision 1.51  2000/04/28 13:01:44  robertj
 * Fixed problem with adjusting tx/rx frame counts in capabilities during fast start.
 *
 * Revision 1.50  2000/04/10 19:45:49  robertj
 * Changed RTP data receive tp be more forgiving, will process packet even if payload type is wrong.
 *
 * Revision 1.49  2000/03/31 20:04:28  robertj
 * Fixed log message for start/end of transmitted talk burst.
 *
 * Revision 1.48  2000/03/29 04:36:38  robertj
 * Improved some trace logging messages.
 *
 * Revision 1.47  2000/03/22 01:31:36  robertj
 * Fixed transmitter loop so codec can return multiple frames (crash in G.711 mode)
 *
 * Revision 1.46  2000/03/21 03:58:00  robertj
 * Fixed stuffed up RTP transmit loop after previous change.
 *
 * Revision 1.45  2000/03/21 03:06:49  robertj
 * Changes to make RTP TX of exact numbers of frames in some codecs.
 *
 * Revision 1.44  2000/03/20 20:59:28  robertj
 * Fixed possible buffer overrun problem in RTP_DataFrames
 *
 * Revision 1.43  2000/02/24 00:34:25  robertj
 * Fixed possible endless loop on channel abort, thanks Yura Ershov
 *
 * Revision 1.42  2000/02/04 05:11:19  craigs
 * Updated for new Makefiles and for new video transmission code
 *
 * Revision 1.41  2000/01/13 04:03:45  robertj
 * Added video transmission
 *
 * Revision 1.40  2000/01/08 06:52:10  robertj
 * Removed invalid assert
 *
 * Revision 1.39  1999/12/23 23:02:35  robertj
 * File reorganision for separating RTP from H.323 and creation of LID for VPB support.
 *
 * Revision 1.38  1999/11/22 01:37:31  robertj
 * Fixed channel closure so OnClosedLogicalChannel() only called if channel was actually started.
 *
 * Revision 1.37  1999/11/20 05:35:26  robertj
 * Extra debugging
 *
 * Revision 1.36  1999/11/20 00:53:47  robertj
 * Fixed ability to have variable sized frames in single RTP packet under G.723.1
 *
 * Revision 1.35  1999/11/19 09:06:25  robertj
 * Changed to close down logical channel if get a transmit codec error.
 *
 * Revision 1.34  1999/11/11 23:28:46  robertj
 * Added first cut silence detection algorithm.
 *
 * Revision 1.33  1999/11/06 11:01:37  robertj
 * Extra debugging.
 *
 * Revision 1.32  1999/11/06 05:37:44  robertj
 * Complete rewrite of termination of connection to avoid numerous race conditions.
 *
 * Revision 1.31  1999/11/01 00:47:46  robertj
 * Added close of logical channel on write error
 *
 * Revision 1.30  1999/10/30 12:38:24  robertj
 * Added more tracing of channel threads.
 *
 * Revision 1.29  1999/10/08 09:59:03  robertj
 * Rewrite of capability for sending multiple audio frames
 *
 * Revision 1.28  1999/10/08 04:58:37  robertj
 * Added capability for sending multiple audio frames in single RTP packet
 *
 * Revision 1.27  1999/09/23 07:25:12  robertj
 * Added open audio and video function to connection and started multi-frame codec send functionality.
 *
 * Revision 1.26  1999/09/21 14:09:02  robertj
 * Removed warnings when no tracing enabled.
 *
 * Revision 1.25  1999/09/18 13:24:38  craigs
 * Added ability to disable jitter buffer
 * Added ability to access entire RTP packet in codec Write
 *
 * Revision 1.24  1999/09/08 04:05:48  robertj
 * Added support for video capabilities & codec, still needs the actual codec itself!
 *
 * Revision 1.23  1999/09/03 02:17:50  robertj
 * Added more debugging
 *
 * Revision 1.22  1999/08/31 12:34:18  robertj
 * Added gatekeeper support.
 *
 * Revision 1.21  1999/07/16 15:02:51  robertj
 * Changed jitter buffer to throw away old packets if jitter exceeded.
 *
 * Revision 1.20  1999/07/16 00:51:03  robertj
 * Some more debugging of fast start.
 *
 * Revision 1.19  1999/07/15 14:45:35  robertj
 * Added propagation of codec open error to shut down logical channel.
 * Fixed control channel start up bug introduced with tunnelling.
 *
 * Revision 1.18  1999/07/15 09:04:31  robertj
 * Fixed some fast start bugs
 *
 * Revision 1.17  1999/07/14 06:04:04  robertj
 * Fixed setting of channel number in fast start.
 *
 * Revision 1.16  1999/07/13 09:53:24  robertj
 * Fixed some problems with jitter buffer and added more debugging.
 *
 * Revision 1.15  1999/07/13 02:50:58  craigs
 * Changed semantics of SetPlayDevice/SetRecordDevice, only descendent
 *    endpoint assumes PSoundChannel devices for audio codec.
 *
 * Revision 1.14  1999/07/10 03:01:48  robertj
 * Removed debugging.
 *
 * Revision 1.13  1999/07/09 06:09:49  robertj
 * Major implementation. An ENORMOUS amount of stuff added everywhere.
 *
 * Revision 1.12  1999/06/25 14:19:40  robertj
 * Fixed termination race condition in logical channel tear down.
 *
 * Revision 1.11  1999/06/24 13:32:45  robertj
 * Fixed ability to change sound device on codec and fixed NM3 G.711 compatibility
 *
 * Revision 1.10  1999/06/22 13:49:40  robertj
 * Added GSM support and further RTP protocol enhancements.
 *
 * Revision 1.9  1999/06/14 05:15:55  robertj
 * Changes for using RTP sessions correctly in H323 Logical Channel context
 *
 * Revision 1.8  1999/06/13 12:41:14  robertj
 * Implement logical channel transmitter.
 * Fixed H245 connect on receiving call.
 *
 * Revision 1.7  1999/06/09 06:18:00  robertj
 * GCC compatibiltiy.
 *
 * Revision 1.6  1999/06/09 05:26:19  robertj
 * Major restructuring of classes.
 *
 * Revision 1.5  1999/06/07 00:54:30  robertj
 * Displayed error on SetOption for buffer size
 *
 * Revision 1.4  1999/06/06 06:06:36  robertj
 * Changes for new ASN compiler and v2 protocol ASN files.
 *
 * Revision 1.3  1999/04/26 06:14:46  craigs
 * Initial implementation for RTP decoding and lots of stuff
 * As a whole, these changes are called "First Noise"
 *
 * Revision 1.2  1999/02/25 03:26:02  robertj
 * BeOS compatibility
 *
 * Revision 1.1  1999/01/16 01:31:09  robertj
 * Initial revision
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "channels.h"
#endif

#include <h323/channels.h>

#include <opal/transports.h>

#if OPAL_VIDEO
#include <codec/vidcodec.h>
#endif

#include <h323/h323ep.h>
#include <h323/h323con.h>
#include <h323/h323rtp.h>
#include <h323/transaddr.h>
#include <h323/h323pdu.h>


#define new PNEW


#define	MAX_PAYLOAD_TYPE_MISMATCHES 8
#define RTP_TRACE_DISPLAY_RATE 16000 // 2 seconds


#if PTRACING

ostream & operator<<(ostream & out, H323Channel::Directions dir)
{
  static const char * const DirNames[H323Channel::NumDirections] = {
    "IsBidirectional", "IsTransmitter", "IsReceiver"
  };

  if (dir < H323Channel::NumDirections && DirNames[dir] != NULL)
    out << DirNames[dir];
  else
    out << "Direction<" << (unsigned)dir << '>';

  return out;
}

#endif


/////////////////////////////////////////////////////////////////////////////

H323ChannelNumber::H323ChannelNumber(unsigned num, PBoolean fromRem)
{
  PAssert(num < 0x10000, PInvalidParameter);
  number = num;
  fromRemote = fromRem;
}


PObject * H323ChannelNumber::Clone() const
{
  return new H323ChannelNumber(number, fromRemote);
}


PINDEX H323ChannelNumber::HashFunction() const
{
  PINDEX hash = (number%17) << 1;
  if (fromRemote)
    hash++;
  return hash;
}


void H323ChannelNumber::PrintOn(ostream & strm) const
{
  strm << (fromRemote ? 'R' : 'T') << '-' << number;
}


PObject::Comparison H323ChannelNumber::Compare(const PObject & obj) const
{
#ifndef PASN_LEANANDMEAN
  PAssert(PIsDescendant(&obj, H323ChannelNumber), PInvalidCast);
#endif
  const H323ChannelNumber & other = (const H323ChannelNumber &)obj;
  if (number < other.number)
    return LessThan;
  if (number > other.number)
    return GreaterThan;
  if (fromRemote && !other.fromRemote)
    return LessThan;
  if (!fromRemote && other.fromRemote)
    return GreaterThan;
  return EqualTo;
}


H323ChannelNumber & H323ChannelNumber::operator++(int)
{
  number++;
  return *this;
}


/////////////////////////////////////////////////////////////////////////////

H323Channel::H323Channel(H323Connection & conn, const H323Capability & cap)
  : endpoint(conn.GetEndPoint()),
    connection(conn)
{
  capability = (H323Capability *)cap.Clone();
  bandwidthUsed = 0;
  terminating = PFalse;
  opened = PFalse;
  paused = PTrue;
}


H323Channel::~H323Channel()
{
  connection.SetBandwidthUsed(bandwidthUsed, 0);

  delete capability;
}


void H323Channel::PrintOn(ostream & strm) const
{
  strm << number;
}


unsigned H323Channel::GetSessionID() const
{
  return 0;
}


PBoolean H323Channel::GetMediaTransportAddress(OpalTransportAddress & /*data*/,
                                           OpalTransportAddress & /*control*/) const
{
  return PFalse;
}


void H323Channel::Close()
{
  if (!opened || terminating)
    return;

  terminating = PTrue;

  // Signal to the connection that this channel is on the way out
  connection.OnClosedLogicalChannel(*this);

  PTRACE(4, "LogChan\tCleaned up " << number);
}


PBoolean H323Channel::OnReceivedPDU(const H245_OpenLogicalChannel & /*pdu*/,
                                unsigned & /*errorCode*/)
{
  return PTrue;
}


PBoolean H323Channel::OnReceivedAckPDU(const H245_OpenLogicalChannelAck & /*pdu*/)
{
  return PTrue;
}


void H323Channel::OnSendOpenAck(const H245_OpenLogicalChannel & /*pdu*/,
                                H245_OpenLogicalChannelAck & /* pdu*/) const
{
}


void H323Channel::OnFlowControl(long PTRACE_PARAM(bitRateRestriction))
{
  PTRACE(3, "LogChan\tOnFlowControl: " << bitRateRestriction);
}


void H323Channel::OnMiscellaneousCommand(const H245_MiscellaneousCommand_type & type)
{
  PTRACE(3, "LogChan\tOnMiscellaneousCommand: chan=" << number << ", type=" << type.GetTagName());
  OpalMediaStream * mediaStream = GetMediaStream();
  if (mediaStream == NULL)
    return;

  switch (type.GetTag()) {
#if OPAL_VIDEO
    case H245_MiscellaneousCommand_type::e_videoFastUpdatePicture :
      mediaStream->ExecuteCommand(OpalVideoUpdatePicture());
      break;

    case H245_MiscellaneousCommand_type::e_videoFastUpdateGOB :
      {
        const H245_MiscellaneousCommand_type_videoFastUpdateGOB & vfuGOB = type;
        mediaStream->ExecuteCommand(OpalVideoUpdatePicture(vfuGOB.m_firstGOB, -1, vfuGOB.m_numberOfGOBs));
      }
      break;

    case H245_MiscellaneousCommand_type::e_videoFastUpdateMB :
      {
        const H245_MiscellaneousCommand_type_videoFastUpdateMB & vfuMB = type;
        mediaStream->ExecuteCommand(OpalVideoUpdatePicture(vfuMB.m_firstGOB, vfuMB.m_firstMB, vfuMB.m_numberOfMBs));
      }
      break;
#endif
  }
}


void H323Channel::OnMiscellaneousIndication(const H245_MiscellaneousIndication_type & PTRACE_PARAM(type))
{
  PTRACE(3, "LogChan\tOnMiscellaneousIndication: chan=" << number
         << ", type=" << type.GetTagName());
}


void H323Channel::OnJitterIndication(DWORD PTRACE_PARAM(jitter),
                                     int   PTRACE_PARAM(skippedFrameCount),
                                     int   PTRACE_PARAM(additionalBuffer))
{
  PTRACE(3, "LogChan\tOnJitterIndication:"
            " jitter=" << jitter <<
            " skippedFrameCount=" << skippedFrameCount <<
            " additionalBuffer=" << additionalBuffer);
}


PBoolean H323Channel::SetBandwidthUsed(unsigned bandwidth)
{
  PTRACE(3, "LogChan\tBandwidth requested/used = "
         << bandwidth/10 << '.' << bandwidth%10 << '/'
         << bandwidthUsed/10 << '.' << bandwidthUsed%10
         << " kb/s");
  if (!connection.SetBandwidthUsed(bandwidthUsed, bandwidth)) {
    bandwidthUsed = 0;
    return PFalse;
  }

  bandwidthUsed = bandwidth;
  return PTrue;
}


PBoolean H323Channel::Open()
{
  if (opened)
    return PTrue;

  // Give the connection (or endpoint) a chance to do something with
  // the opening of the codec.
  if (!connection.OnStartLogicalChannel(*this)) {
    PTRACE(1, "LogChan\t" << (GetDirection() == IsReceiver ? "Receive" : "Transmit")
           << " open failed (OnStartLogicalChannel fail)");
    return PFalse;
  }

  opened = PTrue;
  return PTrue;
}


/////////////////////////////////////////////////////////////////////////////

H323UnidirectionalChannel::H323UnidirectionalChannel(H323Connection & conn,
                                                     const H323Capability & cap,
                                                     Directions direction)
  : H323Channel(conn, cap),
    receiver(direction == IsReceiver)
{
  mediaStream = NULL;
}

H323UnidirectionalChannel::~H323UnidirectionalChannel()
{
  if (!connection.RemoveMediaStream(mediaStream)) {
    delete mediaStream;
    mediaStream = NULL;
  }
}


H323Channel::Directions H323UnidirectionalChannel::GetDirection() const
{
  return receiver ? IsReceiver : IsTransmitter;
}


PBoolean H323UnidirectionalChannel::SetInitialBandwidth()
{
  return SetBandwidthUsed(capability->GetMediaFormat().GetBandwidth()/100);
}


PBoolean H323UnidirectionalChannel::Open()
{
  if (opened)
    return PTrue;

  if (PAssertNULL(mediaStream) == NULL)
    return PFalse;

  if (!mediaStream->Open()) {
    PTRACE(1, "LogChan\t" << (GetDirection() == IsReceiver ? "Receive" : "Transmit")
           << " open failed (OpalMediaStream::Open fail)");
    return PFalse;
  }

  if (!H323Channel::Open())
    return PFalse;

  if (!mediaStream->IsSource())
    return PTrue;

  return connection.OnOpenMediaStream(*mediaStream);
}


PBoolean H323UnidirectionalChannel::Start()
{
  if (!Open())
    return PFalse;

  if (!mediaStream->Start())
    return PFalse;

  //mediaStream->SetCommandNotifier(PCREATE_NOTIFIER(OnMediaCommand));  // TODO: HERE

  paused = PFalse;
  return PTrue;
}


void H323UnidirectionalChannel::Close()
{
  if (terminating)
    return;

  PTRACE(4, "H323RTP\tCleaning up media stream on " << number);

  // If we have source media stream close it
  if (mediaStream != NULL)
    mediaStream->Close();

  H323Channel::Close();
}


void H323UnidirectionalChannel::OnMiscellaneousCommand(const H245_MiscellaneousCommand_type & type)
{
  H323Channel::OnMiscellaneousCommand(type);

  if (mediaStream == NULL)
    return;

#if OPAL_VIDEO
  switch (type.GetTag())
  {
    case H245_MiscellaneousCommand_type::e_videoFreezePicture :
      mediaStream->ExecuteCommand(OpalVideoFreezePicture());
      break;

    case H245_MiscellaneousCommand_type::e_videoFastUpdatePicture:
      mediaStream->ExecuteCommand(OpalVideoUpdatePicture());
      break;

    case H245_MiscellaneousCommand_type::e_videoFastUpdateGOB :
    {
      const H245_MiscellaneousCommand_type_videoFastUpdateGOB & fuGOB = type;
      mediaStream->ExecuteCommand(OpalVideoUpdatePicture(fuGOB.m_firstGOB, -1, fuGOB.m_numberOfGOBs));
    }
    break;

    case H245_MiscellaneousCommand_type::e_videoFastUpdateMB :
    {
      const H245_MiscellaneousCommand_type_videoFastUpdateMB & vfuMB = type;
      mediaStream->ExecuteCommand(OpalVideoUpdatePicture(vfuMB.HasOptionalField(H245_MiscellaneousCommand_type_videoFastUpdateMB::e_firstGOB) ? (int)vfuMB.m_firstGOB : -1,
                                                         vfuMB.HasOptionalField(H245_MiscellaneousCommand_type_videoFastUpdateMB::e_firstMB)  ? (int)vfuMB.m_firstMB  : -1,
                                                         vfuMB.m_numberOfMBs));
    }
    break;

    case H245_MiscellaneousCommand_type::e_videoTemporalSpatialTradeOff :
      mediaStream->ExecuteCommand(OpalTemporalSpatialTradeOff((const PASN_Integer &)type));
      break;
    default:
      break;
  }
#endif
}


void H323UnidirectionalChannel::OnMediaCommand(
#if OPAL_VIDEO
  OpalMediaCommand & command, 
#else
  OpalMediaCommand & , 
#endif
  INT)
{
#if OPAL_VIDEO
  if (PIsDescendant(&command, OpalVideoUpdatePicture)) {
    H323ControlPDU pdu;
    const OpalVideoUpdatePicture & updatePicture = (const OpalVideoUpdatePicture &)command;

    if (updatePicture.GetNumBlocks() < 0)
      pdu.BuildMiscellaneousCommand(GetNumber(), H245_MiscellaneousCommand_type::e_videoFastUpdatePicture);
    else if (updatePicture.GetFirstMB() < 0) {
      H245_MiscellaneousCommand_type_videoFastUpdateGOB & fuGOB = 
            pdu.BuildMiscellaneousCommand(GetNumber(), H245_MiscellaneousCommand_type::e_videoFastUpdateGOB).m_type;
      fuGOB.m_firstGOB = updatePicture.GetFirstGOB();
      fuGOB.m_numberOfGOBs = updatePicture.GetNumBlocks();
    }
    else {
      H245_MiscellaneousCommand_type_videoFastUpdateMB & vfuMB =
            pdu.BuildMiscellaneousCommand(GetNumber(), H245_MiscellaneousCommand_type::e_videoFastUpdateMB).m_type;
      if (updatePicture.GetFirstGOB() >= 0) {
        vfuMB.IncludeOptionalField(H245_MiscellaneousCommand_type_videoFastUpdateMB::e_firstGOB);
        vfuMB.m_firstGOB = updatePicture.GetFirstGOB();
      }
      if (updatePicture.GetFirstMB() >= 0) {
        vfuMB.IncludeOptionalField(H245_MiscellaneousCommand_type_videoFastUpdateMB::e_firstMB);
        vfuMB.m_firstMB = updatePicture.GetFirstMB();
      }
      vfuMB.m_numberOfMBs = updatePicture.GetNumBlocks();
    }

    connection.WriteControlPDU(pdu);
    return;
  }
#endif
}


OpalMediaStream * H323UnidirectionalChannel::GetMediaStream(PBoolean deleted) const
{
  OpalMediaStream * t = mediaStream;
  if (deleted)
    mediaStream = NULL;
  return t;
}


/////////////////////////////////////////////////////////////////////////////

H323BidirectionalChannel::H323BidirectionalChannel(H323Connection & conn,
                                                   const H323Capability & cap)
  : H323Channel(conn, cap)
{
}


H323Channel::Directions H323BidirectionalChannel::GetDirection() const
{
  return IsBidirectional;
}


PBoolean H323BidirectionalChannel::Start()
{
  return PTrue;
}


/////////////////////////////////////////////////////////////////////////////

H323_RealTimeChannel::H323_RealTimeChannel(H323Connection & connection,
                                           const H323Capability & capability,
                                           Directions direction)
  : H323UnidirectionalChannel(connection, capability, direction)
{
  rtpPayloadType = capability.GetMediaFormat().GetPayloadType();
}


PBoolean H323_RealTimeChannel::OnSendingPDU(H245_OpenLogicalChannel & open) const
{
  PTRACE(3, "H323RTP\tOnSendingPDU");

  open.m_forwardLogicalChannelNumber = (unsigned)number;

  if (open.HasOptionalField(H245_OpenLogicalChannel::e_reverseLogicalChannelParameters)) {
    open.m_reverseLogicalChannelParameters.IncludeOptionalField(
            H245_OpenLogicalChannel_reverseLogicalChannelParameters::e_multiplexParameters);
    // Set the communications information for unicast IPv4
    open.m_reverseLogicalChannelParameters.m_multiplexParameters.SetTag(
                H245_OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters
                    ::e_h2250LogicalChannelParameters);

    return OnSendingPDU(open.m_reverseLogicalChannelParameters.m_multiplexParameters);
  }
  else {
    // Set the communications information for unicast IPv4
    open.m_forwardLogicalChannelParameters.m_multiplexParameters.SetTag(
                H245_OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters
                    ::e_h2250LogicalChannelParameters);

    return OnSendingPDU(open.m_forwardLogicalChannelParameters.m_multiplexParameters);
  }
}


void H323_RealTimeChannel::OnSendOpenAck(const H245_OpenLogicalChannel & open,
                                         H245_OpenLogicalChannelAck & ack) const
{
  PTRACE(3, "H323RTP\tOnSendOpenAck");

  // set forwardMultiplexAckParameters option
  ack.IncludeOptionalField(H245_OpenLogicalChannelAck::e_forwardMultiplexAckParameters);

  // select H225 choice
  ack.m_forwardMultiplexAckParameters.SetTag(
	  H245_OpenLogicalChannelAck_forwardMultiplexAckParameters::e_h2250LogicalChannelAckParameters);

  // get H225 parms
  H245_H2250LogicalChannelAckParameters & param = ack.m_forwardMultiplexAckParameters;

  // set session ID
  param.IncludeOptionalField(H245_H2250LogicalChannelAckParameters::e_sessionID);
  const H245_H2250LogicalChannelParameters & openparam =
                          open.m_forwardLogicalChannelParameters.m_multiplexParameters;
  unsigned sessionID = openparam.m_sessionID;
  param.m_sessionID = sessionID;

  OnSendOpenAck(param);

  PTRACE(3, "H323RTP\tSending open logical channel ACK: sessionID=" << sessionID);
}


PBoolean H323_RealTimeChannel::OnReceivedPDU(const H245_OpenLogicalChannel & open,
                                         unsigned & errorCode)
{
  if (receiver)
    number = H323ChannelNumber(open.m_forwardLogicalChannelNumber, PTrue);

  PTRACE(3, "H323RTP\tOnReceivedPDU for channel: " << number);

  PBoolean reverse = open.HasOptionalField(H245_OpenLogicalChannel::e_reverseLogicalChannelParameters);
  const H245_DataType & dataType = reverse ? open.m_reverseLogicalChannelParameters.m_dataType
                                           : open.m_forwardLogicalChannelParameters.m_dataType;

  if (!capability->OnReceivedPDU(dataType, receiver)) {
    PTRACE(1, "H323RTP\tData type not supported");
    errorCode = H245_OpenLogicalChannelReject_cause::e_dataTypeNotSupported;
    return PFalse;
  }

  if (reverse) {
    if (open.m_reverseLogicalChannelParameters.m_multiplexParameters.GetTag() ==
             H245_OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters::e_h2250LogicalChannelParameters)
      return OnReceivedPDU(open.m_reverseLogicalChannelParameters.m_multiplexParameters, errorCode);
  }
  else {
    if (open.m_forwardLogicalChannelParameters.m_multiplexParameters.GetTag() ==
             H245_OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters::e_h2250LogicalChannelParameters)
      return OnReceivedPDU(open.m_forwardLogicalChannelParameters.m_multiplexParameters, errorCode);
  }

  PTRACE(1, "H323RTP\tOnly H.225.0 multiplex supported");
  errorCode = H245_OpenLogicalChannelReject_cause::e_unsuitableReverseParameters;
  return PFalse;
}


PBoolean H323_RealTimeChannel::OnReceivedAckPDU(const H245_OpenLogicalChannelAck & ack)
{
  PTRACE(3, "H323RTP\tOnReceiveOpenAck");

  if (!ack.HasOptionalField(H245_OpenLogicalChannelAck::e_forwardMultiplexAckParameters)) {
    PTRACE(1, "H323RTP\tNo forwardMultiplexAckParameters");
    return PFalse;
  }

  if (ack.m_forwardMultiplexAckParameters.GetTag() !=
            H245_OpenLogicalChannelAck_forwardMultiplexAckParameters::e_h2250LogicalChannelAckParameters) {
    PTRACE(1, "H323RTP\tOnly H.225.0 multiplex supported");
    return PFalse;
  }

  return OnReceivedAckPDU(ack.m_forwardMultiplexAckParameters);
}


PBoolean H323_RealTimeChannel::SetDynamicRTPPayloadType(int newType)
{
  PTRACE(4, "H323RTP\tAttempting to set dynamic RTP payload type: " << newType);

  // This is "no change"
  if (newType == -1)
    return PTrue;

  // Check for illegal type
  if (newType < RTP_DataFrame::DynamicBase || newType >= RTP_DataFrame::IllegalPayloadType)
    return PFalse;

  // Check for overwriting "known" type
  if (rtpPayloadType < RTP_DataFrame::DynamicBase)
    return PFalse;

  rtpPayloadType = (RTP_DataFrame::PayloadTypes)newType;
  PTRACE(3, "H323RTP\tSet dynamic payload type to " << rtpPayloadType);
  return PTrue;
}


/////////////////////////////////////////////////////////////////////////////

H323_RTPChannel::H323_RTPChannel(H323Connection & conn,
                                 const H323Capability & cap,
                                 Directions direction,
                                 RTP_Session & r)
  : H323_RealTimeChannel(conn, cap, direction),
    rtpSession(r),
    rtpCallbacks(*(H323_RTP_Session *)r.GetUserData())
{
  // If we are the receiver of RTP data then we create a source media stream
  mediaStream = conn.CreateMediaStream(capability->GetMediaFormat(), GetSessionID(), receiver);
  PTRACE(3, "H323RTP\t" << (receiver ? "Receiver" : "Transmitter")
         << " created using session " << GetSessionID());
}


H323_RTPChannel::~H323_RTPChannel()
{
  // Finished with the RTP session, this will delete the session if it is no
  // longer referenced by any logical channels.
  connection.ReleaseSession(GetSessionID());
}


unsigned H323_RTPChannel::GetSessionID() const
{
  return rtpSession.GetSessionID();
}


PBoolean H323_RTPChannel::OnSendingPDU(H245_H2250LogicalChannelParameters & param) const
{
  return rtpCallbacks.OnSendingPDU(*this, param);
}


void H323_RTPChannel::OnSendOpenAck(H245_H2250LogicalChannelAckParameters & param) const
{
  rtpCallbacks.OnSendingAckPDU(*this, param);
}


PBoolean H323_RTPChannel::OnReceivedPDU(const H245_H2250LogicalChannelParameters & param,
                                    unsigned & errorCode)
{
  return rtpCallbacks.OnReceivedPDU(*this, param, errorCode);
}


PBoolean H323_RTPChannel::OnReceivedAckPDU(const H245_H2250LogicalChannelAckParameters & param)
{
  return rtpCallbacks.OnReceivedAckPDU(*this, param);
}


/////////////////////////////////////////////////////////////////////////////

H323_ExternalRTPChannel::H323_ExternalRTPChannel(H323Connection & connection,
                                                 const H323Capability & capability,
                                                 Directions direction,
                                                 unsigned id)
  : H323_RealTimeChannel(connection, capability, direction)
{
  Construct(connection, id);
}


H323_ExternalRTPChannel::H323_ExternalRTPChannel(H323Connection & connection,
                                                 const H323Capability & capability,
                                                 Directions direction,
                                                 unsigned id,
                                                 const H323TransportAddress & data,
                                                 const H323TransportAddress & control)
  : H323_RealTimeChannel(connection, capability, direction),
    externalMediaAddress(data),
    externalMediaControlAddress(control)
{
  Construct(connection, id);
}


H323_ExternalRTPChannel::H323_ExternalRTPChannel(H323Connection & connection,
                                                 const H323Capability & capability,
                                                 Directions direction,
                                                 unsigned id,
                                                 const PIPSocket::Address & ip,
                                                 WORD dataPort)
  : H323_RealTimeChannel(connection, capability, direction),
    externalMediaAddress(ip, dataPort),
    externalMediaControlAddress(ip, (WORD)(dataPort+1))
{
  Construct(connection, id);
}

void H323_ExternalRTPChannel::Construct(H323Connection & conn, unsigned id)
{
  mediaStream = new OpalNullMediaStream(conn, capability->GetMediaFormat(), id, receiver);
  sessionID = id;

  PTRACE(3, "H323RTP\tExternal " << (receiver ? "receiver" : "transmitter")
         << " created using session " << GetSessionID());
}


unsigned H323_ExternalRTPChannel::GetSessionID() const
{
  return sessionID;
}


PBoolean H323_ExternalRTPChannel::GetMediaTransportAddress(OpalTransportAddress & data,
                                                       OpalTransportAddress & control) const
{
  data = remoteMediaAddress;
  control = remoteMediaControlAddress;

  if (data.IsEmpty() && control.IsEmpty())
    return PFalse;

  PIPSocket::Address ip;
  WORD port;
  if (data.IsEmpty()) {
    if (control.GetIpAndPort(ip, port))
      data = OpalTransportAddress(ip, (WORD)(port-1));
  }
  else if (control.IsEmpty()) {
    if (data.GetIpAndPort(ip, port))
      control = OpalTransportAddress(ip, (WORD)(port+1));
  }

  return PTrue;
}


PBoolean H323_ExternalRTPChannel::Start()
{
  PSafePtr<OpalConnection> otherParty = connection.GetCall().GetOtherPartyConnection(connection);
  if (otherParty == NULL)
    return PFalse;

  OpalConnection::MediaInformation info;
  if (!otherParty->GetMediaInformation(sessionID, info))
    return PFalse;

  externalMediaAddress = info.data;
  externalMediaControlAddress = info.control;
  return Open();
}


void H323_ExternalRTPChannel::Receive()
{
  // Do nothing
}


void H323_ExternalRTPChannel::Transmit()
{
  // Do nothing
}


PBoolean H323_ExternalRTPChannel::OnSendingPDU(H245_H2250LogicalChannelParameters & param) const
{
  param.m_sessionID = sessionID;

  param.IncludeOptionalField(H245_H2250LogicalChannelParameters::e_mediaGuaranteedDelivery);
  param.m_mediaGuaranteedDelivery = PFalse;

  param.IncludeOptionalField(H245_H2250LogicalChannelParameters::e_silenceSuppression);
  param.m_silenceSuppression = PFalse;

  // unicast must have mediaControlChannel
  param.IncludeOptionalField(H245_H2250LogicalChannelParameters::e_mediaControlChannel);
  externalMediaControlAddress.SetPDU(param.m_mediaControlChannel);

  if (receiver) {
    // set mediaChannel
    param.IncludeOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaChannel);
    externalMediaAddress.SetPDU(param.m_mediaChannel);
  }

  return PTrue;
}


void H323_ExternalRTPChannel::OnSendOpenAck(H245_H2250LogicalChannelAckParameters & param) const
{
  // set mediaControlChannel
  param.IncludeOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaControlChannel);
  externalMediaControlAddress.SetPDU(param.m_mediaControlChannel);

  // set mediaChannel
  param.IncludeOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaChannel);
  externalMediaAddress.SetPDU(param.m_mediaChannel);
}


PBoolean H323_ExternalRTPChannel::OnReceivedPDU(const H245_H2250LogicalChannelParameters & param,
                                          unsigned & errorCode)
{
  // Only support a single audio session
  if (param.m_sessionID != sessionID) {
    PTRACE(1, "LogChan\tOpen for invalid session: " << param.m_sessionID);
    errorCode = H245_OpenLogicalChannelReject_cause::e_invalidSessionID;
    return PFalse;
  }

  if (!param.HasOptionalField(H245_H2250LogicalChannelParameters::e_mediaControlChannel)) {
    PTRACE(1, "LogChan\tNo mediaControlChannel specified");
    errorCode = H245_OpenLogicalChannelReject_cause::e_unspecified;
    return PFalse;
  }

  remoteMediaControlAddress = param.m_mediaControlChannel;
  if (remoteMediaControlAddress.IsEmpty())
    return PFalse;

  if (param.HasOptionalField(H245_H2250LogicalChannelParameters::e_mediaChannel)) {
    remoteMediaAddress = param.m_mediaChannel;
    if (remoteMediaAddress.IsEmpty())
      return PFalse;
  }
  else {
    PIPSocket::Address addr;
    WORD port;
    if (!remoteMediaControlAddress.GetIpAndPort(addr, port))
      return PFalse;
    remoteMediaAddress = OpalTransportAddress(addr, port-1);
  }

  unsigned id = param.m_sessionID;
  if (!remoteMediaAddress.IsEmpty() && (connection.GetMediaTransportAddresses().GetAt(id) == NULL))
    connection.GetMediaTransportAddresses().SetAt(id, new OpalTransportAddress(remoteMediaAddress));

  return PTrue;
}


PBoolean H323_ExternalRTPChannel::OnReceivedAckPDU(const H245_H2250LogicalChannelAckParameters & param)
{
   if (param.HasOptionalField(H245_H2250LogicalChannelAckParameters::e_sessionID) && (param.m_sessionID != sessionID)) {
     PTRACE(2, "LogChan\tAck for invalid session: " << param.m_sessionID);
  }


  if (!param.HasOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaControlChannel)) {
    PTRACE(1, "LogChan\tNo mediaControlChannel specified");
    return PFalse;
  }

  remoteMediaControlAddress = param.m_mediaControlChannel;
  if (remoteMediaControlAddress.IsEmpty())
    return PFalse;

  if (!param.HasOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaChannel)) {
    PTRACE(1, "LogChan\tNo mediaChannel specified");
    return PFalse;
  }

  remoteMediaAddress = param.m_mediaChannel;
  if (remoteMediaAddress.IsEmpty())
    return PFalse;

  unsigned id = param.m_sessionID;
  if (!remoteMediaAddress.IsEmpty() && (connection.GetMediaTransportAddresses().GetAt(id) == NULL))
    connection.GetMediaTransportAddresses().SetAt(id, new OpalTransportAddress(remoteMediaAddress));

  return PTrue;
}


void H323_ExternalRTPChannel::SetExternalAddress(const H323TransportAddress & data,
                                                 const H323TransportAddress & control)
{
  externalMediaAddress = data;
  externalMediaControlAddress = control;

  if (data.IsEmpty() || control.IsEmpty()) {
    PIPSocket::Address ip;
    WORD port;
    if (data.GetIpAndPort(ip, port))
      externalMediaControlAddress = H323TransportAddress(ip, (WORD)(port+1));
    else if (control.GetIpAndPort(ip, port))
      externalMediaAddress = H323TransportAddress(ip, (WORD)(port-1));
  }
}


PBoolean H323_ExternalRTPChannel::GetRemoteAddress(PIPSocket::Address & ip,
                                               WORD & dataPort) const
{
  if (!remoteMediaControlAddress) {
    if (remoteMediaControlAddress.GetIpAndPort(ip, dataPort)) {
      dataPort--;
      return PTrue;
    }
  }

  if (!remoteMediaAddress)
    return remoteMediaAddress.GetIpAndPort(ip, dataPort);

  return PFalse;
}


/////////////////////////////////////////////////////////////////////////////

H323DataChannel::H323DataChannel(H323Connection & conn,
                                 const H323Capability & cap,
                                 Directions dir,
                                 unsigned id)
  : H323UnidirectionalChannel(conn, cap, dir)
{
  sessionID = id;
  listener = NULL;
  autoDeleteListener = PTrue;
  transport = NULL;
  autoDeleteTransport = PTrue;
  separateReverseChannel = PFalse;
}


H323DataChannel::~H323DataChannel()
{
  if (autoDeleteListener)
    delete listener;
  if (autoDeleteTransport)
    delete transport;
}


void H323DataChannel::Close()
{
  if (terminating)
    return;

  PTRACE(4, "LogChan\tCleaning up data channel " << number);

  // Break any I/O blocks and wait for the thread that uses this object to
  // terminate before we allow it to be deleted.
  if (listener != NULL)
    listener->Close();
  if (transport != NULL)
    transport->Close();

  H323UnidirectionalChannel::Close();
}


unsigned H323DataChannel::GetSessionID() const
{
  return sessionID;
}


PBoolean H323DataChannel::OnSendingPDU(H245_OpenLogicalChannel & open) const
{
  PTRACE(3, "LogChan\tOnSendingPDU for channel: " << number);

  open.m_forwardLogicalChannelNumber = (unsigned)number;

  open.m_forwardLogicalChannelParameters.m_multiplexParameters.SetTag(
              H245_OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters
                  ::e_h2250LogicalChannelParameters);
  H245_H2250LogicalChannelParameters & fparam = open.m_forwardLogicalChannelParameters.m_multiplexParameters;
  fparam.m_sessionID = GetSessionID();

  if (separateReverseChannel)
    return PTrue;

  open.IncludeOptionalField(H245_OpenLogicalChannel::e_reverseLogicalChannelParameters);
  open.m_reverseLogicalChannelParameters.IncludeOptionalField(
              H245_OpenLogicalChannel_reverseLogicalChannelParameters::e_multiplexParameters);
  open.m_reverseLogicalChannelParameters.m_multiplexParameters.SetTag(
              H245_OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters
                  ::e_h2250LogicalChannelParameters);
  H245_H2250LogicalChannelParameters & rparam = open.m_reverseLogicalChannelParameters.m_multiplexParameters;
  rparam.m_sessionID = GetSessionID();

  return capability->OnSendingPDU(open.m_reverseLogicalChannelParameters.m_dataType);
}


void H323DataChannel::OnSendOpenAck(const H245_OpenLogicalChannel & /*open*/,
                                    H245_OpenLogicalChannelAck & ack) const
{
  if (listener == NULL && transport == NULL) {
    PTRACE(2, "LogChan\tOnSendOpenAck without a listener or transport");
    return;
  }

  PTRACE(3, "LogChan\tOnSendOpenAck for channel: " << number);

  H245_H2250LogicalChannelAckParameters * param;

  if (separateReverseChannel) {
    ack.IncludeOptionalField(H245_OpenLogicalChannelAck::e_forwardMultiplexAckParameters);
    ack.m_forwardMultiplexAckParameters.SetTag(
              H245_OpenLogicalChannelAck_forwardMultiplexAckParameters::e_h2250LogicalChannelAckParameters);
    param = (H245_H2250LogicalChannelAckParameters*)&ack.m_forwardMultiplexAckParameters.GetObject();
  }
  else {
    ack.IncludeOptionalField(H245_OpenLogicalChannelAck::e_reverseLogicalChannelParameters);
    ack.m_reverseLogicalChannelParameters.m_multiplexParameters.SetTag(
              H245_OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters
                  ::e_h2250LogicalChannelParameters);
    param = (H245_H2250LogicalChannelAckParameters*)
                &ack.m_reverseLogicalChannelParameters.m_multiplexParameters.GetObject();
  }

  unsigned session = GetSessionID();
  if (session != 0) {
    param->IncludeOptionalField(H245_H2250LogicalChannelAckParameters::e_sessionID);
    param->m_sessionID = GetSessionID();
  }

  H323TransportAddress address;
  param->IncludeOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaChannel);
  if (listener != NULL)
    address = listener->GetLocalAddress(connection.GetControlChannel().GetLocalAddress());
  else
    address = transport->GetLocalAddress();

  address.SetPDU(param->m_mediaChannel);
}


PBoolean H323DataChannel::OnReceivedPDU(const H245_OpenLogicalChannel & open,
                                    unsigned & errorCode)
{
  number = H323ChannelNumber(open.m_forwardLogicalChannelNumber, PTrue);

  PTRACE(3, "LogChan\tOnReceivedPDU for data channel: " << number);

  if (!CreateListener()) {
    PTRACE(1, "LogChan\tCould not create listener");
    errorCode = H245_OpenLogicalChannelReject_cause::e_unspecified;
    return PFalse;
  }

  if (separateReverseChannel &&
      open.HasOptionalField(H245_OpenLogicalChannel::e_reverseLogicalChannelParameters)) {
    errorCode = H245_OpenLogicalChannelReject_cause::e_unsuitableReverseParameters;
    PTRACE(1, "LogChan\tOnReceivedPDU has unexpected reverse parameters");
    return PFalse;
  }

  if (!capability->OnReceivedPDU(open.m_forwardLogicalChannelParameters.m_dataType, receiver)) {
    PTRACE(1, "H323RTP\tData type not supported");
    errorCode = H245_OpenLogicalChannelReject_cause::e_dataTypeNotSupported;
    return PFalse;
  }

  return PTrue;
}


PBoolean H323DataChannel::OnReceivedAckPDU(const H245_OpenLogicalChannelAck & ack)
{
  PTRACE(3, "LogChan\tOnReceivedAckPDU");

  const H245_TransportAddress * address;

  if (separateReverseChannel) {
      PTRACE(3, "LogChan\tseparateReverseChannels");
    if (!ack.HasOptionalField(H245_OpenLogicalChannelAck::e_forwardMultiplexAckParameters)) {
      PTRACE(1, "LogChan\tNo forwardMultiplexAckParameters");
      return PFalse;
    }

    if (ack.m_forwardMultiplexAckParameters.GetTag() !=
              H245_OpenLogicalChannelAck_forwardMultiplexAckParameters::e_h2250LogicalChannelAckParameters) {
      PTRACE(1, "LogChan\tOnly H.225.0 multiplex supported");
      return PFalse;
    }

    const H245_H2250LogicalChannelAckParameters & param = ack.m_forwardMultiplexAckParameters;

    if (!param.HasOptionalField(H245_H2250LogicalChannelAckParameters::e_mediaChannel)) {
      PTRACE(1, "LogChan\tNo media channel address provided");
      return PFalse;
    }

    address = &param.m_mediaChannel;

    if (ack.HasOptionalField(H245_OpenLogicalChannelAck::e_reverseLogicalChannelParameters)) {
      PTRACE(3, "LogChan\treverseLogicalChannelParameters set");
      reverseChannel = H323ChannelNumber(ack.m_reverseLogicalChannelParameters.m_reverseLogicalChannelNumber, PTrue);
    }
  }
  else {
    if (!ack.HasOptionalField(H245_OpenLogicalChannelAck::e_reverseLogicalChannelParameters)) {
      PTRACE(1, "LogChan\tNo reverseLogicalChannelParameters");
      return PFalse;
    }

    if (ack.m_reverseLogicalChannelParameters.m_multiplexParameters.GetTag() !=
              H245_OpenLogicalChannelAck_reverseLogicalChannelParameters_multiplexParameters
                              ::e_h2250LogicalChannelParameters) {
      PTRACE(1, "LogChan\tOnly H.225.0 multiplex supported");
      return PFalse;
    }

    const H245_H2250LogicalChannelParameters & param = ack.m_reverseLogicalChannelParameters.m_multiplexParameters;

    if (!param.HasOptionalField(H245_H2250LogicalChannelParameters::e_mediaChannel)) {
      PTRACE(1, "LogChan\tNo media channel address provided");
      return PFalse;
    }

    address = &param.m_mediaChannel;
  }

  if (!CreateTransport()) {
    PTRACE(1, "LogChan\tCould not create transport");
    return PFalse;
  }

  if (!transport->ConnectTo(H323TransportAddress(*address))) {
    PTRACE(1, "LogChan\tCould not connect to remote transport address: " << *address);
    return PFalse;
  }

  return PTrue;
}


PBoolean H323DataChannel::CreateListener()
{
  if (listener == NULL) {
    listener = connection.GetControlChannel().GetLocalAddress().CreateListener(
                          connection.GetEndPoint(), OpalTransportAddress::HostOnly);
    if (listener == NULL)
      return PFalse;

    PTRACE(3, "LogChan\tCreated listener for data channel: " << *listener);
  }

  return listener->Open(NULL);
}


PBoolean H323DataChannel::CreateTransport()
{
  if (transport == NULL) {
    transport = connection.GetControlChannel().GetLocalAddress().CreateTransport(
                          connection.GetEndPoint(), OpalTransportAddress::HostOnly);
    if (transport == NULL)
      return PFalse;

    PTRACE(3, "LogChan\tCreated transport for data channel: " << *transport);
  }

  return transport != NULL;
}


/////////////////////////////////////////////////////////////////////////////
