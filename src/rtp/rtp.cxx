/*
 * rtp.cxx
 *
 * RTP protocol handler
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
 * $Log: rtp.cxx,v $
 * Revision 1.2072.2.1  2007/09/13 05:41:39  rjongbloed
 * Merge from HEAD
 *
 * Revision 2.72  2007/09/09 23:37:20  rjongbloed
 * Fixed confusion over MaxPayloadType meaning
 *
 * Revision 2.71  2007/09/05 14:00:32  csoutheren
 * Applied 1783430 - Closing jitter buffer when releasing a session
 * Thanks to Borko Jandras
 *
 * Revision 2.70  2007/09/05 13:06:44  csoutheren
 * Applied 1720918 - Track marker packets sent/received
 * Thanks to Josh Mahonin
 *
 * Revision 2.69  2007/07/26 11:39:38  rjongbloed
 * Removed redundent code
 *
 * Revision 2.68  2007/07/26 00:39:30  csoutheren
 * Make transmission of RFC2833 independent of the media stream
 *
 * Revision 2.67  2007/07/10 06:25:39  csoutheren
 * Allow any number of SSRC changes if ignoreOtherSources is set to false
 *
 * Revision 2.66  2007/05/29 06:26:21  csoutheren
 * Add Reset function so we can reload used control frames
 *
 * Revision 2.65  2007/05/28 08:37:02  csoutheren
 * Protect against sending BYE when RTP session not connected
 *
 * Revision 2.64  2007/05/14 10:44:09  rjongbloed
 * Added PrintOn function to RTP_DataFrame
 *
 * Revision 2.63  2007/04/20 06:57:18  rjongbloed
 * Fixed various log levels
 *
 * Revision 2.62  2007/04/20 04:34:12  csoutheren
 * Only swallow initial RTP packets for audio
 *
 * Revision 2.61  2007/04/19 06:34:12  csoutheren
 * Applied 1703206 - OpalVideoFastUpdatePicture over SIP
 * Thanks to Josh Mahonin
 *
 * Revision 2.60  2007/04/05 02:41:05  rjongbloed
 * Added ability to have non-dynamic allocation of memory in RTP data frames.
 *
 * Revision 2.59  2007/04/04 02:12:01  rjongbloed
 * Reviewed and adjusted PTRACE log levels
 *   Now follows 1=error,2=warn,3=info,4+=debug
 *
 * Revision 2.58  2007/04/02 05:51:34  rjongbloed
 * Tidied some trace logs to assure all have a category (bit before a tab character) set.
 *
 * Revision 2.57  2007/03/29 23:53:39  rjongbloed
 * Fixed log message about swallowing UDP packets so only prints out
 *   if non-zero packets were swallowed.
 *
 * Revision 2.56  2007/03/29 05:07:34  csoutheren
 * Removed annoying comment
 *
 * Revision 2.55  2007/03/28 05:26:11  csoutheren
 * Add virtual function to wait for incoming data
 * Swallow RTP packets that arrive on the socket before session starts
 *
 * Revision 2.54  2007/03/20 02:32:09  csoutheren
 * Don't send BYE twice or when channel is closed
 *
 * Revision 2.53  2007/03/12 23:05:14  csoutheren
 * Fix typo and increase readability
 *
 * Revision 2.52  2007/03/08 04:36:25  csoutheren
 * Add flag to close RTP session when RTCP BYE received
 *
 * Revision 2.51  2007/03/01 03:31:25  csoutheren
 * Remove spurious zero bytes from the end of RTCP SDES packets
 * Fix format of RTCP BYE packets
 *
 * Revision 2.50  2007/02/23 08:06:20  csoutheren
 * More implementation of ZRTP (not yet complete)
 *
 * Revision 2.49  2007/02/20 21:35:45  csoutheren
 * Fixed stupid typo in RTP open
 *
 * Revision 2.48  2007/02/20 04:26:57  csoutheren
 * Ensure outgoing and incoming SSRC are set for SRTP sessions
 * Fixed problem with sending secure RTCP packets
 *
 * Revision 2.47  2007/02/12 02:44:27  csoutheren
 * Start of support for ZRTP
 *
 * Revision 2.47  2007/02/10 07:08:41  craigs
 * Start of support for ZRTP
 *
 * Revision 2.46  2006/12/08 04:12:12  csoutheren
 * Applied 1589274 - better rtp error handling of malformed rtcp packet
 * Thanks to frederich
 *
 * Revision 2.45  2006/11/29 06:31:59  csoutheren
 * Add support fort RTP BYE
 *
 * Revision 2.44  2006/11/20 03:37:13  csoutheren
 * Allow optional inclusion of RTP aggregation
 *
 * Revision 2.43  2006/11/10 12:40:30  csoutheren
 * Fix RTP payload extension size calculation
 * Thanks to Viktor Krikun
 *
 * Revision 2.42  2006/10/28 16:40:28  dsandras
 * Fixed SIP reinvite without breaking H.323 calls.
 *
 * Revision 2.41  2006/10/24 04:18:28  csoutheren
 * Added support for encrypted RTCP
 *
 * Revision 2.40  2006/09/28 07:42:18  csoutheren
 * Merge of useful SRTP implementation
 *
 * Revision 2.39  2006/09/08 05:12:26  csoutheren
 * Reverted previous patch as this breaks H.323 calls
 *
 * Revision 2.38  2006/09/05 06:18:23  csoutheren
 * Start bringing in SRTP code for libSRTP
 *
 * Revision 2.37  2006/08/29 18:39:21  dsandras
 * Added function to better deal with reinvites.
 *
 * Revision 2.36  2006/08/28 00:03:00  csoutheren
 * Applied 1545095 - RTP destructors patch
 * Thanks to Drazen Dimoti
 *
 * Revision 2.35  2006/08/21 05:24:36  csoutheren
 * Add extra trace log to allow tracking of NAT detection
 *
 * Revision 2.34  2006/08/03 04:57:12  csoutheren
 * Port additional NAT handling logic from OpenH323 and extend into OpalConnection class
 *
 * Revision 2.33  2006/07/28 10:41:51  rjongbloed
 * Fixed DevStudio 2005 warnings on time_t conversions.
 *
 * Revision 2.32  2006/06/22 00:26:26  csoutheren
 * Fix formatting
 *
 * Revision 2.31  2006/06/05 20:12:48  dsandras
 * Prevent infinite loop when being unable to write to the control port of the
 * RTP session. Fixes Ekiga #339306.
 *
 * Revision 2.30  2006/04/30 16:41:14  dsandras
 * Allow exactly one sequence change after a call to SetRemoteSocketInfo. Fixes
 * Ekiga bug #339866.
 *
 * Revision 2.29  2006/03/28 11:20:22  dsandras
 * If STUN can not create a socket pair for RTP/RTCP, create separate sockets
 * and continue. At worse, RTCP won't be received. The SIP part could reuse
 * the RTCP port (different from RTP port + 1) in its SDP if required.
 *
 * Revision 2.28  2006/02/02 07:03:44  csoutheren
 * Added extra logging of first outgoing RTP packet
 *
 * Revision 2.27  2006/01/24 23:31:07  dsandras
 * Fixed bug.
 *
 * Revision 2.26  2006/01/22 21:53:23  dsandras
 * Fail when STUN needs to be used and can not be used.
 *
 * Revision 2.25  2005/12/30 14:29:15  dsandras
 * Removed the assumption that the jitter will contain a 8 kHz signal.
 *
 * Revision 2.24  2005/11/29 11:50:20  dsandras
 * Use PWaitAndSignal so that it works even if no AddSession follows the UseSession. Added missing ignoreOtherSources check.
 *
 * Revision 2.23  2005/11/22 22:37:57  dsandras
 * Reverted the previous change as the bug was triggered by a change in PWLIB.
 *
 * Revision 2.22  2005/11/20 20:56:35  dsandras
 * Allow UseSession to be used and fail without requiring a consecutive AddSession.
 *
 * Revision 2.21  2005/10/20 20:27:38  dsandras
 * Better handling of dynamic RTP session IP address changes.
 *
 * Revision 2.20  2005/08/04 17:17:08  dsandras
 * Fixed remote address change of an established RTP Session.
 *
 * Revision 2.19  2005/04/11 17:34:57  dsandras
 * Added support for dynamic sequence changes in case of Re-INVITE.
 *
 * Revision 2.18  2005/04/10 21:17:05  dsandras
 * Added support for remote address and SyncSource changes during an active RTP Session.
 *
 * Revision 2.17  2005/01/16 23:07:35  csoutheren
 * Fixed problem with IPv6 INADDR_ANY
 *
 * Revision 2.16  2004/04/26 05:37:13  rjongbloed
 * Allowed for selectable auto deletion of RTP user data attached to an RTP session.
 *
 * Revision 2.15  2004/03/02 10:02:46  rjongbloed
 * Added check for unusual error in reading UDP socket, thanks Michael Smith
 *
 * Revision 2.14  2004/03/02 09:57:54  rjongbloed
 * Fixed problems with recent changes to RTP UseSession() function which broke it for H.323
 *
 * Revision 2.13  2004/02/19 10:47:06  rjongbloed
 * Merged OpenH323 version 1.13.1 changes.
 *
 * Revision 2.12  2003/01/07 04:39:53  robertj
 * Updated to OpenH323 v1.11.2
 *
 * Revision 2.11  2002/11/10 11:33:20  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.10  2002/10/09 04:28:10  robertj
 * Added session ID to soem trace logs, thanks Ted Szoczei
 *
 * Revision 2.9  2002/09/04 06:01:49  robertj
 * Updated to OpenH323 v1.9.6
 *
 * Revision 2.8  2002/07/01 04:56:33  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 2.7  2002/04/10 03:10:39  robertj
 * Added referential (container) copy functions to session manager class.
 *
 * Revision 2.6  2002/02/11 09:32:13  robertj
 * Updated to openH323 v1.8.0
 *
 * Revision 2.5  2002/01/14 06:35:58  robertj
 * Updated to OpenH323 v1.7.9
 *
 * Revision 2.4  2002/01/14 02:24:06  robertj
 * Fixed possible divide by zero error in statistics.
 *
 * Revision 2.3  2001/12/07 08:52:28  robertj
 * Used new const PWaitAndSignal
 *
 * Revision 2.2  2001/11/14 06:20:40  robertj
 * Changed sending of control channel reports to be timer based.
 *
 * Revision 2.1  2001/10/05 00:22:14  robertj
 * Updated to PWLib 1.2.0 and OpenH323 1.7.0
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.92  2004/02/09 11:17:50  rjongbloed
 * Improved check for compound RTCP packets so does not possibly acess
 *   memory beyond that allocated in packet. Pointed out by Paul Slootman
 *
 * Revision 1.91  2003/10/28 22:35:21  dereksmithies
 * Fix warning about possible use of unitialized variable.
 *
 * Revision 1.90  2003/10/27 06:03:39  csoutheren
 * Added support for QoS
 *   Thanks to Henry Harrison of AliceStreet
 *
 * Revision 1.89  2003/10/09 09:47:45  csoutheren
 * Fixed problem with re-opening RTP half-channels under unusual
 * circumstances. Thanks to Damien Sandras
 *
 * Revision 1.88  2003/05/02 04:57:47  robertj
 * Added header extension support to RTP data frame class.
 *
 * Revision 1.87  2003/02/07 00:30:21  robertj
 * Changes for bizarre usage of RTP code outside of scope of H.323 specs.
 *
 * Revision 1.86  2003/02/04 23:50:06  robertj
 * Changed trace log for RTP session creation to show local address.
 *
 * Revision 1.85  2003/02/04 07:06:42  robertj
 * Added STUN support.
 *
 * Revision 1.84  2003/01/07 06:32:22  robertj
 * Fixed faint possibility of getting an error on UDP write caused by ICMP
 *   unreachable being received when no UDP read is active on the socket. Then
 *   the UDP write gets the error stopping transmission.
 *
 * Revision 1.83  2002/11/19 01:48:00  robertj
 * Allowed get/set of canonical anme and tool name.
 *
 * Revision 1.82  2002/11/05 03:32:04  robertj
 * Added session ID to trace logs.
 * Fixed possible extra wait exiting RTP read loop on close.
 *
 * Revision 1.81  2002/11/05 01:55:50  robertj
 * Added comments about strange mutex usage.
 *
 * Revision 1.80  2002/10/31 00:47:07  robertj
 * Enhanced jitter buffer system so operates dynamically between minimum and
 *   maximum values. Altered API to assure app writers note the change!
 *
 * Revision 1.79  2002/09/26 04:01:49  robertj
 * Fixed calculation of fraction of packets lost in RR, thanks Awais Ali
 *
 * Revision 1.78  2002/09/03 06:15:32  robertj
 * Added copy constructor/operator for session manager.
 *
 * Revision 1.77  2002/08/05 10:03:48  robertj
 * Cosmetic changes to normalise the usage of pragma interface/implementation.
 *
 * Revision 1.76  2002/07/23 06:28:16  robertj
 * Added statistics call back on first sent or received RTP packet, helps with,
 *   for example, detecting if remote endpoint has started to send audio.
 *
 * Revision 1.75  2002/05/28 02:37:55  robertj
 * Fixed reading data out of RTCP compound statements.
 *
 * Revision 1.74  2002/05/21 07:12:39  robertj
 * Fixed 100% CPU usage loop for checkin on sending RTCP data if
 *   have no RTP data transferred at all.
 *
 * Revision 1.73  2002/05/02 05:58:28  robertj
 * Changed the mechanism for sending RTCP reports so that they will continue
 *   to be sent regardless of if there is any actual data traffic.
 * Added support for compound RTCP statements for sender and receiver reports.
 *
 * Revision 1.72  2002/04/18 05:48:58  robertj
 * Fixed problem with new SSRC value being ignored after RTP session has
 *    been restarted, thanks "Jacky".
 *
 * Revision 1.71  2002/02/09 02:33:49  robertj
 * Improved payload type docuemntation and added Cisco CN.
 *
 * Revision 1.70  2001/12/20 04:34:56  robertj
 * Fixed display of some of the unknown RTP types.
 *
 * Revision 1.69  2001/09/12 07:48:05  robertj
 * Fixed various problems with tracing.
 *
 * Revision 1.68  2001/09/11 00:21:24  robertj
 * Fixed missing stack sizes in endpoint for cleaner thread and jitter thread.
 *
 * Revision 1.67  2001/09/10 08:24:04  robertj
 * Fixed setting of destination RTP address so works with endpoints that
 *   do not get the OLC packets quite right.
 *
 * Revision 1.66  2001/07/11 03:23:54  robertj
 * Bug fixed where every 65536 PDUs the session thinks it is the first PDU again.
 *
 * Revision 1.65  2001/07/06 06:32:24  robertj
 * Added flag and checks for RTP data having specific SSRC.
 * Changed transmitter IP address check so is based on first received
 *    PDU instead of expecting it to come from the host we are sending to.
 *
 * Revision 1.64  2001/06/04 13:43:44  robertj
 * Fixed I/O block breaker code if RTP session is bound to INADDR_ANY interface.
 *
 * Revision 1.63  2001/06/04 11:37:50  robertj
 * Added thread safe enumeration functions of RTP sessions.
 * Added member access functions to UDP based RTP sessions.
 *
 * Revision 1.62  2001/05/24 01:12:23  robertj
 * Fixed sender report timestamp field, thanks Johan Gnosspelius
 *
 * Revision 1.61  2001/05/08 05:28:02  yurik
 * No ifdef _WIN32_WCE anymore - 3+ version of  SDK allows it
 *
 * Revision 1.60  2001/04/24 06:15:50  robertj
 * Added work around for strange Cisco bug which suddenly starts sending
 *   RTP packets beginning at a difference sequence number base.
 *
 * Revision 1.59  2001/04/02 23:58:24  robertj
 * Added jitter calculation to RTP session.
 * Added trace of statistics.
 *
 * Revision 1.58  2001/03/20 07:24:05  robertj
 * Changed RTP SDES to have simple host name instead of attempting a
 *   reverse DNS lookup on IP address as badly configured DNS can cause
 *   a 30 second delay before audio is sent.
 *
 * Revision 1.57  2001/03/15 05:45:45  robertj
 * Changed, yet again, the setting of RTP info to allow for Cisco idosyncracies.
 *
 * Revision 1.56  2001/02/21 08:08:26  robertj
 * Added more debugging.
 *
 * Revision 1.55  2001/02/09 05:13:56  craigs
 * Added pragma implementation to (hopefully) reduce the executable image size
 * under Linux
 *
 * Revision 1.54  2001/01/28 07:06:59  yurik
 * WinCE-port - avoid using of IP_TOS
 *
 * Revision 1.53  2000/12/18 08:59:20  craigs
 * Added ability to set ports
 *
 * Revision 1.52  2000/09/25 22:27:40  robertj
 * Fixed uninitialised variable for lastRRSequenceNumber
 *
 * Revision 1.51  2000/09/25 01:53:20  robertj
 * Fixed MSVC warnings.
 *
 * Revision 1.50  2000/09/25 01:44:13  robertj
 * Fixed possible race condition on shutdown of RTP session with jitter buffer.
 *
 * Revision 1.49  2000/09/22 00:32:34  craigs
 * Added extra logging
 * Fixed problems with no fastConnect with tunelling
 *
 * Revision 1.48  2000/09/21 02:06:07  craigs
 * Added handling for endpoints that return conformant, but useless, RTP address
 * and port numbers
 *
 * Revision 1.47  2000/08/17 00:42:39  robertj
 * Fixed RTP Goodbye message reason string parsing, thanks Thien Nguyen.
 *
 * Revision 1.46  2000/05/30 10:35:41  robertj
 * Fixed GNU compiler warning.
 *
 * Revision 1.45  2000/05/30 06:52:26  robertj
 * Fixed problem with Cisco restarting sequence numbers when changing H.323 logical channels.
 *
 * Revision 1.44  2000/05/23 12:57:37  robertj
 * Added ability to change IP Type Of Service code from applications.
 *
 * Revision 1.43  2000/05/12 00:27:35  robertj
 * Fixed bug in UseSession() that caused asserts on BSD and possible race condition everywhere.
 *
 * Revision 1.42  2000/05/04 11:52:35  robertj
 * Added Packets Too Late statistics, requiring major rearrangement of jitter
 *    buffer code, not also changes semantics of codec Write() function slightly.
 *
 * Revision 1.41  2000/05/02 04:32:27  robertj
 * Fixed copyright notice comment.
 *
 * Revision 1.40  2000/05/01 01:01:49  robertj
 * Added flag for what to do with out of orer packets (use if jitter, don't if not).
 *
 * Revision 1.39  2000/04/30 03:55:09  robertj
 * Improved the RTCP messages, epecially reports
 *
 * Revision 1.38  2000/04/28 12:56:39  robertj
 * Fixed transmission of SDES record in RTCP channel.
 *
 * Revision 1.37  2000/04/19 01:50:05  robertj
 * Improved debugging messages.
 *
 * Revision 1.36  2000/04/13 18:07:39  robertj
 * Fixed missing mutex release causing possible deadlocks.
 *
 * Revision 1.35  2000/04/10 17:40:05  robertj
 * Fixed debug output of RTP payload types to allow for unknown numbers.
 *
 * Revision 1.34  2000/04/05 04:09:24  robertj
 * Fixed portability problem with max() macro.
 *
 * Revision 1.33  2000/04/05 03:17:32  robertj
 * Added more RTP statistics gathering and H.245 round trip delay calculation.
 *
 * Revision 1.32  2000/04/03 18:15:44  robertj
 * Added "fractional" part of RTCP status NTP timestamp field.
 *
 * Revision 1.31  2000/03/23 02:54:57  robertj
 * Added sending of SDES control packets.
 *
 * Revision 1.30  2000/03/20 20:53:42  robertj
 * Fixed problem with being able to reopen for reading an RTP_Session (Cisco compatibilty)
 *
 * Revision 1.29  2000/03/04 12:32:23  robertj
 * Added setting of TOS field in IP header to get poor mans QoS on some routers.
 *
 * Revision 1.28  2000/02/29 13:00:13  robertj
 * Added extra statistic display for RTP packets out of order.
 *
 * Revision 1.27  2000/02/29 02:13:56  robertj
 * Fixed RTP receive of both control and data, ignores ECONNRESET/ECONNREFUSED errors.
 *
 * Revision 1.26  2000/02/27 10:56:24  robertj
 * Fixed error in code allowing non-consecutive RTP port numbers coming from broken stacks, thanks Vassili Leonov.
 *
 * Revision 1.25  2000/02/17 12:07:44  robertj
 * Used ne wPWLib random number generator after finding major problem in MSVC rand().
 *
 * Revision 1.24  2000/01/29 07:10:20  robertj
 * Fixed problem with RTP transmit to host that is not yet ready causing the
 *   receive side to die with ECONNRESET, thanks Ian MacDonald
 *
 * Revision 1.23  2000/01/20 05:57:46  robertj
 * Added extra flexibility in receiving incorrectly formed OpenLogicalChannel PDU's
 *
 * Revision 1.22  2000/01/14 00:31:40  robertj
 * Bug fix for RTP port allocation (MSVC optimised version), Thanks to Ian MacDonald.
 *
 * Revision 1.21  1999/12/30 09:14:49  robertj
 * Changed payload type functions to use enum.
 *
 * Revision 1.20  1999/12/23 23:02:36  robertj
 * File reorganision for separating RTP from H.323 and creation of LID for VPB support.
 *
 * Revision 1.19  1999/11/29 04:50:11  robertj
 * Added adaptive threshold calculation to silence detection.
 *
 * Revision 1.18  1999/11/22 00:09:59  robertj
 * Fixed error in RTP transmit rewrite for ternary state, didn't transmit at all!
 *
 * Revision 1.17  1999/11/20 05:35:48  robertj
 * Fixed possibly I/O block in RTP read loops.
 *
 * Revision 1.16  1999/11/19 13:13:31  robertj
 * Fixed Windows 95 compatibility issue in shutdown of RTP reading.
 *
 * Revision 1.15  1999/11/19 10:23:16  robertj
 * Fixed local binding of socket to correct address on multi-homes systems.
 *
 * Revision 1.14  1999/11/19 09:17:15  robertj
 * Fixed problems with aycnhronous shut down of logical channels.
 *
 * Revision 1.13  1999/11/17 03:49:51  robertj
 * Added RTP statistics display.
 *
 * Revision 1.12  1999/11/14 11:41:07  robertj
 * Added access functions to RTP statistics.
 *
 * Revision 1.11  1999/10/18 23:55:11  robertj
 * Fixed bug in setting contributing sources, length put into wrong spot in PDU header
 *
 * Revision 1.10  1999/09/21 14:10:04  robertj
 * Removed warnings when no tracing enabled.
 *
 * Revision 1.9  1999/09/05 00:58:37  robertj
 * Removed requirement that OpenLogicalChannalAck sessionId match original OLC.
 *
 * Revision 1.8  1999/09/03 02:17:50  robertj
 * Added more debugging
 *
 * Revision 1.7  1999/08/31 12:34:19  robertj
 * Added gatekeeper support.
 *
 * Revision 1.6  1999/07/16 02:13:54  robertj
 * Slowed down the control channel statistics packets.
 *
 * Revision 1.5  1999/07/13 09:53:24  robertj
 * Fixed some problems with jitter buffer and added more debugging.
 *
 * Revision 1.4  1999/07/09 06:09:52  robertj
 * Major implementation. An ENORMOUS amount of stuff added everywhere.
 *
 * Revision 1.3  1999/06/22 13:49:40  robertj
 * Added GSM support and further RTP protocol enhancements.
 *
 * Revision 1.2  1999/06/14 08:42:52  robertj
 * Fixed bug in dropped packets display was negative.
 *
 * Revision 1.1  1999/06/14 06:12:25  robertj
 * Changes for using RTP sessions correctly in H323 Logical Channel context
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "rtp.h"
#endif

#include <rtp/rtp.h>

#include <rtp/jitter.h>
#include <ptclib/random.h>
#include <ptclib/pstun.h>
#include <opal/connection.h>

#define new PNEW


const unsigned SecondsFrom1900to1970 = (70*365+17)*24*60*60U;

#define UDP_BUFFER_SIZE 32768

#define MIN_HEADER_SIZE 12

namespace PWLibStupidLinkerHacks {
extern int opalLoader;

static class InstantiateMe
{
  public:
    InstantiateMe()
    { 
      opalLoader = 1; 
    }
} instance;

}; // namespace PWLibStupidLinkerHacks

/////////////////////////////////////////////////////////////////////////////

RTP_DataFrame::RTP_DataFrame(PINDEX sz)
  : PBYTEArray(MIN_HEADER_SIZE+sz)
{
  payloadSize = sz;
  theArray[0] = '\x80';
}


RTP_DataFrame::RTP_DataFrame(const BYTE * data, PINDEX len, BOOL dynamic)
  : PBYTEArray(data, len, dynamic)
{
  payloadSize = len - GetHeaderSize();
}

void RTP_DataFrame::SetExtension(BOOL ext)
{
  if (ext)
    theArray[0] |= 0x10;
  else
    theArray[0] &= 0xef;
}


void RTP_DataFrame::SetMarker(BOOL m)
{
  if (m)
    theArray[1] |= 0x80;
  else
    theArray[1] &= 0x7f;
}


void RTP_DataFrame::SetPayloadType(PayloadTypes t)
{
  PAssert(t <= 0x7f, PInvalidParameter);

  theArray[1] &= 0x80;
  theArray[1] |= t;
}


DWORD RTP_DataFrame::GetContribSource(PINDEX idx) const
{
  PAssert(idx < GetContribSrcCount(), PInvalidParameter);
  return ((PUInt32b *)&theArray[MIN_HEADER_SIZE])[idx];
}


void RTP_DataFrame::SetContribSource(PINDEX idx, DWORD src)
{
  PAssert(idx <= 15, PInvalidParameter);

  if (idx >= GetContribSrcCount()) {
    BYTE * oldPayload = GetPayloadPtr();
    theArray[0] &= 0xf0;
    theArray[0] |= idx+1;
    SetSize(GetHeaderSize()+payloadSize);
    memmove(GetPayloadPtr(), oldPayload, payloadSize);
  }

  ((PUInt32b *)&theArray[MIN_HEADER_SIZE])[idx] = src;
}


PINDEX RTP_DataFrame::GetHeaderSize() const
{
  PINDEX sz = MIN_HEADER_SIZE + 4*GetContribSrcCount();

  if (GetExtension())
    sz += 4 + GetExtensionSize();

  return sz;
}


int RTP_DataFrame::GetExtensionType() const
{
  if (GetExtension())
    return *(PUInt16b *)&theArray[MIN_HEADER_SIZE + 4*GetContribSrcCount()];

  return -1;
}


void RTP_DataFrame::SetExtensionType(int type)
{
  if (type < 0)
    SetExtension(FALSE);
  else {
    if (!GetExtension())
      SetExtensionSize(0);
    *(PUInt16b *)&theArray[MIN_HEADER_SIZE + 4*GetContribSrcCount()] = (WORD)type;
  }
}


PINDEX RTP_DataFrame::GetExtensionSize() const
{
  if (GetExtension())
    return *(PUInt16b *)&theArray[MIN_HEADER_SIZE + 4*GetContribSrcCount() + 2] * 4;

  return 0;
}


BOOL RTP_DataFrame::SetExtensionSize(PINDEX sz)
{
  if (!SetMinSize(MIN_HEADER_SIZE + 4*GetContribSrcCount() + 4+4*sz + payloadSize))
    return FALSE;

  SetExtension(TRUE);
  *(PUInt16b *)&theArray[MIN_HEADER_SIZE + 4*GetContribSrcCount() + 2] = (WORD)sz;
  return TRUE;
}


BYTE * RTP_DataFrame::GetExtensionPtr() const
{
  if (GetExtension())
    return (BYTE *)&theArray[MIN_HEADER_SIZE + 4*GetContribSrcCount() + 4];

  return NULL;
}


BOOL RTP_DataFrame::SetPayloadSize(PINDEX sz)
{
  payloadSize = sz;
  return SetMinSize(GetHeaderSize()+payloadSize);
}


void RTP_DataFrame::PrintOn(ostream & strm) const
{
  strm <<  "V="  << GetVersion()
       << " X="  << GetExtension()
       << " M="  << GetMarker()
       << " PT=" << GetPayloadType()
       << " SN=" << GetSequenceNumber()
       << " TS=" << GetTimestamp()
       << " SSRC=" << GetSyncSource()
       << " size=" << GetPayloadSize()
       << '\n';

  int csrcCount = GetContribSrcCount();
  for (int csrc = 0; csrc < csrcCount; csrc++)
    strm << "  CSRC[" << csrc << "]=" << GetContribSource(csrc) << '\n';

  if (GetExtension())
    strm << "  Header Extension Type: " << GetExtensionType() << '\n'
         << hex << setfill('0') << PBYTEArray(GetExtensionPtr(), GetExtensionSize(), false) << setfill(' ') << dec << '\n';

  strm << hex << setfill('0') << PBYTEArray(GetPayloadPtr(), GetPayloadSize(), false) << setfill(' ') << dec;
}


#if PTRACING
static const char * const PayloadTypesNames[RTP_DataFrame::LastKnownPayloadType] = {
  "PCMU",
  "FS1016",
  "G721",
  "GSM",
  "G7231",
  "DVI4_8k",
  "DVI4_16k",
  "LPC",
  "PCMA",
  "G722",
  "L16_Stereo",
  "L16_Mono",
  "G723",
  "CN",
  "MPA",
  "G728",
  "DVI4_11k",
  "DVI4_22k",
  "G729",
  "CiscoCN",
  NULL, NULL, NULL, NULL, NULL,
  "CelB",
  "JPEG",
  NULL, NULL, NULL, NULL,
  "H261",
  "MPV",
  "MP2T",
  "H263"
};

ostream & operator<<(ostream & o, RTP_DataFrame::PayloadTypes t)
{
  if ((PINDEX)t < PARRAYSIZE(PayloadTypesNames) && PayloadTypesNames[t] != NULL)
    o << PayloadTypesNames[t];
  else
    o << "[pt=" << (int)t << ']';
  return o;
}

#endif


/////////////////////////////////////////////////////////////////////////////

RTP_ControlFrame::RTP_ControlFrame(PINDEX sz)
  : PBYTEArray(sz)
{
  compoundOffset = 0;
  payloadSize = 0;
}

void RTP_ControlFrame::Reset(PINDEX size)
{
  SetSize(size);
  compoundOffset = 0;
  payloadSize = 0;
}


void RTP_ControlFrame::SetCount(unsigned count)
{
  PAssert(count < 32, PInvalidParameter);
  theArray[compoundOffset] &= 0xe0;
  theArray[compoundOffset] |= count;
}


void RTP_ControlFrame::SetPayloadType(unsigned t)
{
  PAssert(t < 256, PInvalidParameter);
  theArray[compoundOffset+1] = (BYTE)t;
}

PINDEX RTP_ControlFrame::GetCompoundSize() const 
{ 
  // transmitted length is the offset of the last compound block
  // plus the compound length of the last block
  return compoundOffset + *(PUInt16b *)&theArray[compoundOffset+2]*4;
}

void RTP_ControlFrame::SetPayloadSize(PINDEX sz)
{
  payloadSize = sz;

  // compound size is in words, rounded up to nearest word
  PINDEX compoundSize = (payloadSize + 3) & ~3;
  PAssert(compoundSize <= 0xffff, PInvalidParameter);

  // make sure buffer is big enough for previous packets plus packet header plus payload
  SetMinSize(compoundOffset + 4 + 4*(compoundSize));

  // put the new compound size into the packet (always at offset 2)
  *(PUInt16b *)&theArray[compoundOffset+2] = (WORD)(compoundSize / 4);
}

BYTE * RTP_ControlFrame::GetPayloadPtr() const 
{ 
  // payload for current packet is always one DWORD after the current compound start
  if ((GetPayloadSize() == 0) || ((compoundOffset + 4) >= GetSize()))
    return NULL;
  return (BYTE *)(theArray + compoundOffset + 4); 
}

BOOL RTP_ControlFrame::ReadNextPacket()
{
  // skip over current packet
  compoundOffset += GetPayloadSize() + 4;

  // see if another packet is feasible
  if (compoundOffset + 4 > GetSize())
    return FALSE;

  // check if payload size for new packet is legal
  return compoundOffset + GetPayloadSize() + 4 <= GetSize();
}


BOOL RTP_ControlFrame::StartNewPacket()
{
  // allocate storage for new packet header
  if (!SetMinSize(compoundOffset + 4))
    return FALSE;

  theArray[compoundOffset] = '\x80'; // Set version 2
  theArray[compoundOffset+1] = 0;    // Set payload type to illegal
  theArray[compoundOffset+2] = 0;    // Set payload size to zero
  theArray[compoundOffset+3] = 0;

  // payload is now zero bytes
  payloadSize = 0;
  SetPayloadSize(payloadSize);

  return TRUE;
}

void RTP_ControlFrame::EndPacket()
{
  // all packets must align to DWORD boundaries
  while (((4 + payloadSize) & 3) != 0) {
    theArray[compoundOffset + 4 + payloadSize - 1] = 0;
    ++payloadSize;
  }

  compoundOffset += 4 + payloadSize;
  payloadSize = 0;
}

void RTP_ControlFrame::StartSourceDescription(DWORD src)
{
  // extend payload to include SSRC + END
  SetPayloadSize(payloadSize + 4 + 1);  
  SetPayloadType(RTP_ControlFrame::e_SourceDescription);
  SetCount(GetCount()+1); // will be incremented automatically

  // get ptr to new item SDES
  BYTE * payload = GetPayloadPtr();
  *(PUInt32b *)payload = src;
  payload[4] = e_END;
}


void RTP_ControlFrame::AddSourceDescriptionItem(unsigned type, const PString & data)
{
  // get ptr to new item, remembering that END was inserted previously
  BYTE * payload = GetPayloadPtr() + payloadSize - 1;

  // length of new item
  PINDEX dataLength = data.GetLength();

  // add storage for new item (note that END has already been included)
  SetPayloadSize(payloadSize + 1 + 1 + dataLength);

  // insert new item
  payload[0] = (BYTE)type;
  payload[1] = (BYTE)dataLength;
  memcpy(payload+2, (const char *)data, dataLength);

  // insert new END
  payload[2+dataLength] = (BYTE)e_END;
}


void RTP_ControlFrame::ReceiverReport::SetLostPackets(unsigned packets)
{
  lost[0] = (BYTE)(packets >> 16);
  lost[1] = (BYTE)(packets >> 8);
  lost[2] = (BYTE)packets;
}


/////////////////////////////////////////////////////////////////////////////

void RTP_UserData::OnTxStatistics(const RTP_Session & /*session*/) const
{
}


