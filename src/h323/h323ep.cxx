/*
 * h323ep.cxx
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
 * $Log: h323ep.cxx,v $
 * Revision 1.2037  2005/07/11 01:52:24  csoutheren
 * Extended AnsweringCall to work for SIP as well as H.323
 * Fixed problems with external RTP connection in H.323
 * Added call to OnClosedMediaStream
 *
 * Revision 2.35  2004/11/07 12:30:12  rjongbloed
 * Minor optimisation
 *
 * Revision 2.34  2004/09/24 10:06:22  rjongbloed
 * Changed default endpoint capabilities to be all possible capabilities if not
 *   set explicitly by application before first call.
 *
 * Revision 2.33  2004/08/14 07:56:31  rjongbloed
 * Major revision to utilise the PSafeCollection classes for the connections and calls.
 *
 * Revision 2.32  2004/07/11 12:42:12  rjongbloed
 * Added function on endpoints to get the list of all media formats any
 *   connection the endpoint may create can support.
 *
 * Revision 2.31  2004/06/05 14:32:32  rjongbloed
 * Added ability to have separate gatekeeper username to endpoint local alias name.
 *
 * Revision 2.30  2004/06/04 06:54:18  csoutheren
 * Migrated updates from OpenH323 1.14.1
 *
 * Revision 2.29  2004/04/07 08:21:03  rjongbloed
 * Changes for new RTTI system.
 *
 * Revision 2.28  2004/03/13 06:25:54  rjongbloed
 * Slight rearrangement of local party name and alias list to beter match common
 *   behaviour in ancestor.
 * Abstracted local party name for endpoint into ancestor from H.,323.
 *
 * Revision 2.27  2004/02/19 10:47:04  rjongbloed
 * Merged OpenH323 version 1.13.1 changes.
 *
 * Revision 2.26  2003/03/07 08:12:01  robertj
 * Removed lock not needed in new architecture, hang over from OpenH323.
 *
 * Revision 2.25  2003/03/06 03:57:47  robertj
 * IVR support (work in progress) requiring large changes everywhere.
 *
 * Revision 2.24  2003/01/24 11:31:01  robertj
 * Fixed correct UDP address being used for gatkeeper discovery, thanks Chih-Wei Huang
 *
 * Revision 2.23  2003/01/07 04:39:53  robertj
 * Updated to OpenH323 v1.11.2
 *
 * Revision 2.22  2002/11/10 11:33:19  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.21  2002/09/06 02:40:27  robertj
 * Added ability to specify stream or datagram (TCP or UDP) transport is to
 * be created from a transport address regardless of the addresses mode.
 *
 * Revision 2.20  2002/09/04 06:01:48  robertj
 * Updated to OpenH323 v1.9.6
 *
 * Revision 2.19  2002/07/04 07:41:47  robertj
 * Fixed memory/thread leak of transports.
 *
 * Revision 2.18  2002/07/01 04:56:32  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 2.17  2002/03/27 04:02:01  robertj
 * Fixed correct pasing of arguments for starting outgoing call.
 *
 * Revision 2.16  2002/03/18 00:33:36  robertj
 * Removed duplicate initialBandwidth variable in H.323 class, moved to ancestor.
 *
 * Revision 2.15  2002/02/11 09:32:13  robertj
 * Updated to openH323 v1.8.0
 *
 * Revision 2.14  2002/01/22 05:28:27  robertj
 * Revamp of user input API triggered by RFC2833 support
 * Update from OpenH323
 *
 * Revision 2.13  2002/01/14 06:35:58  robertj
 * Updated to OpenH323 v1.7.9
 *
 * Revision 2.12  2001/11/13 06:25:56  robertj
 * Changed SetUpConnection() so returns BOOL as returning
 *   pointer to connection is not useful.
 *
 * Revision 2.11  2001/11/13 04:29:48  robertj
 * Changed OpalTransportAddress CreateTransport and CreateListsner functions
 *   to have extra parameter to control local binding of sockets.
 *
 * Revision 2.10  2001/11/12 05:32:12  robertj
 * Added OpalTransportAddress::GetIpAddress when don't need port number.
 *
 * Revision 2.9  2001/11/02 10:45:19  robertj
 * Updated to OpenH323 v1.7.3
 *
 * Revision 2.8  2001/10/15 04:35:44  robertj
 * Removed answerCall signal and replaced with state based functions.
 * Maintained H.323 answerCall API for backward compatibility.
 *
 * Revision 2.7  2001/10/05 00:22:14  robertj
 * Updated to PWLib 1.2.0 and OpenH323 1.7.0
 *
 * Revision 2.6  2001/08/23 03:15:51  robertj
 * Added missing Lock() calls in SetUpConnection
 *
 * Revision 2.5  2001/08/22 10:20:09  robertj
 * Changed connection locking to use double mutex to guarantee that
 *   no threads can ever deadlock or access deleted connection.
 *
 * Revision 2.4  2001/08/17 08:27:44  robertj
 * Update from OpenH323
 * Moved call end reasons enum from OpalConnection to global.
 *
 * Revision 2.3  2001/08/13 05:10:39  robertj
 * Updates from OpenH323 v1.6.0 release.
 *
 * Revision 2.2  2001/08/01 05:14:19  robertj
 * Moved media formats list from endpoint to connection.
 * Fixed accept of call to early enough to set connections capabilities.
 *
 * Revision 2.1  2001/07/30 07:22:25  robertj
 * Abstracted listener management from H.323 to OpalEndPoint class.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.170  2004/01/26 11:42:36  rjongbloed
 * Added pass through of port numbers to STUN client
 *
 * Revision 1.169  2003/12/29 04:59:25  csoutheren
 * Added callbacks on H323EndPoint when gatekeeper discovery succeeds or fails
 *
 * Revision 1.168  2003/12/28 02:37:49  csoutheren
 * Added H323EndPoint::OnOutgoingCall
 *
 * Revision 1.167  2003/12/28 00:06:51  csoutheren
 * Added callbacks on H323EndPoint when gatekeeper registration succeeds or fails
 *
 * Revision 1.166  2003/04/28 09:01:15  robertj
 * Fixed problem with backward compatibility with non-url based remote
 *   addresses passed to MakeCall()
 *
 * Revision 1.165  2003/04/24 01:49:33  dereks
 * Add ability to set no media timeout interval
 *
 * Revision 1.164  2003/04/10 09:41:26  robertj
 * Added associated transport to new GetInterfaceAddresses() function so
 *   interfaces can be ordered according to active transport links. Improves
 *   interoperability.
 *
 * Revision 1.163  2003/04/10 01:01:56  craigs
 * Added functions to access to lists of interfaces
 *
 * Revision 1.162  2003/04/07 13:09:30  robertj
 * Added ILS support to callto URL parsing in MakeCall(), ie can now call hosts
 *   registered with an ILS directory.
 *
 * Revision 1.161  2003/04/07 11:11:45  craigs
 * Fixed compile problem on Linux
 *
 * Revision 1.160  2003/04/04 08:04:35  robertj
 * Added support for URL's in MakeCall, especially h323 and callto schemes.
 *
 * Revision 1.159  2003/03/04 03:58:32  robertj
 * Fixed missing local interface usage if specified in UseGatekeeper()
 *
 * Revision 1.158  2003/02/28 09:00:37  rogerh
 * remove redundant code
 *
 * Revision 1.157  2003/02/25 23:51:49  robertj
 * Fxied bug where not getting last port in range, thanks Sonya Cooper-Hull
 *
 * Revision 1.156  2003/02/09 00:48:09  robertj
 * Added function to return if registered with gatekeeper.
 *
 * Revision 1.155  2003/02/05 06:32:10  robertj
 * Fixed non-stun symmetric NAT support recently broken.
 *
 * Revision 1.154  2003/02/05 04:56:35  robertj
 * Fixed setting STUN server to enpty string clearing stun variable.
 *
 * Revision 1.153  2003/02/04 07:06:41  robertj
 * Added STUN support.
 *
 * Revision 1.152  2003/02/01 13:31:22  robertj
 * Changes to support CAT authentication in RAS.
 *
 * Revision 1.151  2003/01/26 05:57:29  robertj
 * Changed ParsePartyName so will accept addresses of the form
 *   alias@gk:address which will do an LRQ call to "address" using "alias"
 *   to determine the IP address to connect to.
 *
 * Revision 1.150  2003/01/23 02:36:32  robertj
 * Increased (and made configurable) timeout for H.245 channel TCP connection.
 *
 * Revision 1.149  2003/01/06 06:13:37  robertj
 * Increased maximum possible jitter configuration to 10 seconds.
 *
 * Revision 1.148  2002/11/27 06:54:57  robertj
 * Added Service Control Session management as per Annex K/H.323 via RAS
 *   only at this stage.
 * Added H.248 ASN and very primitive infrastructure for linking into the
 *   Service Control Session management system.
 * Added basic infrastructure for Annex K/H.323 HTTP transport system.
 * Added Call Credit Service Control to display account balances.
 *
 * Revision 1.147  2002/11/19 07:07:44  robertj
 * Changed priority so H.235 standard authentication used by preference.
 *
 * Revision 1.146  2002/11/15 06:53:24  robertj
 * Fixed non facility redirect calls being able to be cleared!
 *
 * Revision 1.145  2002/11/15 05:17:26  robertj
 * Added facility redirect support without changing the call token for access
 *   to the call. If it gets redirected a new H323Connection object is
 *   created but it looks like the same thing to an application.
 *
 * Revision 1.144  2002/11/14 22:06:08  robertj
 * Increased default maximum number of ports.
 *
 * Revision 1.143  2002/11/10 08:10:43  robertj
 * Moved constants for "well known" ports to better place (OPAL change).
 *
 * Revision 1.142  2002/10/31 00:42:41  robertj
 * Enhanced jitter buffer system so operates dynamically between minimum and
 *   maximum values. Altered API to assure app writers note the change!
 *
 * Revision 1.141  2002/10/24 07:18:24  robertj
 * Changed gatekeeper call so if can do local DNS lookup or using IP address
 *   then uses destCallSignalAddress for ARQ instead of destinationInfo.
 *
 * Revision 1.140  2002/10/23 06:06:13  robertj
 * Added function to be smarter in using a gatekeeper for use by endpoint.
 *
 * Revision 1.139  2002/10/01 06:38:36  robertj
 * Removed GNU compiler warning
 *
 * Revision 1.138  2002/10/01 03:07:15  robertj
 * Added version number functions for OpenH323 library itself, plus included
 *   library version in the default vendor information.
 *
 * Revision 1.137  2002/08/15 04:56:56  robertj
 * Fixed operation of ports system assuring RTP is even numbers.
 *
 * Revision 1.136  2002/08/05 10:03:47  robertj
 * Cosmetic changes to normalise the usage of pragma interface/implementation.
 *
 * Revision 1.135  2002/07/19 11:25:10  robertj
 * Added extra trace on attempt to clear non existent call.
 *
 * Revision 1.134  2002/07/19 03:39:22  robertj
 * Bullet proofed setting of RTP IP port base, can't be zero!
 *
 * Revision 1.133  2002/07/18 01:50:14  robertj
 * Changed port secltion code to force apps to use function interface.
 *
 * Revision 1.132  2002/07/04 00:11:25  robertj
 * Fixed setting of gk password when password changed in endpoint.
 *
 * Revision 1.131  2002/06/22 05:48:42  robertj
 * Added partial implementation for H.450.11 Call Intrusion
 *
 * Revision 1.130  2002/06/15 03:06:46  robertj
 * Fixed locking of connection on H.250.2 transfer, thanks Gilles Delcayre
 *
 * Revision 1.129  2002/06/13 06:15:20  robertj
 * Allowed TransferCall() to be used on H323Connection as well as H323EndPoint.
 *
 * Revision 1.128  2002/06/12 03:55:21  robertj
 * Added function to add/remove multiple listeners in one go comparing against
 *   what is already running so does not interrupt unchanged listeners.
 *
 * Revision 1.127  2002/05/29 06:40:33  robertj
 * Changed sending of endSession/ReleaseComplete PDU's to occur immediately
 *   on call clearance and not wait for background thread to do it.
 * Stricter compliance by waiting for reply endSession before closing down.
 *
 * Revision 1.126  2002/05/28 06:16:12  robertj
 * Split UDP (for RAS) from RTP port bases.
 * Added current port variable so cycles around the port range specified which
 *   fixes some wierd problems on some platforms, thanks Federico Pinna
 *
 * Revision 1.125  2002/05/17 03:39:01  robertj
 * Fixed problems with H.235 authentication on RAS for server and client.
 *
 * Revision 1.124  2002/05/15 23:57:49  robertj
 * Added function to get the tokens for all active calls.
 * Fixed setting of password info in gatekeeper so is before discovery.
 *
 * Revision 1.123  2002/05/02 07:56:28  robertj
 * Added automatic clearing of call if no media (RTP data) is transferred in a
 *   configurable (default 5 minutes) amount of time.
 *
 * Revision 1.122  2002/04/18 01:40:56  robertj
 * Fixed bad variable name for disabling DTMF detection, very confusing.
 *
 * Revision 1.121  2002/04/17 00:50:57  robertj
 * Added ability to disable the in band DTMF detection.
 *
 * Revision 1.120  2002/04/10 08:09:03  robertj
 * Allowed zero TCP ports
 *
 * Revision 1.119  2002/04/10 06:50:08  robertj
 * Added functions to set port member variables.
 *
 * Revision 1.118  2002/04/01 21:32:24  robertj
 * Fixed problem with busy loop on Solaris, thanks chad@broadmind.com
 *
 * Revision 1.117  2002/02/04 07:17:56  robertj
 * Added H.450.2 Consultation Transfer, thanks Norwood Systems.
 *
 * Revision 1.116  2002/01/24 06:29:06  robertj
 * Added option to disable H.245 negotiation in SETUP pdu, this required
 *   API change so have a bit mask instead of a series of booleans.
 *
 * Revision 1.115  2002/01/17 07:05:04  robertj
 * Added support for RFC2833 embedded DTMF in the RTP stream.
 *
 * Revision 1.114  2002/01/14 00:00:04  robertj
 * Added CallTransfer timeouts to endpoint, hanks Ben Madsen of Norwood Systems.
 *
 * Revision 1.113  2002/01/08 04:45:04  robertj
 * Added MakeCallLocked() so can start a call with the H323Connection instance
 *   initally locked so can do things to it before the call really starts.
 *
 * Revision 1.112  2001/12/22 03:20:05  robertj
 * Added create protocol function to H323Connection.
 *
 * Revision 1.111  2001/12/14 08:36:36  robertj
 * More implementation of T.38, thanks Adam Lazur
 *
 * Revision 1.110  2001/12/13 11:01:37  robertj
 * Fixed missing initialisation of auto fax start variables.
 *
 * Revision 1.109  2001/11/01 06:11:57  robertj
 * Plugged very small mutex hole that could cause crashes.
 *
 * Revision 1.108  2001/11/01 00:59:07  robertj
 * Added auto setting of silence detect mode when using PSoundChannel codec.
 *
 * Revision 1.107  2001/11/01 00:27:35  robertj
 * Added default Fast Start disabled and H.245 tunneling disable flags
 *   to the endpoint instance.
 *
 * Revision 1.106  2001/10/30 07:34:33  robertj
 * Added trace for when cleaner thread start, stops and runs
 *
 * Revision 1.105  2001/10/24 07:25:59  robertj
 * Fixed possible deadlocks during destruction of H323Connection object.
 *
 * Revision 1.104  2001/09/26 06:20:59  robertj
 * Fixed properly nesting connection locking and unlocking requiring a quite
 *   large change to teh implementation of how calls are answered.
 *
 * Revision 1.103  2001/09/13 02:41:21  robertj
 * Fixed call reference generation to use full range and common code.
 *
 * Revision 1.102  2001/09/11 01:24:36  robertj
 * Added conditional compilation to remove video and/or audio codecs.
 *
 * Revision 1.101  2001/09/11 00:21:23  robertj
 * Fixed missing stack sizes in endpoint for cleaner thread and jitter thread.
 *
 * Revision 1.100  2001/08/24 13:38:25  rogerh
 * Free the locally declared listener.
 *
 * Revision 1.99  2001/08/24 13:23:59  rogerh
 * Undo a recent memory leak fix. The listener object has to be deleted in the
 * user application as they use the listener when StartListener() fails.
 * Add a Resume() if StartListener() fails so user applications can delete
 * the suspended thread.
 *
 * Revision 1.98  2001/08/22 06:54:51  robertj
 * Changed connection locking to use double mutex to guarantee that
 *   no threads can ever deadlock or access deleted connection.
 *
 * Revision 1.97  2001/08/16 07:49:19  robertj
 * Changed the H.450 support to be more extensible. Protocol handlers
 *   are now in separate classes instead of all in H323Connection.
 *
 * Revision 1.96  2001/08/10 11:03:52  robertj
 * Major changes to H.235 support in RAS to support server.
 *
 * Revision 1.95  2001/08/08 23:55:27  robertj
 * Fixed problem with setting gk password before have a gk variable.
 * Fixed memory leak if listener fails to open.
 * Fixed problem with being able to specify an alias with an @ in it.
 *
 * Revision 1.94  2001/08/06 07:44:55  robertj
 * Fixed problems with building without SSL
 *
 * Revision 1.93  2001/08/06 03:08:56  robertj
 * Fission of h323.h to h323ep.h & h323con.h, h323.h now just includes files.
 *
 * Revision 1.92  2001/08/02 04:31:48  robertj
 * Changed to still maintain gatekeeper link if GRQ succeeded but RRQ
 *   failed. Thanks Ben Madsen & Graeme Reid.
 *
 * Revision 1.91  2001/07/17 04:44:32  robertj
 * Partial implementation of T.120 and T.38 logical channels.
 *
 * Revision 1.90  2001/07/06 07:28:55  robertj
 * Fixed rearranged connection creation to avoid possible nested mutex.
 *
 * Revision 1.89  2001/07/06 04:46:19  robertj
 * Rearranged connection creation to avoid possible nested mutex.
 *
 * Revision 1.88  2001/06/19 08:43:38  robertj
 * Fixed deadlock if ClearCallSynchronous ever called from cleaner thread.
 *
 * Revision 1.87  2001/06/19 03:55:30  robertj
 * Added transport to CreateConnection() function so can use that as part of
 *   the connection creation decision making process.
 *
 * Revision 1.86  2001/06/15 00:55:53  robertj
 * Added thread name for cleaner thread
 *
 * Revision 1.85  2001/06/14 23:18:06  robertj
 * Change to allow for CreateConnection() to return NULL to abort call.
 *
 * Revision 1.84  2001/06/14 04:23:32  robertj
 * Changed incoming call to pass setup pdu to endpoint so it can create
 *   different connection subclasses depending on the pdu eg its alias
 *
 * Revision 1.83  2001/06/02 01:35:32  robertj
 * Added thread names.
 *
 * Revision 1.82  2001/05/30 11:13:54  robertj
 * Fixed possible deadlock when getting connection from endpoint.
 *
 * Revision 1.81  2001/05/14 05:56:28  robertj
 * Added H323 capability registration system so can add capabilities by
 *   string name instead of having to instantiate explicit classes.
 *
 * Revision 1.80  2001/05/01 04:34:11  robertj
 * Changed call transfer API slightly to be consistent with new hold function.
 *
 * Revision 1.79  2001/04/23 01:31:15  robertj
 * Improved the locking of connections especially at shutdown.
 *
 * Revision 1.78  2001/04/11 03:01:29  robertj
 * Added H.450.2 (call transfer), thanks a LOT to Graeme Reid & Norwood Systems
 *
 * Revision 1.77  2001/03/21 04:52:42  robertj
 * Added H.235 security to gatekeepers, thanks Fürbass Franz!
 *
 * Revision 1.76  2001/03/17 00:05:52  robertj
 * Fixed problems with Gatekeeper RIP handling.
 *
 * Revision 1.75  2001/03/15 00:24:47  robertj
 * Added function for setting gatekeeper with address and identifier values.
 *
 * Revision 1.74  2001/02/27 00:03:59  robertj
 * Fixed possible deadlock in FindConnectionsWithLock(), thanks Hans Andersen
 *
 * Revision 1.73  2001/02/16 04:11:35  robertj
 * Added ability for RemoveListener() to remove all listeners.
 *
 * Revision 1.72  2001/01/18 06:04:18  robertj
 * Bullet proofed code so local alias can not be empty string. This actually
 *   fixes an ASN PER encoding bug causing an assert.
 *
 * Revision 1.71  2001/01/11 02:19:15  craigs
 * Fixed problem with possible window of opportunity for ClearCallSynchronous
 * to return even though call has not been cleared
 *
 * Revision 1.70  2000/12/22 03:54:33  craigs
 * Fixed problem with listening fix
 *
 * Revision 1.69  2000/12/21 12:38:24  craigs
 * Fixed problem with "Listener thread did not terminate" assert
 * when listener port is in use
 *
 * Revision 1.68  2000/12/20 00:51:03  robertj
 * Fixed MSVC compatibility issues (No trace).
 *
 * Revision 1.67  2000/12/19 22:33:44  dereks
 * Adjust so that the video channel is used for reading/writing raw video
 * data, which better modularizes the video codec.
 *
 * Revision 1.66  2000/12/18 08:59:20  craigs
 * Added ability to set ports
 *
 * Revision 1.65  2000/12/18 01:22:28  robertj
 * Changed semantics or HasConnection() so returns TRUE until the connection
 *   has been deleted and not just until ClearCall() was executure on it.
 *
 * Revision 1.64  2000/11/27 02:44:06  craigs
 * Added ClearCall Synchronous to H323Connection and H323Endpoint to
 * avoid race conditions with destroying descendant classes
 *
 * Revision 1.63  2000/11/26 23:13:23  craigs
 * Added ability to pass user data to H323Connection constructor
 *
 * Revision 1.62  2000/11/12 23:49:16  craigs
 * Added per connection versions of OnEstablished and OnCleared
 *
 * Revision 1.61  2000/10/25 00:53:52  robertj
 * Used official manafacturer code for the OpenH323 project.
 *
 * Revision 1.60  2000/10/20 06:10:51  robertj
 * Fixed very small race condition on creating new connectionon incoming call.
 *
 * Revision 1.59  2000/10/19 04:07:50  robertj
 * Added function to be able to remove a listener.
 *
 * Revision 1.58  2000/10/04 12:21:07  robertj
 * Changed setting of callToken in H323Connection to be as early as possible.
 *
 * Revision 1.57  2000/09/25 12:59:34  robertj
 * Added StartListener() function that takes a H323TransportAddress to start
 *     listeners bound to specific interfaces.
 *
 * Revision 1.56  2000/09/14 23:03:45  robertj
 * Increased timeout on asserting because of driver lockup
 *
 * Revision 1.55  2000/09/01 02:13:05  robertj
 * Added ability to select a gatekeeper on LAN via it's identifier name.
 *
 * Revision 1.54  2000/08/30 05:37:44  robertj
 * Fixed bogus destCallSignalAddress in ARQ messages.
 *
 * Revision 1.53  2000/08/29 02:59:50  robertj
 * Added some debug output for channel open/close.
 *
 * Revision 1.52  2000/08/25 01:10:28  robertj
 * Added assert if various thrads ever fail to terminate.
 *
 * Revision 1.51  2000/07/13 12:34:41  robertj
 * Split autoStartVideo so can select receive and transmit independently
 *
 * Revision 1.50  2000/07/11 19:30:15  robertj
 * Fixed problem where failure to unregister from gatekeeper prevented new registration.
 *
 * Revision 1.49  2000/07/09 14:59:28  robertj
 * Fixed bug if using default transport (no '@') for address when gatekeeper present, used port 1719.
 *
 * Revision 1.48  2000/07/04 04:15:40  robertj
 * Fixed capability check of "combinations" for fast start cases.
 *
 * Revision 1.47  2000/07/04 01:16:50  robertj
 * Added check for capability allowed in "combinations" set, still needs more done yet.
 *
 * Revision 1.46  2000/06/29 11:00:04  robertj
 * Added user interface for sound buffer depth adjustment.
 *
 * Revision 1.45  2000/06/29 07:10:32  robertj
 * Fixed incorrect setting of default number of audio buffers on Win32 systems.
 *
 * Revision 1.44  2000/06/23 02:48:24  robertj
 * Added ability to adjust sound channel buffer depth, needed increasing under Win32.
 *
 * Revision 1.43  2000/06/20 02:38:28  robertj
 * Changed H323TransportAddress to default to IP.
 *
 * Revision 1.42  2000/06/17 09:13:06  robertj
 * Removed redundent line of code, thanks David Iodice.
 *
 * Revision 1.41  2000/06/07 05:48:06  robertj
 * Added call forwarding.
 *
 * Revision 1.40  2000/06/03 03:16:39  robertj
 * Fixed using the wrong capability table (should be connections) for some operations.
 *
 * Revision 1.39  2000/05/25 00:34:59  robertj
 * Changed default tos on Unix platforms to avoid needing to be root.
 *
 * Revision 1.38  2000/05/23 12:57:37  robertj
 * Added ability to change IP Type Of Service code from applications.
 *
 * Revision 1.37  2000/05/23 11:32:37  robertj
 * Rewrite of capability table to combine 2 structures into one and move functionality into that class
 *    allowing some normalisation of usage across several applications.
 * Changed H323Connection so gets a copy of capabilities instead of using endponts, allows adjustments
 *    to be done depending on the remote client application.
 *
 * Revision 1.36  2000/05/16 07:38:50  robertj
 * Added extra debug info indicating sound channel buffer size.
 *
 * Revision 1.35  2000/05/11 03:48:11  craigs
 * Moved SetBuffer command to fix audio delay
 *
 * Revision 1.34  2000/05/02 04:32:26  robertj
 * Fixed copyright notice comment.
 *
 * Revision 1.33  2000/05/01 13:00:28  robertj
 * Changed SetCapability() to append capabilities to TCS, helps with assuring no gaps in set.
 *
 * Revision 1.32  2000/04/30 03:58:37  robertj
 * Changed PSoundChannel to be only double bufferred, this is all that is needed with jitter bufferring.
 *
 * Revision 1.31  2000/04/11 04:02:48  robertj
 * Improved call initiation with gatekeeper, no longer require @address as
 *    will default to gk alias if no @ and registered with gk.
 * Added new call end reasons for gatekeeper denied calls.
 *
 * Revision 1.30  2000/04/10 20:37:33  robertj
 * Added support for more sophisticated DTMF and hook flash user indication.
 *
 * Revision 1.29  2000/04/06 17:50:17  robertj
 * Added auto-start (including fast start) of video channels, selectable via boolean on the endpoint.
 *
 * Revision 1.28  2000/04/05 03:17:31  robertj
 * Added more RTP statistics gathering and H.245 round trip delay calculation.
 *
 * Revision 1.27  2000/03/29 02:14:45  robertj
 * Changed TerminationReason to CallEndReason to use correct telephony nomenclature.
 * Added CallEndReason for capability exchange failure.
 *
 * Revision 1.26  2000/03/25 02:03:36  robertj
 * Added default transport for gatekeeper to be UDP.
 *
 * Revision 1.25  2000/03/23 02:45:29  robertj
 * Changed ClearAllCalls() so will wait for calls to be closed (usefull in endpoint dtors).
 *
 * Revision 1.24  2000/02/17 12:07:43  robertj
 * Used ne wPWLib random number generator after finding major problem in MSVC rand().
 *
 * Revision 1.23  2000/01/07 08:22:49  robertj
 * Added status functions for connection and added transport independent MakeCall
 *
 * Revision 1.22  1999/12/23 23:02:36  robertj
 * File reorganision for separating RTP from H.323 and creation of LID for VPB support.
 *
 * Revision 1.21  1999/12/11 02:21:00  robertj
 * Added ability to have multiple aliases on local endpoint.
 *
 * Revision 1.20  1999/12/09 23:14:20  robertj
 * Fixed deadock in termination with gatekeeper removal.
 *
 * Revision 1.19  1999/11/19 08:07:27  robertj
 * Changed default jitter time to 50 milliseconds.
 *
 * Revision 1.18  1999/11/06 05:37:45  robertj
 * Complete rewrite of termination of connection to avoid numerous race conditions.
 *
 * Revision 1.17  1999/10/30 12:34:47  robertj
 * Added information callback for closed logical channel on H323EndPoint.
 *
 * Revision 1.16  1999/10/19 00:04:57  robertj
 * Changed OpenAudioChannel and OpenVideoChannel to allow a codec AttachChannel with no autodelete.
 *
 * Revision 1.15  1999/10/14 12:05:03  robertj
 * Fixed deadlock possibilities in clearing calls.
 *
 * Revision 1.14  1999/10/09 01:18:23  craigs
 * Added codecs to OpenAudioChannel and OpenVideoDevice functions
 *
 * Revision 1.13  1999/10/08 08:31:45  robertj
 * Fixed failure to adjust capability when startign channel
 *
 * Revision 1.12  1999/09/23 07:25:12  robertj
 * Added open audio and video function to connection and started multi-frame codec send functionality.
 *
 * Revision 1.11  1999/09/21 14:24:48  robertj
 * Changed SetCapability() so automatically calls AddCapability().
 *
 * Revision 1.10  1999/09/21 14:12:40  robertj
 * Removed warnings when no tracing enabled.
 *
 * Revision 1.9  1999/09/21 08:10:03  craigs
 * Added support for video devices and H261 codec
 *
 * Revision 1.8  1999/09/14 06:52:54  robertj
 * Added better support for multi-homed client hosts.
 *
 * Revision 1.7  1999/09/13 14:23:11  robertj
 * Changed MakeCall() function return value to be something useful.
 *
 * Revision 1.6  1999/09/10 02:55:36  robertj
 * Changed t35 country code to Australia (finally found magic number).
 *
 * Revision 1.5  1999/09/08 04:05:49  robertj
 * Added support for video capabilities & codec, still needs the actual codec itself!
 *
 * Revision 1.4  1999/08/31 12:34:19  robertj
 * Added gatekeeper support.
 *
 * Revision 1.3  1999/08/27 15:42:44  craigs
 * Fixed problem with local call tokens using ambiguous interface names, and connect timeouts not changing connection state
 *
 * Revision 1.2  1999/08/27 09:46:05  robertj
 * Added sepearte function to initialise vendor information from endpoint.
 *
 * Revision 1.1  1999/08/25 05:10:36  robertj
 * File fission (critical mass reached).
 * Improved way in which remote capabilities are created, removed case statement!
 * Changed MakeCall, so immediately spawns thread, no black on TCP connect.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "h323ep.h"
#endif

#include <h323/h323ep.h>

#include <ptclib/random.h>
#include <opal/call.h>
#include <h323/h323pdu.h>
#include <h323/gkclient.h>
#include <ptclib/url.h>
#include <ptclib/pils.h>


#define new PNEW

#if !PTRACING // Stuff to remove unised parameters warning
#define PTRACE_isEncoding
#define PTRACE_channel
#endif

BYTE H323EndPoint::defaultT35CountryCode    = 9; // Country code for Australia
BYTE H323EndPoint::defaultT35Extension      = 0;
WORD H323EndPoint::defaultManufacturerCode  = 61; // Allocated by Australian Communications Authority, Oct 2000;

/////////////////////////////////////////////////////////////////////////////

H323EndPoint::H323EndPoint(OpalManager & manager)
  : OpalEndPoint(manager, "h323", CanTerminateCall),
    signallingChannelCallTimeout(0, 0, 1),  // Minutes
    controlChannelStartTimeout(0, 0, 2),    // Minutes
    endSessionTimeout(0, 10),               // Seconds
    masterSlaveDeterminationTimeout(0, 30), // Seconds
    capabilityExchangeTimeout(0, 30),       // Seconds
    logicalChannelTimeout(0, 30),           // Seconds
    requestModeTimeout(0, 30),              // Seconds
    roundTripDelayTimeout(0, 10),           // Seconds
    roundTripDelayRate(0, 0, 1),            // Minutes
    gatekeeperRequestTimeout(0, 5),         // Seconds
    rasRequestTimeout(0, 3),                // Seconds
    callTransferT1(0,10),                   // Seconds
    callTransferT2(0,10),                   // Seconds
    callTransferT3(0,10),                   // Seconds
    callTransferT4(0,10),                   // Seconds
    callIntrusionT1(0,30),                  // Seconds
    callIntrusionT2(0,30),                  // Seconds
    callIntrusionT3(0,30),                  // Seconds
    callIntrusionT4(0,30),                  // Seconds
    callIntrusionT5(0,10),                  // Seconds
    callIntrusionT6(0,10)                   // Seconds
{
  // Set port in OpalEndPoint class
  defaultSignalPort = DefaultTcpPort;

  localAliasNames.AppendString(defaultLocalPartyName);

  autoStartReceiveFax = autoStartTransmitFax = FALSE;

  autoCallForward = TRUE;
  disableFastStart = FALSE;
  disableH245Tunneling = FALSE;
  disableH245inSetup = FALSE;
  canDisplayAmountString = FALSE;
  canEnforceDurationLimit = TRUE;
  callIntrusionProtectionLevel = 3; //H45011_CIProtectionLevel::e_fullProtection;
  defaultSendUserInputMode = H323Connection::SendUserInputAsString;

  terminalType = e_TerminalOnly;
  clearCallOnRoundTripFail = FALSE;

  t35CountryCode   = defaultT35CountryCode;   // Country code for Australia
  t35Extension     = defaultT35Extension;
  manufacturerCode = defaultManufacturerCode; // Allocated by Australian Communications Authority, Oct 2000

  masterSlaveDeterminationRetries = 10;
  gatekeeperRequestRetries = 2;
  rasRequestRetries = 2;

  gatekeeper = NULL;

  PTRACE(3, "H323\tCreated endpoint.");
}


H323EndPoint::~H323EndPoint()
{
  // And shut down the gatekeeper (if there was one)
  RemoveGatekeeper();

  // Shut down the listeners as soon as possible to avoid race conditions
  listeners.RemoveAll();

  PTRACE(3, "H323\tDeleted endpoint.");
}


void H323EndPoint::SetEndpointTypeInfo(H225_EndpointType & info) const
{
  info.IncludeOptionalField(H225_EndpointType::e_vendor);
  SetVendorIdentifierInfo(info.m_vendor);

  switch (terminalType) {
    case e_TerminalOnly :
    case e_TerminalAndMC :
      info.IncludeOptionalField(H225_EndpointType::e_terminal);
      break;
    case e_GatewayOnly :
    case e_GatewayAndMC :
    case e_GatewayAndMCWithDataMP :
    case e_GatewayAndMCWithAudioMP :
    case e_GatewayAndMCWithAVMP :
      info.IncludeOptionalField(H225_EndpointType::e_gateway);
      break;
    case e_GatekeeperOnly :
    case e_GatekeeperWithDataMP :
    case e_GatekeeperWithAudioMP :
    case e_GatekeeperWithAVMP :
      info.IncludeOptionalField(H225_EndpointType::e_gatekeeper);
      break;
    case e_MCUOnly :
    case e_MCUWithDataMP :
    case e_MCUWithAudioMP :
    case e_MCUWithAVMP :
      info.IncludeOptionalField(H225_EndpointType::e_mcu);
      info.m_mc = TRUE;
  }
}


void H323EndPoint::SetVendorIdentifierInfo(H225_VendorIdentifier & info) const
{
  SetH221NonStandardInfo(info.m_vendor);

  info.IncludeOptionalField(H225_VendorIdentifier::e_productId);
  info.m_productId = PProcess::Current().GetManufacturer() & PProcess::Current().GetName();
  info.m_productId.SetSize(info.m_productId.GetSize()+2);

  info.IncludeOptionalField(H225_VendorIdentifier::e_versionId);
  info.m_versionId = PProcess::Current().GetVersion(TRUE) + " (OPAL v" + OpalGetVersion() + ')';
  info.m_versionId.SetSize(info.m_versionId.GetSize()+2);
}


void H323EndPoint::SetH221NonStandardInfo(H225_H221NonStandard & info) const
{
  info.m_t35CountryCode = t35CountryCode;
  info.m_t35Extension = t35Extension;
  info.m_manufacturerCode = manufacturerCode;
}


H323Capability * H323EndPoint::FindCapability(const H245_Capability & cap) const
{
  return GetCapabilities().FindCapability(cap);
}


H323Capability * H323EndPoint::FindCapability(const H245_DataType & dataType) const
{
  return GetCapabilities().FindCapability(dataType);
}


H323Capability * H323EndPoint::FindCapability(H323Capability::MainTypes mainType,
                                              unsigned subType) const
{
  return GetCapabilities().FindCapability(mainType, subType);
}


void H323EndPoint::AddCapability(H323Capability * capability)
{
  capabilities.Add(capability);
}


PINDEX H323EndPoint::SetCapability(PINDEX descriptorNum,
                                   PINDEX simultaneousNum,
                                   H323Capability * capability)
{
  return capabilities.SetCapability(descriptorNum, simultaneousNum, capability);
}


PINDEX H323EndPoint::AddAllCapabilities(PINDEX descriptorNum,
                                        PINDEX simultaneous,
                                        const PString & name)
{
  return capabilities.AddAllCapabilities(*this, descriptorNum, simultaneous, name);
}


void H323EndPoint::AddAllUserInputCapabilities(PINDEX descriptorNum,
                                               PINDEX simultaneous)
{
  H323_UserInputCapability::AddAllCapabilities(capabilities, descriptorNum, simultaneous);
}


void H323EndPoint::RemoveCapabilities(const PStringArray & codecNames)
{
  capabilities.Remove(codecNames);
}


void H323EndPoint::ReorderCapabilities(const PStringArray & preferenceOrder)
{
  capabilities.Reorder(preferenceOrder);
}


const H323Capabilities & H323EndPoint::GetCapabilities() const
{
  if (capabilities.GetSize() == 0) {
    capabilities.AddAllCapabilities(*this, P_MAX_INDEX, P_MAX_INDEX, "*");
    H323_UserInputCapability::AddAllCapabilities(capabilities, P_MAX_INDEX, P_MAX_INDEX);
  }

  return capabilities;
}


BOOL H323EndPoint::UseGatekeeper(const PString & address,
                                 const PString & identifier,
                                 const PString & localAddress)
{
  if (gatekeeper != NULL) {
    BOOL same = TRUE;

    if (!address)
      same = gatekeeper->GetTransport().GetRemoteAddress().IsEquivalent(address);

    if (!same && !identifier)
      same = gatekeeper->GetIdentifier() == identifier;

    if (!same && !localAddress)
      same = gatekeeper->GetTransport().GetLocalAddress().IsEquivalent(localAddress);

    if (same) {
      PTRACE(2, "H323\tUsing existing gatekeeper " << *gatekeeper);
      return TRUE;
    }
  }

  H323Transport * transport = NULL;
  if (!localAddress.IsEmpty()) {
    H323TransportAddress iface(localAddress);
    PIPSocket::Address ip;
    WORD port = H225_RAS::DefaultRasUdpPort;
    if (iface.GetIpAndPort(ip, port))
      transport = new H323TransportUDP(*this, ip, port);
  }

  if (address.IsEmpty()) {
    if (identifier.IsEmpty())
      return DiscoverGatekeeper(transport);
    else
      return LocateGatekeeper(identifier, transport);
  }
  else {
    if (identifier.IsEmpty())
      return SetGatekeeper(address, transport);
    else
      return SetGatekeeperZone(address, identifier, transport);
  }
}


BOOL H323EndPoint::SetGatekeeper(const PString & address, H323Transport * transport)
{
  H323Gatekeeper * gk = InternalCreateGatekeeper(transport);
  H323TransportAddress h323addr(address, H225_RAS::DefaultRasUdpPort, "udp");
  return InternalRegisterGatekeeper(gk, gk->DiscoverByAddress(h323addr));
}


BOOL H323EndPoint::SetGatekeeperZone(const PString & address,
                                     const PString & identifier,
                                     H323Transport * transport)
{
  H323Gatekeeper * gk = InternalCreateGatekeeper(transport);
  H323TransportAddress h323addr(address, H225_RAS::DefaultRasUdpPort, "udp");
  return InternalRegisterGatekeeper(gk, gk->DiscoverByNameAndAddress(identifier, h323addr));
}


BOOL H323EndPoint::LocateGatekeeper(const PString & identifier, H323Transport * transport)
{
  H323Gatekeeper * gk = InternalCreateGatekeeper(transport);
  return InternalRegisterGatekeeper(gk, gk->DiscoverByName(identifier));
}


BOOL H323EndPoint::DiscoverGatekeeper(H323Transport * transport)
{
  H323Gatekeeper * gk = InternalCreateGatekeeper(transport);
  return InternalRegisterGatekeeper(gk, gk->DiscoverAny());
}


H323Gatekeeper * H323EndPoint::InternalCreateGatekeeper(H323Transport * transport)
{
  RemoveGatekeeper(H225_UnregRequestReason::e_reregistrationRequired);

  if (transport == NULL)
    transport = new H323TransportUDP(*this);


  H323Gatekeeper * gk = CreateGatekeeper(transport);

  gk->SetPassword(gatekeeperPassword, gatekeeperUsername);

  return gk;
}


BOOL H323EndPoint::InternalRegisterGatekeeper(H323Gatekeeper * gk, BOOL discovered)
{
  if (discovered) {
    if (gk->RegistrationRequest()) {
      gatekeeper = gk;
      return TRUE;
    }

    // RRQ was rejected continue trying
    gatekeeper = gk;
  }
  else // Only stop listening if the GRQ was rejected
    delete gk;

  return FALSE;
}


H323Gatekeeper * H323EndPoint::CreateGatekeeper(H323Transport * transport)
{
  return new H323Gatekeeper(*this, transport);
}


BOOL H323EndPoint::IsRegisteredWithGatekeeper() const
{
  if (gatekeeper == NULL)
    return FALSE;

  return gatekeeper->IsRegistered();
}


BOOL H323EndPoint::RemoveGatekeeper(int reason)
{
  BOOL ok = TRUE;

  if (gatekeeper == NULL)
    return ok;

  ClearAllCalls();

  if (gatekeeper->IsRegistered()) // If we are registered send a URQ
    ok = gatekeeper->UnregistrationRequest(reason);

  delete gatekeeper;
  gatekeeper = NULL;

  return ok;
}


void H323EndPoint::SetGatekeeperPassword(const PString & password, const PString & username)
{
  gatekeeperUsername = username;
  gatekeeperPassword = password;

  if (gatekeeper != NULL) {
    gatekeeper->SetPassword(gatekeeperPassword, gatekeeperUsername);
    if (gatekeeper->IsRegistered()) // If we are registered send a URQ
      gatekeeper->UnregistrationRequest(H225_UnregRequestReason::e_reregistrationRequired);
    InternalRegisterGatekeeper(gatekeeper, TRUE);
  }
}

void H323EndPoint::OnGatekeeperConfirm()
{
}

void H323EndPoint::OnGatekeeperReject()
{
}

void H323EndPoint::OnRegistrationConfirm()
{
}

void H323EndPoint::OnRegistrationReject()
{
}

H235Authenticators H323EndPoint::CreateAuthenticators()
{
  H235Authenticators authenticators;

#if P_SSL
  authenticators.Append(new H235AuthProcedure1);
#endif
  authenticators.Append(new H235AuthSimpleMD5);
  authenticators.Append(new H235AuthCAT);

  return authenticators;
}


BOOL H323EndPoint::MakeConnection(OpalCall & call,
                                  const PString & remoteParty,
                                  void * userData)
{
  PTRACE(2, "H323\tMaking call to: " << remoteParty);
  return InternalMakeCall(call,
                          PString::Empty(),
                          PString::Empty(),
                          UINT_MAX,
                          remoteParty,
                          userData);
}


OpalMediaFormatList H323EndPoint::GetMediaFormats() const
{
  return OpalMediaFormat::GetAllRegisteredMediaFormats();
}


BOOL H323EndPoint::NewIncomingConnection(OpalTransport * transport)
{
  PTRACE(3, "H225\tAwaiting first PDU");
  transport->SetReadTimeout(15000); // Await 15 seconds after connect for first byte
  H323SignalPDU pdu;
  if (!pdu.Read(*transport)) {
    PTRACE(1, "H225\tFailed to get initial Q.931 PDU, connection not started.");
    return TRUE;
  }

  unsigned callReference = pdu.GetQ931().GetCallReference();
  PTRACE(3, "H225\tIncoming call, first PDU: callReference=" << callReference);

  // Get a new (or old) connection from the endpoint, calculate token
  PString token = transport->GetRemoteAddress();
  token.sprintf("/%u", callReference);

  PSafePtr<H323Connection> connection = FindConnectionWithLock(token);

  if (connection == NULL) {
    connection = CreateConnection(*manager.CreateCall(), token, NULL,
                                  *transport, PString::Empty(), PString::Empty(), &pdu);
    if (connection == NULL) {
      PTRACE(1, "H225\tEndpoint could not create connection, "
                "sending release complete PDU: callRef=" << callReference);
      Q931 reply;
      reply.BuildReleaseComplete(callReference, TRUE);
      PBYTEArray rawData;
      reply.Encode(rawData);
      transport->WritePDU(rawData);
      return TRUE;
    }

    connectionsActive.SetAt(token, connection);
  }

  PTRACE(3, "H323\tCreated new connection: " << token);
  connection->AttachSignalChannel(token, transport, TRUE);

  if (connection->HandleSignalPDU(pdu)) {
    // All subsequent PDU's should wait forever
    transport->SetReadTimeout(PMaxTimeInterval);
    connection->HandleSignallingChannel();
  }
  else {
    connection->ClearCall(H323Connection::EndedByTransportFail);
    PTRACE(1, "H225\tSignal channel stopped on first PDU.");
  }

  return FALSE;
}


H323Connection * H323EndPoint::CreateConnection(OpalCall & call,
                                                const PString & token,
                                                void * /*userData*/,
                                                OpalTransport & /*transport*/,
                                                const PString & alias,
                                                const H323TransportAddress & address,
                                                H323SignalPDU * /*setupPDU*/)
{
  return new H323Connection(call, *this, token, alias, address);
}