void RTP_UserData::OnRxStatistics(const RTP_Session & /*session*/) const
{
}

#if OPAL_VIDEO
void RTP_UserData::OnRxIntraFrameRequest(const RTP_Session & /*session*/) const
{
}

void RTP_UserData::OnTxIntraFrameRequest(const RTP_Session & /*session*/) const
{
}
#endif


/////////////////////////////////////////////////////////////////////////////

RTP_Session::RTP_Session(
                         PHandleAggregator * _aggregator, 
                         unsigned id, RTP_UserData * data, BOOL autoDelete)
: canonicalName(PProcess::Current().GetUserName()),
  toolName(PProcess::Current().GetName()),
  reportTimeInterval(0, 12),  // Seconds
  reportTimer(reportTimeInterval),
  aggregator(_aggregator)
{
  PAssert(id > 0 && id < 256, PInvalidParameter);
  sessionID = (BYTE)id;

  referenceCount = 1;
  userData = data;
  autoDeleteUserData = autoDelete;
  jitter = NULL;

  ignoreOtherSources = TRUE;
  ignoreOutOfOrderPackets = TRUE;
  ignorePayloadTypeChanges = TRUE;
  syncSourceOut = PRandom::Number();

  timeStampOut = PRandom::Number();
  timeStampOffs = 0;
  timeStampOffsetEstablished = FALSE;
  timeStampIsPremedia = FALSE;
  lastSentPacketTime = PTimer::Tick();

  syncSourceIn = 0;
  allowSyncSourceInChange = FALSE;
  allowRemoteTransmitAddressChange = FALSE;
  allowSequenceChange = FALSE;
  txStatisticsInterval = 100;  // Number of data packets between tx reports
  rxStatisticsInterval = 100;  // Number of data packets between rx reports
  lastSentSequenceNumber = (WORD)PRandom::Number();
  expectedSequenceNumber = 0;
  lastRRSequenceNumber = 0;
  consecutiveOutOfOrderPackets = 0;

  packetsSent = 0;
  rtcpPacketsSent = 0;
  octetsSent = 0;
  packetsReceived = 0;
  octetsReceived = 0;
  packetsLost = 0;
  packetsOutOfOrder = 0;
  averageSendTime = 0;
  maximumSendTime = 0;
  minimumSendTime = 0;
  averageReceiveTime = 0;
  maximumReceiveTime = 0;
  minimumReceiveTime = 0;
  jitterLevel = 0;
  maximumJitterLevel = 0;
  markerRecvCount = 0;
  markerSendCount = 0;

  txStatisticsCount = 0;
  rxStatisticsCount = 0;
  averageSendTimeAccum = 0;
  maximumSendTimeAccum = 0;
  minimumSendTimeAccum = 0xffffffff;
  averageReceiveTimeAccum = 0;
  maximumReceiveTimeAccum = 0;
  minimumReceiveTimeAccum = 0xffffffff;
  packetsLostSinceLastRR = 0;
  lastTransitTime = 0;

  lastReceivedPayloadType = RTP_DataFrame::IllegalPayloadType;

  closeOnBye = FALSE;
  byeSent    = FALSE;
}