BOOL H323EndPoint::SetupTransfer(const PString & oldToken,
                                 const PString & callIdentity,
                                 const PString & remoteParty,
                                 void * userData)
{
  PTRACE(2, "H323\tTransferring call to: " << remoteParty);
  return InternalMakeCall(*manager.CreateCall(),
                          oldToken,
                          callIdentity,
                          UINT_MAX,
                          remoteParty,
                          userData);
}


BOOL H323EndPoint::InternalMakeCall(OpalCall & call,
                                    const PString & existingToken,
                                    const PString & callIdentity,
                                    unsigned capabilityLevel,
                                    const PString & remoteParty,
                                    void * userData)
{
  PString alias;
  H323TransportAddress address;
  if (!ParsePartyName(remoteParty, alias, address)) {
    PTRACE(2, "H323\tCould not parse \"" << remoteParty << '"');
    return FALSE;
  }

  // Restriction: the call must be made on the same transport as the one
  // that the gatekeeper is using.
  H323Transport * transport;
  if (gatekeeper != NULL)
    transport = gatekeeper->GetTransport().GetLocalAddress().CreateTransport(
                                          *this, OpalTransportAddress::Streamed);
  else
    transport = address.CreateTransport(*this, OpalTransportAddress::NoBinding);

  if (transport == NULL) {
    PTRACE(1, "H323\tInvalid transport in \"" << remoteParty << '"');
    return FALSE;
  }

  inUseFlag.Wait();

  PString newToken;
  if (existingToken.IsEmpty()) {
    do {
      newToken = psprintf("localhost/%u", Q931::GenerateCallReference());
    } while (connectionsActive.Contains(newToken));
  }
  else {
    PSafePtr<OpalConnection> otherConnection = GetConnectionWithLock(existingToken, PSafeReference);
    if (otherConnection == NULL) {
      PTRACE(1, "H225\tTransfer connection disappeared, aborting setup.");
      return FALSE;
    }
    // Move old connection on token to new value and flag for removal
    PString adjustedToken;
    unsigned tieBreaker = 0;
    do {
      adjustedToken = newToken + "-replaced";
      adjustedToken.sprintf("-%u", ++tieBreaker);
    } while (connectionsActive.Contains(adjustedToken));
    connectionsActive.RemoveAt(newToken);
    connectionsActive.SetAt(adjustedToken, otherConnection);
//    call.Release(connection);
    PTRACE(3, "H323\tOverwriting call " << newToken << ", renamed to " << adjustedToken);
  }

  H323Connection * connection = CreateConnection(call, newToken, userData, *transport, alias, address, NULL);
  if (connection == NULL) {
    PTRACE(1, "H225\tEndpoint could not create connection, aborting setup.");
    return FALSE;
  }

  connectionsActive.SetAt(newToken, connection);

  inUseFlag.Signal();

  connection->AttachSignalChannel(newToken, transport, FALSE);

  if (!callIdentity) {
    if (capabilityLevel == UINT_MAX)
      connection->HandleTransferCall(existingToken, callIdentity);
    else {
      connection->HandleIntrudeCall(existingToken, callIdentity);
      connection->IntrudeCall(capabilityLevel);
    }
  }

  PTRACE(3, "H323\tCreated new connection: " << newToken);

  // See if we are starting an outgoing connection as first in a call
  if (call.GetConnection(0) == connection)
    connection->SetUpConnection();

  return TRUE;
}


void H323EndPoint::TransferCall(const PString & token, 
                                const PString & remoteParty,
                                const PString & callIdentity)
{
  PSafePtr<H323Connection> connection = FindConnectionWithLock(token);
  if (connection != NULL)
    connection->TransferCall(remoteParty, callIdentity);
}


void H323EndPoint::ConsultationTransfer(const PString & primaryCallToken,   
                                        const PString & secondaryCallToken)
{
  PSafePtr<H323Connection> secondaryCall = FindConnectionWithLock(secondaryCallToken);
  if (secondaryCall != NULL)
    secondaryCall->ConsultationTransfer(primaryCallToken);
}


void H323EndPoint::HoldCall(const PString & token, BOOL localHold)
{
  PSafePtr<H323Connection> connection = FindConnectionWithLock(token);
  if (connection != NULL)
    connection->HoldCall(localHold);
}


BOOL H323EndPoint::IntrudeCall(const PString & remoteParty,
                               unsigned capabilityLevel,
                               void * userData)
{
  return InternalMakeCall(*manager.CreateCall(),
                          PString::Empty(),
                          PString::Empty(),
                          capabilityLevel,
                          remoteParty,
                          userData);
}


BOOL H323EndPoint::ParsePartyName(const PString & remoteParty,
                                  PString & alias,
                                  H323TransportAddress & address)
{
  PURL url(remoteParty, "h323");

  // Special adjustment if 
  if (remoteParty.Find('@') == P_MAX_INDEX &&
      remoteParty.NumCompare(url.GetScheme()) != EqualTo) {
    if (gatekeeper == NULL)
      url.Parse("h323:@" + remoteParty);
    else
      url.Parse("h323:" + remoteParty);
  }

  alias = url.GetUserName();

  address = url.GetHostName();
  if (!address && url.GetPort() != 0)
    address.sprintf(":%u", url.GetPort());

  if (alias.IsEmpty() && address.IsEmpty()) {
    PTRACE(1, "H323\tAttempt to use invalid URL \"" << remoteParty << '"');
    return FALSE;
  }

  BOOL gatekeeperSpecified = FALSE;
  BOOL gatewaySpecified = FALSE;

  PCaselessString type = url.GetParamVars()("type");

  if (url.GetScheme() == "callto") {
    // Do not yet support ILS
    if (type == "directory") {
#if P_LDAP
      PString server = url.GetHostName();
      if (server.IsEmpty())
        server = GetDefaultILSServer();
      if (server.IsEmpty())
        return FALSE;

      PILSSession ils;
      if (!ils.Open(server, url.GetPort())) {
        PTRACE(1, "H323\tCould not open ILS server at \"" << server
               << "\" - " << ils.GetErrorText());
        return FALSE;
      }

      PILSSession::RTPerson person;
      if (!ils.SearchPerson(alias, person)) {
        PTRACE(1, "H323\tCould not find "
               << server << '/' << alias << ": " << ils.GetErrorText());
        return FALSE;
      }

      if (!person.sipAddress.IsValid()) {
        PTRACE(1, "H323\tILS user " << server << '/' << alias
               << " does not have a valid IP address");
        return FALSE;
      }

      // Get the IP address
      address = H323TransportAddress(person.sipAddress);

      // Get the port if provided
      for (PINDEX i = 0; i < person.sport.GetSize(); i++) {
        if (person.sport[i] != 1503) { // Dont use the T.120 port
          address = H323TransportAddress(person.sipAddress, person.sport[i]);
          break;
        }
      }

      alias = PString::Empty(); // No alias for ILS lookup, only host
      return TRUE;
#else
      return FALSE;
#endif
    }

    if (url.GetParamVars().Contains("gateway"))
      gatewaySpecified = TRUE;
  }
  else if (url.GetScheme() == "h323") {
    if (type == "gw")
      gatewaySpecified = TRUE;
    else if (type == "gk")
      gatekeeperSpecified = TRUE;
    else if (!type) {
      PTRACE(1, "H323\tUnsupported host type \"" << type << "\" in h323 URL");
      return FALSE;
    }
  }

  // User explicitly asked fo a look up to a gk
  if (gatekeeperSpecified) {
    if (alias.IsEmpty()) {
      PTRACE(1, "H323\tAttempt to use explict gatekeeper without alias!");
      return FALSE;
    }

    if (address.IsEmpty()) {
      PTRACE(1, "H323\tAttempt to use explict gatekeeper without address!");
      return FALSE;
    }

    H323TransportAddress gkAddr = address;
    PTRACE(3, "H323\tLooking for \"" << alias << "\" on gatekeeper at " << gkAddr);

    H323Gatekeeper * gk = CreateGatekeeper(new H323TransportUDP(*this));

    BOOL ok = gk->DiscoverByAddress(gkAddr);
    if (ok) {
      ok = gk->LocationRequest(alias, address);
      if (ok) {
        PTRACE(3, "H323\tLocation Request of \"" << alias << "\" on gk " << gkAddr << " found " << address);
      }
      else {
        PTRACE(1, "H323\tLocation Request failed for \"" << alias << "\" on gk " << gkAddr);
      }
    }
    else {
      PTRACE(1, "H323\tLocation Request discovery failed for gk " << gkAddr);
    }

    delete gk;

    return ok;
  }

  // User explicitly said to use a gw, or we do not have a gk to do look up
  if (gatekeeper == NULL || gatewaySpecified) {
    // If URL did not have a host, but user said to use gw, or we do not have
    // a gk to do a lookup so we MUST have a host, use alias must be host
    if (address.IsEmpty()) {
      address = alias;
      alias = PString::Empty();
    }
    return TRUE;
  }

  // We have a gatekeeper and user provided a host, all done
  if (!address)
    return TRUE;

  // We have a gk and user did not explicitly supply a host, so lets
  // do a check to see if it is an IP address or hostname
  if (alias.FindOneOf("$.:[") != P_MAX_INDEX) {
    H323TransportAddress test = alias;
    PIPSocket::Address ip;
    if (test.GetIpAddress(ip) && ip.IsValid()) {
      // The alias was a valid internet address, use it as such
      alias = PString::Empty();
      address = test;
    }
  }

  return TRUE;
}