RTP_Session::~RTP_Session()
{
  PTRACE_IF(3, packetsSent != 0 || packetsReceived != 0,
      "RTP\tFinal statistics:\n"
      "    packetsSent       = " << packetsSent << "\n"
      "    octetsSent        = " << octetsSent << "\n"
      "    averageSendTime   = " << averageSendTime << "\n"
      "    maximumSendTime   = " << maximumSendTime << "\n"
      "    minimumSendTime   = " << minimumSendTime << "\n"
      "    packetsReceived   = " << packetsReceived << "\n"
      "    octetsReceived    = " << octetsReceived << "\n"
      "    packetsLost       = " << packetsLost << "\n"
      "    packetsTooLate    = " << GetPacketsTooLate() << "\n"
      "    packetsOutOfOrder = " << packetsOutOfOrder << "\n"
      "    averageReceiveTime= " << averageReceiveTime << "\n"
      "    maximumReceiveTime= " << maximumReceiveTime << "\n"
      "    minimumReceiveTime= " << minimumReceiveTime << "\n"
      "    averageJitter     = " << (jitterLevel >> 7) << "\n"
      "    maximumJitter     = " << (maximumJitterLevel >> 7)
     );
  delete jitter;
  if (autoDeleteUserData)
    delete userData;
}

void RTP_Session::SendBYE()
{
  if (byeSent)
    return;

  byeSent = TRUE;

  RTP_ControlFrame report;

  // if any packets sent, put in a non-zero report 
  // else put in a zero report
  if (packetsSent != 0 || rtcpPacketsSent != 0) 
    InsertReportPacket(report);
  else {
    // Send empty RR as nothing has happened
    report.StartNewPacket();
    report.SetPayloadType(RTP_ControlFrame::e_ReceiverReport);
    report.SetPayloadSize(4);  // length is SSRC 
    report.SetCount(0);

    // add the SSRC to the start of the payload
    BYTE * payload = report.GetPayloadPtr();
    *(PUInt32b *)payload = syncSourceOut;
    report.EndPacket();
  }

  const char * reasonStr = "session ending";

  // insert BYE
  report.StartNewPacket();
  report.SetPayloadType(RTP_ControlFrame::e_Goodbye);
  report.SetPayloadSize(4+1+strlen(reasonStr));  // length is SSRC + reasonLen + reason

  BYTE * payload = report.GetPayloadPtr();

  // one SSRC
  report.SetCount(1);
  *(PUInt32b *)payload = syncSourceOut;

  // insert reason
  payload[4] = (BYTE)strlen(reasonStr);
  memcpy((char *)(payload+5), reasonStr, payload[4]);

  report.EndPacket();
  WriteControl(report);
}