PSafePtr<H323Connection> H323EndPoint::FindConnectionWithLock(const PString & token, PSafetyMode mode)
{
  PSafePtr<H323Connection> connnection = PSafePtrCast<OpalConnection, H323Connection>(GetConnectionWithLock(token, mode));
  if (connnection != NULL)
    return connnection;

  for (connnection = PSafePtrCast<OpalConnection, H323Connection>(connectionsActive.GetAt(0)); connnection != NULL; ++connnection) {
    if (connnection->GetCallIdentifier().AsString() == token)
      return connnection;
    if (connnection->GetConferenceIdentifier().AsString() == token)
      return connnection;
  }

  return NULL;
}


BOOL H323EndPoint::OnIncomingCall(H323Connection & connection,
                                  const H323SignalPDU & /*setupPDU*/,
                                  H323SignalPDU & /*alertingPDU*/)
{
  return connection.OnIncomingConnection();
}


BOOL H323EndPoint::OnCallTransferInitiate(H323Connection & /*connection*/,
                                          const PString & /*remoteParty*/)
{
  return TRUE;
}


BOOL H323EndPoint::OnCallTransferIdentify(H323Connection & /*connection*/)
{
  return TRUE;
}


OpalConnection::AnswerCallResponse
       H323EndPoint::OnAnswerCall(H323Connection & connection,
                                  const PString & caller,
                                  const H323SignalPDU & /*setupPDU*/,
                                  H323SignalPDU & /*connectPDU*/)
{
  return connection.OnAnswerCall(caller);
}