PString RTP_Session::GetCanonicalName() const
{
  PWaitAndSignal mutex(reportMutex);
  PString s = canonicalName;
  s.MakeUnique();
  return s;
}


void RTP_Session::SetCanonicalName(const PString & name)
{
  PWaitAndSignal mutex(reportMutex);
  canonicalName = name;
}


PString RTP_Session::GetToolName() const
{
  PWaitAndSignal mutex(reportMutex);
  PString s = toolName;
  s.MakeUnique();
  return s;
}


void RTP_Session::SetToolName(const PString & name)
{
  PWaitAndSignal mutex(reportMutex);
  toolName = name;
}


void RTP_Session::SetUserData(RTP_UserData * data, BOOL autoDelete)
{
  if (autoDeleteUserData)
    delete userData;
  userData = data;
  autoDeleteUserData = autoDelete;
}


void RTP_Session::SetJitterBufferSize(unsigned minJitterDelay,
              unsigned maxJitterDelay,
              unsigned timeUnits,
              PINDEX stackSize)
{
  if (minJitterDelay == 0 && maxJitterDelay == 0) {
    delete jitter;
    jitter = NULL;
  }
  else if (jitter != NULL)
    jitter->SetDelay(minJitterDelay, maxJitterDelay);
  else {
    SetIgnoreOutOfOrderPackets(FALSE);
    jitter = new RTP_JitterBuffer(*this, minJitterDelay, maxJitterDelay, timeUnits, stackSize);
    jitter->Resume(aggregator);
  }
}


unsigned RTP_Session::GetJitterBufferSize() const
{
  return jitter != NULL ? jitter->GetJitterTime() : 0;
}

unsigned RTP_Session::GetJitterTimeUnits() const
{
  return jitter != NULL ? jitter->GetTimeUnits() : 0;
}


BOOL RTP_Session::ReadBufferedData(DWORD timestamp, RTP_DataFrame & frame)
{
  if (jitter != NULL)
    return jitter->ReadData(timestamp, frame);
  else
    return ReadData(frame, TRUE);
}


void RTP_Session::SetTxStatisticsInterval(unsigned packets)
{
  txStatisticsInterval = PMAX(packets, 2);
  txStatisticsCount = 0;
  averageSendTimeAccum = 0;
  maximumSendTimeAccum = 0;
  minimumSendTimeAccum = 0xffffffff;
}


void RTP_Session::SetRxStatisticsInterval(unsigned packets)
{
  rxStatisticsInterval = PMAX(packets, 2);
  rxStatisticsCount = 0;
  averageReceiveTimeAccum = 0;
  maximumReceiveTimeAccum = 0;
  minimumReceiveTimeAccum = 0xffffffff;
}


void RTP_Session::AddReceiverReport(RTP_ControlFrame::ReceiverReport & receiver)
{
  receiver.ssrc = syncSourceIn;
  receiver.SetLostPackets(packetsLost);

  if (expectedSequenceNumber > lastRRSequenceNumber)
    receiver.fraction = (BYTE)((packetsLostSinceLastRR<<8)/(expectedSequenceNumber - lastRRSequenceNumber));
  else
    receiver.fraction = 0;
  packetsLostSinceLastRR = 0;

  receiver.last_seq = lastRRSequenceNumber;
  lastRRSequenceNumber = expectedSequenceNumber;

  receiver.jitter = jitterLevel >> 4; // Allow for rounding protection bits

  // The following have not been calculated yet.
  receiver.lsr = 0;
  receiver.dlsr = 0;

  PTRACE(3, "RTP\tSentReceiverReport:"
   " ssrc=" << receiver.ssrc
   << " fraction=" << (unsigned)receiver.fraction
   << " lost=" << receiver.GetLostPackets()
   << " last_seq=" << receiver.last_seq
   << " jitter=" << receiver.jitter
   << " lsr=" << receiver.lsr
   << " dlsr=" << receiver.dlsr);
}