OpalConnection::AnswerCallResponse
       H323EndPoint::OnAnswerCall(OpalConnection & connection,
       const PString & caller
)
{
  return OpalEndPoint::OnAnswerCall(connection, caller);
}

BOOL H323EndPoint::OnAlerting(H323Connection & connection,
                              const H323SignalPDU & /*alertingPDU*/,
                              const PString & /*username*/)
{
  PTRACE(1, "H225\tReceived alerting PDU.");
  ((OpalConnection&)connection).OnAlerting();
  return TRUE;
}


BOOL H323EndPoint::OnConnectionForwarded(H323Connection & /*connection*/,
                                         const PString & /*forwardParty*/,
                                         const H323SignalPDU & /*pdu*/)
{
  return FALSE;
}


BOOL H323EndPoint::ForwardConnection(H323Connection & connection,
                                     const PString & forwardParty,
                                     const H323SignalPDU & /*pdu*/)
{
  if (!InternalMakeCall(connection.GetCall(),
                        connection.GetCallToken(),
                        PString::Empty(),
                        UINT_MAX,
                        forwardParty,
                        NULL))
    return FALSE;

  connection.SetCallEndReason(H323Connection::EndedByCallForwarded);

  return TRUE;
}


void H323EndPoint::OnConnectionEstablished(H323Connection & /*connection*/,
                                           const PString & /*token*/)
{
}