RTP_Session::SendReceiveStatus RTP_Session::OnSendData(RTP_DataFrame & frame)
{
  PWaitAndSignal m(sendDataMutex);

  PTimeInterval tick = PTimer::Tick();  // Timestamp set now

  frame.SetSequenceNumber(++lastSentSequenceNumber);
  frame.SetSyncSource(syncSourceOut);

  // special handling for first packet
  if (packetsSent == 0) {
    if (!timeStampIsPremedia) {
      timeStampOffs = frame.GetTimestamp() - timeStampOut;
      timeStampOffsetEstablished = TRUE;
    }
    frame.SetTimestamp(frame.GetTimestamp() + timeStampOffs);

    PTRACE(3, "RTP\tFirst sent data:"
     " ver=" << frame.GetVersion()
     << " pt=" << frame.GetPayloadType()
     << " psz=" << frame.GetPayloadSize()
     << " m=" << frame.GetMarker()
     << " x=" << frame.GetExtension()
     << " seq=" << frame.GetSequenceNumber()
     << " ts=" << frame.GetTimestamp()
     << " src=" << frame.GetSyncSource()
     << " ccnt=" << frame.GetContribSrcCount());
  }
  else {
    frame.SetTimestamp(frame.GetTimestamp() + timeStampOffs);

    // Only do statistics on subsequent packets
    if (!frame.GetMarker()) {
      DWORD diff = (tick - lastSentPacketTime).GetInterval();

      averageSendTimeAccum += diff;
      if (diff > maximumSendTimeAccum)
        maximumSendTimeAccum = diff;
      if (diff < minimumSendTimeAccum)
        minimumSendTimeAccum = diff;
      txStatisticsCount++;
    }
  }

  lastSentPacketTime = tick;

  octetsSent += frame.GetPayloadSize();
  packetsSent++;

  if(frame.GetMarker())
    markerSendCount++;

  // Call the statistics call-back on the first PDU with total count == 1
  if (packetsSent == 1 && userData != NULL)
    userData->OnTxStatistics(*this);

  if (!SendReport())
    return e_AbortTransport;

  if (txStatisticsCount < txStatisticsInterval)
    return e_ProcessPacket;

  txStatisticsCount = 0;

  averageSendTime = averageSendTimeAccum/txStatisticsInterval;
  maximumSendTime = maximumSendTimeAccum;
  minimumSendTime = minimumSendTimeAccum;

  averageSendTimeAccum = 0;
  maximumSendTimeAccum = 0;
  minimumSendTimeAccum = 0xffffffff;

  PTRACE(3, "RTP\tTransmit statistics: "
   " packets=" << packetsSent <<
   " octets=" << octetsSent <<
   " avgTime=" << averageSendTime <<
   " maxTime=" << maximumSendTime <<
   " minTime=" << minimumSendTime
  );

  if (userData != NULL)
    userData->OnTxStatistics(*this);

  return e_ProcessPacket;
}

RTP_Session::SendReceiveStatus RTP_Session::OnSendControl(RTP_ControlFrame & frame, PINDEX & /*len*/)
{
  rtcpPacketsSent++;
#if OPAL_VIDEO
  if(frame.GetPayloadType() == RTP_ControlFrame::e_IntraFrameRequest && userData != NULL)
    userData->OnTxIntraFrameRequest(*this);
#endif 
  return e_ProcessPacket;
}

RTP_Session::SendReceiveStatus RTP_Session::OnReceiveData(RTP_DataFrame & frame)
{
  // Check that the PDU is the right version
  if (frame.GetVersion() != RTP_DataFrame::ProtocolVersion)
    return e_IgnorePacket; // Non fatal error, just ignore

  // Check if expected payload type
  if (lastReceivedPayloadType == RTP_DataFrame::IllegalPayloadType)
    lastReceivedPayloadType = frame.GetPayloadType();

  if (lastReceivedPayloadType != frame.GetPayloadType() && !ignorePayloadTypeChanges) {

    PTRACE(4, "RTP\tReceived payload type " << frame.GetPayloadType() << ", but was expecting " << lastReceivedPayloadType);
    return e_IgnorePacket;
  }

  // Check for if a control packet rather than data packet.
  if (frame.GetPayloadType() > RTP_DataFrame::MaxPayloadType)
    return e_IgnorePacket; // Non fatal error, just ignore

  PTimeInterval tick = PTimer::Tick();  // Get timestamp now

  // Have not got SSRC yet, so grab it now
  if (syncSourceIn == 0)
    syncSourceIn = frame.GetSyncSource();

  // Check packet sequence numbers
  if (packetsReceived == 0) {
    expectedSequenceNumber = (WORD)(frame.GetSequenceNumber() + 1);
    PTRACE(3, "RTP\tFirst receive data:"
     " ver=" << frame.GetVersion()
     << " pt=" << frame.GetPayloadType()
     << " psz=" << frame.GetPayloadSize()
     << " m=" << frame.GetMarker()
     << " x=" << frame.GetExtension()
     << " seq=" << frame.GetSequenceNumber()
     << " ts=" << frame.GetTimestamp()
     << " src=" << frame.GetSyncSource()
     << " ccnt=" << frame.GetContribSrcCount());
  }
  else {
    if (frame.GetSyncSource() != syncSourceIn) {
      if (!ignoreOtherSources)
        syncSourceIn = frame.GetSyncSource();
      else if (allowSyncSourceInChange) {
        PTRACE(2, "RTP\tAllowed one SSRC change from SSRC=" << syncSourceIn << " to =" << frame.GetSyncSource());
        syncSourceIn = frame.GetSyncSource();
        allowSyncSourceInChange = FALSE;
      }
      else {
        PTRACE(2, "RTP\tPacket from SSRC=" << frame.GetSyncSource() << " ignored, expecting SSRC=" << syncSourceIn);
        return e_IgnorePacket; // Non fatal error, just ignore
      }
    }

    WORD sequenceNumber = frame.GetSequenceNumber();
    if (sequenceNumber == expectedSequenceNumber) {
      expectedSequenceNumber++;
      consecutiveOutOfOrderPackets = 0;
      // Only do statistics on packets after first received in talk burst
      if (!frame.GetMarker()) {
        DWORD diff = (tick - lastReceivedPacketTime).GetInterval();

        averageReceiveTimeAccum += diff;
        if (diff > maximumReceiveTimeAccum)
          maximumReceiveTimeAccum = diff;
        if (diff < minimumReceiveTimeAccum)
          minimumReceiveTimeAccum = diff;
        rxStatisticsCount++;

        // The following has the implicit assumption that something that has jitter
        // is an audio codec and thus is in 8kHz timestamp units.
        diff *= 8;
        long variance = diff - lastTransitTime;
        lastTransitTime = diff;
        if (variance < 0)
          variance = -variance;
        jitterLevel += variance - ((jitterLevel+8) >> 4);
        if (jitterLevel > maximumJitterLevel)
          maximumJitterLevel = jitterLevel;
      }
      else {
        markerRecvCount++;
      }
    }
    else if (allowSequenceChange) {
      expectedSequenceNumber = (WORD) (sequenceNumber + 1);
      allowSequenceChange = FALSE;
    }
    else if (sequenceNumber < expectedSequenceNumber) {
      PTRACE(2, "RTP\tOut of order packet, received "
       << sequenceNumber << " expected " << expectedSequenceNumber
       << " ssrc=" << syncSourceIn);
      packetsOutOfOrder++;

      // Check for Cisco bug where sequence numbers suddenly start incrementing
      // from a different base.
      if (++consecutiveOutOfOrderPackets > 10) {
        expectedSequenceNumber = (WORD)(sequenceNumber + 1);
        PTRACE(2, "RTP\tAbnormal change of sequence numbers, adjusting to expect " << expectedSequenceNumber << " ssrc=" << syncSourceIn);
      }

      if (ignoreOutOfOrderPackets)
        return e_IgnorePacket; // Non fatal error, just ignore
    }
    else {
      unsigned dropped = sequenceNumber - expectedSequenceNumber;
      packetsLost += dropped;
      packetsLostSinceLastRR += dropped;
      PTRACE(2, "RTP\tDropped " << dropped << " packet(s) at " << sequenceNumber
       << ", ssrc=" << syncSourceIn);
      expectedSequenceNumber = (WORD)(sequenceNumber + 1);
      consecutiveOutOfOrderPackets = 0;
    }
  }

  lastReceivedPacketTime = tick;

  octetsReceived += frame.GetPayloadSize();
  packetsReceived++;

  // Call the statistics call-back on the first PDU with total count == 1
  if (packetsReceived == 1 && userData != NULL)
    userData->OnRxStatistics(*this);

  if (!SendReport())
    return e_AbortTransport;

  if (rxStatisticsCount < rxStatisticsInterval)
    return e_ProcessPacket;

  rxStatisticsCount = 0;

  averageReceiveTime = averageReceiveTimeAccum/rxStatisticsInterval;
  maximumReceiveTime = maximumReceiveTimeAccum;
  minimumReceiveTime = minimumReceiveTimeAccum;

  averageReceiveTimeAccum = 0;
  maximumReceiveTimeAccum = 0;
  minimumReceiveTimeAccum = 0xffffffff;

  PTRACE(4, "RTP\tReceive statistics: "
   " packets=" << packetsReceived <<
   " octets=" << octetsReceived <<
   " lost=" << packetsLost <<
   " tooLate=" << GetPacketsTooLate() <<
   " order=" << packetsOutOfOrder <<
   " avgTime=" << averageReceiveTime <<
   " maxTime=" << maximumReceiveTime <<
   " minTime=" << minimumReceiveTime <<
   " jitter=" << (jitterLevel >> 7) <<
   " maxJitter=" << (maximumJitterLevel >> 7)
  );

  if (userData != NULL)
    userData->OnRxStatistics(*this);

  return e_ProcessPacket;
}

BOOL RTP_Session::InsertReportPacket(RTP_ControlFrame & report)
{
  // No packets sent yet, so only set RR
  if (packetsSent == 0) {

    // Send RR as we are not transmitting
    report.StartNewPacket();
    report.SetPayloadType(RTP_ControlFrame::e_ReceiverReport);
    report.SetPayloadSize(4 + sizeof(RTP_ControlFrame::ReceiverReport));  // length is SSRC of packet sender plus RR
    report.SetCount(1);
    BYTE * payload = report.GetPayloadPtr();

    // add the SSRC to the start of the payload
    *(PUInt32b *)payload = syncSourceOut;

    // add the RR after the SSRC
    AddReceiverReport(*(RTP_ControlFrame::ReceiverReport *)(payload+4));
  }
  else
  {
    // send SR and RR
    report.StartNewPacket();
    report.SetPayloadType(RTP_ControlFrame::e_SenderReport);
    report.SetPayloadSize(4 + sizeof(RTP_ControlFrame::SenderReport));  // length is SSRC of packet sender plus SR
    report.SetCount(0);
    BYTE * payload = report.GetPayloadPtr();

    // add the SSRC to the start of the payload
    *(PUInt32b *)payload = syncSourceOut;

    // add the SR after the SSRC
    RTP_ControlFrame::SenderReport * sender = (RTP_ControlFrame::SenderReport *)(payload+4);
    PTime now;
    sender->ntp_sec  = (DWORD)(now.GetTimeInSeconds()+SecondsFrom1900to1970); // Convert from 1970 to 1900
    sender->ntp_frac = now.GetMicrosecond()*4294; // Scale microseconds to "fraction" from 0 to 2^32
    sender->rtp_ts   = lastSentTimestamp;
    sender->psent    = packetsSent;
    sender->osent    = octetsSent;

    PTRACE(3, "RTP\tSentSenderReport: "
     " ssrc=" << syncSourceOut
     << " ntp=" << sender->ntp_sec << '.' << sender->ntp_frac
     << " rtp=" << sender->rtp_ts
     << " psent=" << sender->psent
     << " osent=" << sender->osent);

    if (syncSourceIn != 0) {
      report.SetPayloadSize(4 + sizeof(RTP_ControlFrame::SenderReport) + sizeof(RTP_ControlFrame::ReceiverReport));
      report.SetCount(1);
      AddReceiverReport(*(RTP_ControlFrame::ReceiverReport *)(payload+4+sizeof(RTP_ControlFrame::SenderReport)));
    }
  }

  report.EndPacket();

  // Wait a fuzzy amount of time so things don't get into lock step
  int interval = (int)reportTimeInterval.GetMilliSeconds();
  int third = interval/3;
  interval += PRandom::Number()%(2*third);
  interval -= third;
  reportTimer = interval;

  return TRUE;
}

BOOL RTP_Session::SendReport()
{
  PWaitAndSignal mutex(reportMutex);

  if (reportTimer.IsRunning())
    return TRUE;

  // Have not got anything yet, do nothing
  if (packetsSent == 0 && packetsReceived == 0) {
    reportTimer = reportTimeInterval;
    return TRUE;
  }

  RTP_ControlFrame report;

  InsertReportPacket(report);

  // Add the SDES part to compound RTCP packet
  PTRACE(3, "RTP\tSending SDES: " << canonicalName);
  report.StartNewPacket();

  report.SetCount(0); // will be incremented automatically
  report.StartSourceDescription  (syncSourceOut);
  report.AddSourceDescriptionItem(RTP_ControlFrame::e_CNAME, canonicalName);
  report.AddSourceDescriptionItem(RTP_ControlFrame::e_TOOL, toolName);
  report.EndPacket();

  BOOL stat = WriteControl(report);

  return stat;
}


static RTP_Session::ReceiverReportArray
BuildReceiverReportArray(const RTP_ControlFrame & frame, PINDEX offset)
{
  RTP_Session::ReceiverReportArray reports;

  const RTP_ControlFrame::ReceiverReport * rr = (const RTP_ControlFrame::ReceiverReport *)(frame.GetPayloadPtr()+offset);
  for (PINDEX repIdx = 0; repIdx < (PINDEX)frame.GetCount(); repIdx++) {
    RTP_Session::ReceiverReport * report = new RTP_Session::ReceiverReport;
    report->sourceIdentifier = rr->ssrc;
    report->fractionLost = rr->fraction;
    report->totalLost = rr->GetLostPackets();
    report->lastSequenceNumber = rr->last_seq;
    report->jitter = rr->jitter;
    report->lastTimestamp = (PInt64)(DWORD)rr->lsr;
    report->delay = ((PInt64)rr->dlsr << 16)/1000;
    reports.SetAt(repIdx, report);
    rr++;
  }

  return reports;
}


RTP_Session::SendReceiveStatus RTP_Session::OnReceiveControl(RTP_ControlFrame & frame)
{
  do {
    BYTE * payload = frame.GetPayloadPtr();
    unsigned size = frame.GetPayloadSize(); 
    if ((payload == NULL) || (size == 0) || ((payload + size) > (frame.GetPointer() + frame.GetSize()))){
      /* TODO: 1.shall we test for a maximum size ? Indeed but what's the value ? *
               2. what's the correct exit status ? */
      PTRACE(2, "RTP\tOnReceiveControl invalid frame");

      break;
    }
    switch (frame.GetPayloadType()) {
    case RTP_ControlFrame::e_SenderReport :
      if (size >= sizeof(RTP_ControlFrame::SenderReport)) {
        SenderReport sender;
        sender.sourceIdentifier = *(const PUInt32b *)payload;
        const RTP_ControlFrame::SenderReport & sr = *(const RTP_ControlFrame::SenderReport *)(payload+4);
        sender.realTimestamp = PTime(sr.ntp_sec-SecondsFrom1900to1970, sr.ntp_frac/4294);
        sender.rtpTimestamp = sr.rtp_ts;
        sender.packetsSent = sr.psent;
        sender.octetsSent = sr.osent;
        OnRxSenderReport(sender, BuildReceiverReportArray(frame, sizeof(RTP_ControlFrame::SenderReport)));
      }
      else {
        PTRACE(2, "RTP\tSenderReport packet truncated");
      }
      break;

    case RTP_ControlFrame::e_ReceiverReport :
      if (size >= 4)
        OnRxReceiverReport(*(const PUInt32b *)payload,
        BuildReceiverReportArray(frame, sizeof(PUInt32b)));
      else {
        PTRACE(2, "RTP\tReceiverReport packet truncated");
      }
      break;

    case RTP_ControlFrame::e_SourceDescription :
      if (size >= frame.GetCount()*sizeof(RTP_ControlFrame::SourceDescription)) {
        SourceDescriptionArray descriptions;
        const RTP_ControlFrame::SourceDescription * sdes = (const RTP_ControlFrame::SourceDescription *)payload;
        PINDEX srcIdx;
        for (srcIdx = 0; srcIdx < (PINDEX)frame.GetCount(); srcIdx++) {
          descriptions.SetAt(srcIdx, new SourceDescription(sdes->src));
          const RTP_ControlFrame::SourceDescription::Item * item = sdes->item;
          unsigned uiSizeCurrent = 0;   /* current size of the items already parsed */
          while ((item != NULL) && (item->type != RTP_ControlFrame::e_END)) {
            descriptions[srcIdx].items.SetAt(item->type, PString(item->data, item->length));
            uiSizeCurrent += item->GetLengthTotal();
            PTRACE(4,"RTP\tSourceDescription item " << item << ", current size = " << uiSizeCurrent);
            
            /* avoid reading where GetNextItem() shall not */
            if (uiSizeCurrent >= size){
              PTRACE(4,"RTP\tSourceDescription end of items");
              item = NULL;
              break;
            } else {
              item = item->GetNextItem();
            }
          }
          /* RTP_ControlFrame::e_END doesn't have a length field, so do NOT call item->GetNextItem()
             otherwise it reads over the buffer */
          if((item == NULL) || 
            (item->type == RTP_ControlFrame::e_END) || 
            ((sdes = (const RTP_ControlFrame::SourceDescription *)item->GetNextItem()) == NULL)){
            break;
          }
        }
        OnRxSourceDescription(descriptions);
      }
      else {
        PTRACE(2, "RTP\tSourceDescription packet truncated");
      }
      break;

    case RTP_ControlFrame::e_Goodbye :
    {
      unsigned count = frame.GetCount()*4;
      if ((size >= 4) && (count > 0)) {
        PString str;
  
        if (size > count){
          if((payload[count] + sizeof(DWORD) /*SSRC*/ + sizeof(unsigned char) /* length */) <= size){
            str = PString((const char *)(payload+count+1), payload[count]);
          } else {
            PTRACE(2, "RTP\tGoodbye packet invalid");
          }
        }
        PDWORDArray sources(frame.GetCount());
        for (PINDEX i = 0; i < (PINDEX)frame.GetCount(); i++){
          sources[i] = ((const PUInt32b *)payload)[i];
        }  
        OnRxGoodbye(sources, str);
        }
      else {
        PTRACE(2, "RTP\tGoodbye packet truncated");
      }
      if (closeOnBye) {
        PTRACE(3, "RTP\tGoodbye packet closing transport");
        return e_AbortTransport;
      }
    break;

    }
    case RTP_ControlFrame::e_ApplDefined :
      if (size >= 4) {
        PString str((const char *)(payload+4), 4);
        OnRxApplDefined(str, frame.GetCount(), *(const PUInt32b *)payload,
        payload+8, frame.GetPayloadSize()-8);
      }
      else {
        PTRACE(2, "RTP\tApplDefined packet truncated");
      }
      break;

#if OPAL_VIDEO
     case RTP_ControlFrame::e_IntraFrameRequest :
      if(userData != NULL)
        userData->OnRxIntraFrameRequest(*this);
      break;
#endif

    default :
      PTRACE(2, "RTP\tUnknown control payload type: " << frame.GetPayloadType());
    }
  } while (frame.ReadNextPacket());

  return e_ProcessPacket;
}


void RTP_Session::OnRxSenderReport(const SenderReport & PTRACE_PARAM(sender),
           const ReceiverReportArray & PTRACE_PARAM(reports))
{
#if PTRACING
  if (PTrace::CanTrace(3)) {
    ostream & strm = PTrace::Begin(3, __FILE__, __LINE__);
    strm << "RTP\tOnRxSenderReport: " << sender << '\n';
    for (PINDEX i = 0; i < reports.GetSize(); i++)
      strm << "  RR: " << reports[i] << '\n';
    strm << PTrace::End;
  }
#endif
}


void RTP_Session::OnRxReceiverReport(DWORD PTRACE_PARAM(src),
             const ReceiverReportArray & PTRACE_PARAM(reports))
{
#if PTRACING
  if (PTrace::CanTrace(3)) {
    ostream & strm = PTrace::Begin(2, __FILE__, __LINE__);
    strm << "RTP\tOnReceiverReport: ssrc=" << src << '\n';
    for (PINDEX i = 0; i < reports.GetSize(); i++)
      strm << "  RR: " << reports[i] << '\n';
    strm << PTrace::End;
  }
#endif
}


void RTP_Session::OnRxSourceDescription(const SourceDescriptionArray & PTRACE_PARAM(description))
{
#if PTRACING
  if (PTrace::CanTrace(3)) {
    ostream & strm = PTrace::Begin(3, __FILE__, __LINE__);
    strm << "RTP\tOnSourceDescription: " << description.GetSize() << " entries";
    for (PINDEX i = 0; i < description.GetSize(); i++)
      strm << "\n  " << description[i];
    strm << PTrace::End;
  }
#endif
}


void RTP_Session::OnRxGoodbye(const PDWORDArray & PTRACE_PARAM(src), const PString & PTRACE_PARAM(reason))
{
  PTRACE(3, "RTP\tOnGoodbye: \"" << reason << "\" srcs=" << src);
}


void RTP_Session::OnRxApplDefined(const PString & PTRACE_PARAM(type),
          unsigned PTRACE_PARAM(subtype), DWORD PTRACE_PARAM(src),
          const BYTE * /*data*/, PINDEX PTRACE_PARAM(size))
{
  PTRACE(3, "RTP\tOnApplDefined: \"" << type << "\"-" << subtype
   << " " << src << " [" << size << ']');
}


void RTP_Session::ReceiverReport::PrintOn(ostream & strm) const
{
  strm << "ssrc=" << sourceIdentifier
    << " fraction=" << fractionLost
    << " lost=" << totalLost
    << " last_seq=" << lastSequenceNumber
    << " jitter=" << jitter
    << " lsr=" << lastTimestamp
    << " dlsr=" << delay;
}


void RTP_Session::SenderReport::PrintOn(ostream & strm) const
{
  strm << "ssrc=" << sourceIdentifier
    << " ntp=" << realTimestamp.AsString("yyyy/M/d-h:m:s.uuuu")
    << " rtp=" << rtpTimestamp
    << " psent=" << packetsSent
    << " osent=" << octetsSent;
}