BOOL H323EndPoint::IsConnectionEstablished(const PString & token)
{
  PSafePtr<H323Connection> connection = FindConnectionWithLock(token);
  return connection != NULL && connection->IsEstablished();
}


BOOL H323EndPoint::OnOutgoingCall(H323Connection & /*connection*/,
                                  const H323SignalPDU & /*connectPDU*/)
{
  PTRACE(1, "H225\tReceived connect PDU.");
  return TRUE;
}


void H323EndPoint::OnConnectionCleared(H323Connection & /*connection*/,
                                       const PString & /*token*/)
{
}


#if PTRACING
static void OnStartStopChannel(const char * startstop, const H323Channel & channel)
{
  const char * dir;
  switch (channel.GetDirection()) {
    case H323Channel::IsTransmitter :
      dir = "send";
      break;

    case H323Channel::IsReceiver :
      dir = "receiv";
      break;

    default :
      dir = "us";
      break;
  }

  PTRACE(2, "H323\t" << startstop << "ed "
                     << dir << "ing logical channel: "
                     << channel.GetCapability());
}
#endif


BOOL H323EndPoint::OnStartLogicalChannel(H323Connection & /*connection*/,
                                         H323Channel & PTRACE_channel)
{
#if PTRACING
  OnStartStopChannel("Start", PTRACE_channel);
#endif
  return TRUE;
}