void RTP_Session::SourceDescription::PrintOn(ostream & strm) const
{
  static const char * const DescriptionNames[RTP_ControlFrame::NumDescriptionTypes] = {
    "END", "CNAME", "NAME", "EMAIL", "PHONE", "LOC", "TOOL", "NOTE", "PRIV"
  };

  strm << "ssrc=" << sourceIdentifier;
  for (PINDEX i = 0; i < items.GetSize(); i++) {
    strm << "\n  item[" << i << "]: type=";
    unsigned typeNum = items.GetKeyAt(i);
    if (typeNum < PARRAYSIZE(DescriptionNames))
      strm << DescriptionNames[typeNum];
    else
      strm << typeNum;
    strm << " data=\""
      << items.GetDataAt(i)
      << '"';
  }
}


DWORD RTP_Session::GetPacketsTooLate() const
{
  return jitter != NULL ? jitter->GetPacketsTooLate() : 0;
}

BOOL RTP_Session::WriteOOBData(RTP_DataFrame &)
{
  return TRUE;
}

/////////////////////////////////////////////////////////////////////////////

RTP_SessionManager::RTP_SessionManager()
{
  enumerationIndex = P_MAX_INDEX;
}


RTP_SessionManager::RTP_SessionManager(const RTP_SessionManager & sm)
: sessions(sm.sessions)
{
  enumerationIndex = P_MAX_INDEX;
}


RTP_SessionManager & RTP_SessionManager::operator=(const RTP_SessionManager & sm)
{
  PWaitAndSignal m1(mutex);
  PWaitAndSignal m2(sm.mutex);
  sessions = sm.sessions;
  return *this;
}


RTP_Session * RTP_SessionManager::UseSession(unsigned sessionID)
{
  PWaitAndSignal m(mutex);

  RTP_Session * session = sessions.GetAt(sessionID);
  if (session == NULL) 
    return NULL; 
  
  PTRACE(3, "RTP\tFound existing session " << sessionID);
  session->IncrementReference();

  return session;
}


void RTP_SessionManager::AddSession(RTP_Session * session)
{
  PWaitAndSignal m(mutex);
  
  if (session != NULL) {
    PTRACE(3, "RTP\tAdding session " << *session);
    sessions.SetAt(session->GetSessionID(), session);
  }
}


void RTP_SessionManager::ReleaseSession(unsigned sessionID,
                                        BOOL clearAll)
{
  PTRACE(3, "RTP\tReleasing session " << sessionID);

  mutex.Wait();

  while (sessions.Contains(sessionID)) {
    RTP_Session * session = &sessions[sessionID];
    if (session->DecrementReference()) {
      PTRACE(3, "RTP\tDeleting session " << sessionID);
      session->Close(TRUE);
      session->SetJitterBufferSize(0, 0);
      sessions.SetAt(sessionID, NULL);
    }
    if (!clearAll)
      break;
  }

  mutex.Signal();
}


RTP_Session * RTP_SessionManager::GetSession(unsigned sessionID) const
{
  PWaitAndSignal wait(mutex);
  if (!sessions.Contains(sessionID))
    return NULL;

  PTRACE(3, "RTP\tFound existing session " << sessionID);
  return &sessions[sessionID];
}


RTP_Session * RTP_SessionManager::First()
{
  mutex.Wait();

  enumerationIndex = 0;
  return Next();
}


RTP_Session * RTP_SessionManager::Next()
{
  if (enumerationIndex < sessions.GetSize())
    return &sessions.GetDataAt(enumerationIndex++);

  Exit();
  return NULL;
}


void RTP_SessionManager::Exit()
{
  enumerationIndex = P_MAX_INDEX;
  mutex.Signal();
}


/////////////////////////////////////////////////////////////////////////////

static void SetMinBufferSize(PUDPSocket & sock, int buftype)
{
  int sz = 0;
  if (sock.GetOption(buftype, sz)) {
    if (sz >= UDP_BUFFER_SIZE)
      return;
  }

  if (!sock.SetOption(buftype, UDP_BUFFER_SIZE)) {
    PTRACE(1, "RTP_UDP\tSetOption(" << buftype << ") failed: " << sock.GetErrorText());
  }
}


RTP_UDP::RTP_UDP(PHandleAggregator * _aggregator, unsigned id, BOOL _remoteIsNAT)
  : RTP_Session(_aggregator, id),
    remoteAddress(0),
    remoteTransmitAddress(0),
    remoteIsNAT(_remoteIsNAT)
{
  PTRACE(4, "RTP_UDP\tRTP session created with NAT flag set to " << remoteIsNAT);
  remoteDataPort = 0;
  remoteControlPort = 0;
  shutdownRead = FALSE;
  shutdownWrite = FALSE;
  dataSocket = NULL;
  controlSocket = NULL;
  appliedQOS = FALSE;
}


RTP_UDP::~RTP_UDP()
{
  Close(TRUE);
  Close(FALSE);

  // We need to do this to make sure that the sockets are not
  // deleted before select decides there is no more data coming
  // over them and exits the reading thread.
  if (jitter)
    PAssert(jitter->WaitForTermination(10000), "Jitter buffer thread did not terminate");
  
  delete dataSocket;
  delete controlSocket;
}


void RTP_UDP::ApplyQOS(const PIPSocket::Address & addr)
{
  if (controlSocket != NULL)
    controlSocket->SetSendAddress(addr,GetRemoteControlPort());
  if (dataSocket != NULL)
    dataSocket->SetSendAddress(addr,GetRemoteDataPort());
  appliedQOS = TRUE;
}


BOOL RTP_UDP::ModifyQOS(RTP_QOS * rtpqos)
{
  BOOL retval = FALSE;

  if (rtpqos == NULL)
    return retval;

  if (controlSocket != NULL)
    retval = controlSocket->ModifyQoSSpec(&(rtpqos->ctrlQoS));
    
  if (dataSocket != NULL)
    retval &= dataSocket->ModifyQoSSpec(&(rtpqos->dataQoS));

  appliedQOS = FALSE;
  return retval;
}

BOOL RTP_UDP::Open(PIPSocket::Address _localAddress,
                   WORD portBase, WORD portMax,
                   BYTE tos,
                   PSTUNClient * stun,
                   RTP_QOS * rtpQos)
{
  first = TRUE;
  // save local address 
  localAddress = _localAddress;

  localDataPort    = (WORD)(portBase&0xfffe);
  localControlPort = (WORD)(localDataPort + 1);

  delete dataSocket;
  delete controlSocket;
  dataSocket = NULL;
  controlSocket = NULL;

  byeSent = FALSE;

  PQoS * dataQos = NULL;
  PQoS * ctrlQos = NULL;
  if (rtpQos != NULL) {
    dataQos = &(rtpQos->dataQoS);
    ctrlQos = &(rtpQos->ctrlQoS);
  }

  // allow for special case of portBase == 0 or portMax == 0, which indicates a shared RTP session
  if ((portBase != 0) || (portMax != 0)) {
    if (stun != NULL) {
      if (stun->CreateSocketPair(dataSocket, controlSocket)) {
        dataSocket->GetLocalAddress(localAddress, localDataPort);
        controlSocket->GetLocalAddress(localAddress, localControlPort);
      }
      else {
        PTRACE(2, "RTP\tSTUN could not create RTP/RTCP socket pair; trying to create RTP socket anyway.");
        if (stun->CreateSocket(dataSocket)) {
          dataSocket->GetLocalAddress(localAddress, localDataPort);
        }
        else {
          PTRACE(1, "RTP\tSTUN could not create RTP socket either.");
          return FALSE;
        }
        if (stun->CreateSocket(controlSocket)) {
          controlSocket->GetLocalAddress(localAddress, localControlPort);
        }
      }
    }

    if (dataSocket == NULL || controlSocket == NULL) {
      dataSocket = new PUDPSocket(dataQos);
      controlSocket = new PUDPSocket(ctrlQos);
      while (!dataSocket->Listen(localAddress,    1, localDataPort) ||
             !controlSocket->Listen(localAddress, 1, localControlPort)) {
        dataSocket->Close();
        controlSocket->Close();
        if ((localDataPort > portMax) || (localDataPort > 0xfffd))
          return FALSE; // If it ever gets to here the OS has some SERIOUS problems!
        localDataPort    += 2;
        localControlPort += 2;
      }
    }

#   ifndef __BEOS__
    // Set the IP Type Of Service field for prioritisation of media UDP packets
    // through some Cisco routers and Linux boxes
    if (!dataSocket->SetOption(IP_TOS, tos, IPPROTO_IP)) {
      PTRACE(1, "RTP_UDP\tCould not set TOS field in IP header: " << dataSocket->GetErrorText());
    }

    // Increase internal buffer size on media UDP sockets
    SetMinBufferSize(*dataSocket,    SO_RCVBUF);
    SetMinBufferSize(*dataSocket,    SO_SNDBUF);
    SetMinBufferSize(*controlSocket, SO_RCVBUF);
    SetMinBufferSize(*controlSocket, SO_SNDBUF);
#   endif
  }

  shutdownRead = FALSE;
  shutdownWrite = FALSE;

  if (canonicalName.Find('@') == P_MAX_INDEX)
    canonicalName += '@' + GetLocalHostName();

  PTRACE(3, "RTP_UDP\tSession " << sessionID << " created: "
         << localAddress << ':' << localDataPort << '-' << localControlPort
         << " ssrc=" << syncSourceOut);
  

  return TRUE;
}


void RTP_UDP::Reopen(BOOL reading)
{
  if (reading)
    shutdownRead = FALSE;
  else
    shutdownWrite = FALSE;
}


void RTP_UDP::Close(BOOL reading)
{
  if (!shutdownRead && !shutdownWrite)
    SendBYE();

  if (reading) {
    if (!shutdownRead) {
      PTRACE(3, "RTP_UDP\tSession " << sessionID << ", Shutting down read.");
      syncSourceIn = 0;
      shutdownRead = TRUE;
      if (dataSocket != NULL && controlSocket != NULL) {
        PIPSocket::Address addr;
        controlSocket->GetLocalAddress(addr);
        if (addr.IsAny())
          PIPSocket::GetHostAddress(addr);
        dataSocket->WriteTo("", 1, addr, controlSocket->GetPort());
      }
    }
  }
  else {
    PTRACE(3, "RTP_UDP\tSession " << sessionID << ", Shutting down write.");
    shutdownWrite = TRUE;
  }
}


PString RTP_UDP::GetLocalHostName()
{
  return PIPSocket::GetHostName();
}


BOOL RTP_UDP::SetRemoteSocketInfo(PIPSocket::Address address, WORD port, BOOL isDataPort)
{
  if (remoteIsNAT) {
    PTRACE(2, "RTP_UDP\tIgnoring remote socket info as remote is behind NAT");
    return TRUE;
  }

  PTRACE(3, "RTP_UDP\tSetRemoteSocketInfo: session=" << sessionID << ' '
         << (isDataPort ? "data" : "control") << " channel, "
            "new=" << address << ':' << port << ", "
            "local=" << localAddress << ':' << localDataPort << '-' << localControlPort << ", "
            "remote=" << remoteAddress << ':' << remoteDataPort << '-' << remoteControlPort);

  if (localAddress == address && remoteAddress == address && (isDataPort ? localDataPort : localControlPort) == port)
    return TRUE;
  
  remoteAddress = address;
  
  allowSyncSourceInChange = TRUE;
  allowRemoteTransmitAddressChange = TRUE;
  allowSequenceChange = TRUE;

  if (isDataPort) {
    remoteDataPort = port;
    if (remoteControlPort == 0 || allowRemoteTransmitAddressChange)
      remoteControlPort = (WORD)(port + 1);
  }
  else {
    remoteControlPort = port;
    if (remoteDataPort == 0 || allowRemoteTransmitAddressChange)
      remoteDataPort = (WORD)(port - 1);
  }

  if (!appliedQOS)
      ApplyQOS(remoteAddress);
  
  return remoteAddress != 0 && port != 0;
}


BOOL RTP_UDP::ReadData(RTP_DataFrame & frame, BOOL loop)
{
  do {
    int selectStatus = WaitForPDU(*dataSocket, *controlSocket, reportTimer);

    if (shutdownRead) {
      PTRACE(3, "RTP_UDP\tSession " << sessionID << ", Read shutdown.");
      shutdownRead = FALSE;
      return FALSE;
    }

    switch (selectStatus) {
      case -2 :
        if (ReadControlPDU() == e_AbortTransport)
          return FALSE;
        break;

      case -3 :
        if (ReadControlPDU() == e_AbortTransport)
          return FALSE;
        // Then do -1 case

      case -1 :
        switch (ReadDataPDU(frame)) {
          case e_ProcessPacket :
            if (!shutdownRead)
              return TRUE;
          case e_IgnorePacket :
            break;
          case e_AbortTransport :
            return FALSE;
        }
        break;

      case 0 :
        if (!SendReport())
          return FALSE;
        break;

      case PSocket::Interrupted:
        PTRACE(2, "RTP_UDP\tSession " << sessionID << ", Interrupted.");
        return FALSE;

      default :
        PTRACE(1, "RTP_UDP\tSession " << sessionID << ", Select error: "
                << PChannel::GetErrorText((PChannel::Errors)selectStatus));
        return FALSE;
    }
  } while (loop);

  frame.SetSize(0);
  return TRUE;
}

int RTP_UDP::WaitForPDU(PUDPSocket & dataSocket, PUDPSocket & controlSocket, const PTimeInterval & timeout)
{
  if (first && (sessionID == 1)) {
    BYTE buffer[1500];
    PINDEX count = 0;
    do {
      switch (PSocket::Select(dataSocket, controlSocket, PTimeInterval(0))) {
        case -2:
          controlSocket.Read(buffer, sizeof(buffer));
          ++count;
          break;

        case -3:
          controlSocket.Read(buffer, sizeof(buffer));
          ++count;
        case -1:
          dataSocket.Read(buffer, sizeof(buffer));
          ++count;
          break;
        case 0:
          first = FALSE;
          break;
      }
    } while (first);
    PTRACE_IF(2, count > 0, "RTP_UDP\tSwallowed " << count << " RTP packets on startup");
  }

  return PSocket::Select(dataSocket, controlSocket, timeout);
}

RTP_Session::SendReceiveStatus RTP_UDP::ReadDataOrControlPDU(PUDPSocket & socket,
                                                             PBYTEArray & frame,
                                                             BOOL fromDataChannel)
{
#if PTRACING
  const char * channelName = fromDataChannel ? "Data" : "Control";
#endif
  PIPSocket::Address addr;
  WORD port;

  if (socket.ReadFrom(frame.GetPointer(), frame.GetSize(), addr, port)) {
    if (ignoreOtherSources) {
      // If remote address never set from higher levels, then try and figure
      // it out from the first packet received.
      if (!remoteAddress.IsValid()) {
        remoteAddress = addr;
        PTRACE(4, "RTP\tSet remote address from first " << channelName
               << " PDU from " << addr << ':' << port);
      }
      if (fromDataChannel) {
        if (remoteDataPort == 0)
          remoteDataPort = port;
      }
      else {
        if (remoteControlPort == 0)
          remoteControlPort = port;
      }

      if (!remoteTransmitAddress.IsValid())
        remoteTransmitAddress = addr;
      else if (allowRemoteTransmitAddressChange && remoteAddress == addr) {
        remoteTransmitAddress = addr;
        allowRemoteTransmitAddressChange = FALSE;
      }
      else if (remoteTransmitAddress != addr && !allowRemoteTransmitAddressChange && !ignoreOtherSources) {
        PTRACE(2, "RTP_UDP\tSession " << sessionID << ", "
               << channelName << " PDU from incorrect host, "
              " is " << addr << " should be " << remoteTransmitAddress);
        return RTP_Session::e_IgnorePacket;
      }
    }
    if (remoteAddress.IsValid() && !appliedQOS) 
      ApplyQOS(remoteAddress);

    return RTP_Session::e_ProcessPacket;
  }

  switch (socket.GetErrorNumber()) {
    case ECONNRESET :
    case ECONNREFUSED :
      PTRACE(2, "RTP_UDP\tSession " << sessionID << ", "
             << channelName << " port on remote not ready.");
      return RTP_Session::e_IgnorePacket;

    case EAGAIN :
      // Shouldn't happen, but it does.
      return RTP_Session::e_IgnorePacket;

    default:
      PTRACE(1, "RTP_UDP\t" << channelName << " read error ("
             << socket.GetErrorNumber(PChannel::LastReadError) << "): "
             << socket.GetErrorText(PChannel::LastReadError));
      return RTP_Session::e_AbortTransport;
  }
}


RTP_Session::SendReceiveStatus RTP_UDP::ReadDataPDU(RTP_DataFrame & frame)
{
  SendReceiveStatus status = ReadDataOrControlPDU(*dataSocket, frame, TRUE);
  if (status != e_ProcessPacket)
    return status;

  // Check received PDU is big enough
  PINDEX pduSize = dataSocket->GetLastReadCount();
  if (pduSize < RTP_DataFrame::MinHeaderSize || pduSize < frame.GetHeaderSize()) {
    PTRACE(2, "RTP_UDP\tSession " << sessionID
           << ", Received data packet too small: " << pduSize << " bytes");
    return e_IgnorePacket;
  }

  frame.SetPayloadSize(pduSize - frame.GetHeaderSize());
  return OnReceiveData(frame);
}


RTP_Session::SendReceiveStatus RTP_UDP::ReadControlPDU()
{
  RTP_ControlFrame frame(2048);

  SendReceiveStatus status = ReadDataOrControlPDU(*controlSocket, frame, FALSE);
  if (status != e_ProcessPacket)
    return status;

  PINDEX pduSize = controlSocket->GetLastReadCount();
  if (pduSize < 4 || pduSize < 4+frame.GetPayloadSize()) {
    PTRACE(2, "RTP_UDP\tSession " << sessionID
           << ", Received control packet too small: " << pduSize << " bytes");
    return e_IgnorePacket;
  }

  frame.SetSize(pduSize);
  return OnReceiveControl(frame);
}

BOOL RTP_UDP::WriteOOBData(RTP_DataFrame & frame)
{
  PWaitAndSignal m(sendDataMutex);

  // if media has not already established a timestamp, ensure that OnSendData does not use this one
  if (!timeStampOffsetEstablished) {
    timeStampOffs = 0;
    timeStampIsPremedia = TRUE;
  }

  // set the timestamp
  frame.SetTimestamp(timeStampOffs + timeStampOut + (PTimer::Tick() - lastSentPacketTime).GetInterval() * 8);

  // write the data
  BOOL stat = WriteData(frame);
  timeStampIsPremedia = FALSE;
  return stat;
}

BOOL RTP_UDP::WriteData(RTP_DataFrame & frame)
{
  if (shutdownWrite) {
    PTRACE(3, "RTP_UDP\tSession " << sessionID << ", Write shutdown.");
    shutdownWrite = FALSE;
    return FALSE;
  }

  // Trying to send a PDU before we are set up!
  if (!remoteAddress.IsValid() || remoteDataPort == 0)
    return TRUE;

  switch (OnSendData(frame)) {
    case e_ProcessPacket :
      break;
    case e_IgnorePacket :
      return TRUE;
    case e_AbortTransport :
      return FALSE;
  }

  while (!dataSocket->WriteTo(frame.GetPointer(),
                             frame.GetHeaderSize()+frame.GetPayloadSize(),
                             remoteAddress, remoteDataPort)) {
    switch (dataSocket->GetErrorNumber()) {
      case ECONNRESET :
      case ECONNREFUSED :
        PTRACE(2, "RTP_UDP\tSession " << sessionID << ", data port on remote not ready.");
        break;

      default:
        PTRACE(1, "RTP_UDP\tSession " << sessionID
               << ", Write error on data port ("
               << dataSocket->GetErrorNumber(PChannel::LastWriteError) << "): "
               << dataSocket->GetErrorText(PChannel::LastWriteError));
        return FALSE;
    }
  }

  return TRUE;
}


BOOL RTP_UDP::WriteControl(RTP_ControlFrame & frame)
{
  // Trying to send a PDU before we are set up!
  if (!remoteAddress.IsValid() || remoteControlPort == 0 || controlSocket == NULL)
    return TRUE;

  PINDEX len = frame.GetCompoundSize();
  switch (OnSendControl(frame, len)) {
    case e_ProcessPacket :
      break;
    case e_IgnorePacket :
      return TRUE;
    case e_AbortTransport :
      return FALSE;
  }

  while (!controlSocket->WriteTo(frame.GetPointer(), len,
                                remoteAddress, remoteControlPort)) {
    switch (controlSocket->GetErrorNumber()) {
      case ECONNRESET :
      case ECONNREFUSED :
        PTRACE(2, "RTP_UDP\tSession " << sessionID << ", control port on remote not ready.");
        break;

      default:
        PTRACE(1, "RTP_UDP\tSession " << sessionID
               << ", Write error on control port ("
               << controlSocket->GetErrorNumber(PChannel::LastWriteError) << "): "
               << controlSocket->GetErrorText(PChannel::LastWriteError));
        return FALSE;
    }
  }

  return TRUE;
}

#if OPAL_VIDEO
void RTP_Session::SendIntraFrameRequest(){
    // Create packet
    RTP_ControlFrame request;
    request.StartNewPacket();
    request.SetPayloadType(RTP_ControlFrame::e_IntraFrameRequest);
    request.SetPayloadSize(4);
    // Insert SSRC
    request.SetCount(1);
    BYTE * payload = request.GetPayloadPtr();
    *(PUInt32b *)payload = syncSourceOut;
    // Send it
    request.EndPacket();
    WriteControl(request);
}
#endif

/////////////////////////////////////////////////////////////////////////////

SecureRTP_UDP::SecureRTP_UDP(PHandleAggregator * _aggregator, unsigned id, BOOL remoteIsNAT)
  : RTP_UDP(_aggregator, id, remoteIsNAT)

{
  securityParms = NULL;
}

SecureRTP_UDP::~SecureRTP_UDP()
{
  delete securityParms;
}

void SecureRTP_UDP::SetSecurityMode(OpalSecurityMode * newParms)
{ 
  if (securityParms != NULL)
    delete securityParms;
  securityParms = newParms; 
}
  
OpalSecurityMode * SecureRTP_UDP::GetSecurityParms() const
{ 
  return securityParms; 
}

/////////////////////////////////////////////////////////////////////////////