void H323EndPoint::OnClosedLogicalChannel(H323Connection & /*connection*/,
                                          const H323Channel & PTRACE_channel)
{
#if PTRACING
  OnStartStopChannel("Stopp", PTRACE_channel);
#endif
}


void H323EndPoint::OnRTPStatistics(const H323Connection & /*connection*/,
                                   const RTP_Session & /*session*/) const
{
}


void H323EndPoint::OnHTTPServiceControl(unsigned /*opeartion*/,
                                        unsigned /*sessionId*/,
                                        const PString & /*url*/)
{
}


void H323EndPoint::OnCallCreditServiceControl(const PString & /*amount*/, BOOL /*mode*/)
{
}


void H323EndPoint::OnServiceControlSession(unsigned type,
                                           unsigned sessionId,
                                           const H323ServiceControlSession & session,
                                           H323Connection * connection)
{
  session.OnChange(type, sessionId, *this, connection);
}

BOOL H323EndPoint::OnConferenceInvite(const H323SignalPDU & /*setupPDU */)
{
  return FALSE;
}

BOOL H323EndPoint::OnCallIndependentSupplementaryService(const H323SignalPDU & /* setupPDU */)
{
  return FALSE;
}

BOOL H323EndPoint::OnNegotiateConferenceCapabilities(const H323SignalPDU & /* setupPDU */)
{
  return FALSE;
}

H323ServiceControlSession * H323EndPoint::CreateServiceControlSession(const H225_ServiceControlDescriptor & contents)
{
  switch (contents.GetTag()) {
    case H225_ServiceControlDescriptor::e_url :
      return new H323HTTPServiceControl(contents);

    case H225_ServiceControlDescriptor::e_callCreditServiceControl :
      return new H323CallCreditServiceControl(contents);
  }

  return NULL;
}


void H323EndPoint::SetLocalUserName(const PString & name)
{
  PAssert(!name, "Must have non-empty string in AliasAddress!");
  if (name.IsEmpty())
    return;

  localAliasNames.RemoveAll();
  localAliasNames.AppendString(name);
}


BOOL H323EndPoint::AddAliasName(const PString & name)
{
  PAssert(!name, "Must have non-empty string in AliasAddress!");

  if (localAliasNames.GetValuesIndex(name) != P_MAX_INDEX)
    return FALSE;

  localAliasNames.AppendString(name);
  return TRUE;
}


BOOL H323EndPoint::RemoveAliasName(const PString & name)
{
  PINDEX pos = localAliasNames.GetValuesIndex(name);
  if (pos == P_MAX_INDEX)
    return FALSE;

  PAssert(localAliasNames.GetSize() > 1, "Must have at least one AliasAddress!");
  if (localAliasNames.GetSize() < 2)
    return FALSE;

  localAliasNames.RemoveAt(pos);
  return TRUE;
}


BOOL H323EndPoint::IsTerminal() const
{
  switch (terminalType) {
    case e_TerminalOnly :
    case e_TerminalAndMC :
      return TRUE;

    default :
      return FALSE;
  }
}


BOOL H323EndPoint::IsGateway() const
{
  switch (terminalType) {
    case e_GatewayOnly :
    case e_GatewayAndMC :
    case e_GatewayAndMCWithDataMP :
    case e_GatewayAndMCWithAudioMP :
    case e_GatewayAndMCWithAVMP :
      return TRUE;

    default :
      return FALSE;
  }
}


BOOL H323EndPoint::IsGatekeeper() const
{
  switch (terminalType) {
    case e_GatekeeperOnly :
    case e_GatekeeperWithDataMP :
    case e_GatekeeperWithAudioMP :
    case e_GatekeeperWithAVMP :
      return TRUE;

    default :
      return FALSE;
  }
}


BOOL H323EndPoint::IsMCU() const
{
  switch (terminalType) {
    case e_MCUOnly :
    case e_MCUWithDataMP :
    case e_MCUWithAudioMP :
    case e_MCUWithAVMP :
      return TRUE;

    default :
      return FALSE;
  }
}


void H323EndPoint::TranslateTCPAddress(PIPSocket::Address & localAddr,
                                       const PIPSocket::Address & remoteAddr)
{
  manager.TranslateIPAddress(localAddr, remoteAddr);
}


/////////////////////////////////////////////////////////////////////////////
