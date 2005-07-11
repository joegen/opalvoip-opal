/*
 * h323.cxx
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
 * Contributor(s): ______________________________________.
 *
 * $Log: h323.cxx,v $
 * Revision 1.2081  2005/07/11 06:52:16  csoutheren
 * Added support for outgoing calls using external RTP
 *
 * Revision 2.79  2005/07/11 01:52:24  csoutheren
 * Extended AnsweringCall to work for SIP as well as H.323
 * Fixed problems with external RTP connection in H.323
 * Added call to OnClosedMediaStream
 *
 * Revision 2.78  2005/04/10 21:09:11  dsandras
 * Added support for connection hold (new API).
 *
 * Revision 2.77  2005/04/10 21:08:14  dsandras
 * Added support for Blind Transfer (new API).
 *
 * Revision 2.76  2005/04/10 21:07:24  dsandras
 * Added support for function that returns the "best guess" h323 callback URL for a connection.
 *
 * Revision 2.75  2005/03/01 19:22:18  dsandras
 * Call AdjustMediaFormats or the SinkMediaStream can be opened with a remote codec that is disabled locally.
 *
 * Revision 2.74  2005/03/01 18:43:15  dsandras
 * Removed call to OnEstablished() as it is already called via H323UnidirectionalChannel::Open().
 *
 * Revision 2.73  2005/02/24 19:58:26  dsandras
 * Fixed problem with transmit channel not started on connect thanks to DDV.
 *
 * Revision 2.72  2005/02/19 22:42:55  dsandras
 * Removed unuseful assertion.
 *
 * Revision 2.71  2005/01/15 23:32:41  csoutheren
 * Added debugging to help find slow-start problem
 *
 * Revision 2.70  2005/01/15 23:27:31  dsandras
 * Only start channels if H.245 Master.
 *
 * Revision 2.69  2005/01/15 22:35:14  dsandras
 * Lock things before accessing/modifying them.
 *
 * Revision 2.68  2005/01/15 21:33:56  dsandras
 * Only start the H.245 channel if it doesn't exist yet.
 *
 * Revision 2.67  2004/11/07 12:29:05  rjongbloed
 * Fixed missing initialisation of call reference value.
 * Added change so that a DNS lookup is not made if an e164 calling party number
 *   is present in SETUP, for Dmitriy
 *
 * Revision 2.66  2004/10/04 12:57:06  rjongbloed
 * Fixed temporary unlocking of read/write mutex so if cannot relock does not
 *   get an unbalanced unlock error.
 *
 * Revision 2.65  2004/10/03 14:18:47  rjongbloed
 * Fixed missing check for established after connecting.
 *
 * Revision 2.64  2004/10/02 12:30:22  rjongbloed
 * Fixed keeping H.323 and OPAL state variables in sync around "established" connections.
 *
 * Revision 2.63  2004/09/01 12:21:28  rjongbloed
 * Added initialisation of H323EndPoints capability table to be all codecs so can
 *   correctly build remote caps from fqast connect params. This had knock on effect
 *   with const keywords added in numerous places.
 *
 * Revision 2.61  2004/08/14 07:56:31  rjongbloed
 * Major revision to utilise the PSafeCollection classes for the connections and calls.
 *
 * Revision 2.60  2004/07/17 09:48:26  rjongbloed
 * Allowed for case where OnSetUp() goes all the way to connected/established in
 *   one go, eg on an auto answer IVR system.
 *
 * Revision 2.59  2004/07/11 12:46:45  rjongbloed
 * Added function on endpoints to get the list of all media formats any
 *   connection the endpoint may create can support.
 * Fixed multple calls of OnSetLocalCapabilities changing caps after having sent to remote.
 * Made sure local caps have the UII caps set in it.
 *
 * Revision 2.58  2004/07/04 12:52:43  rjongbloed
 * Fixed setting of local capabilities to after routing, so we know the peer protocols media list.
 *
 * Revision 2.57  2004/06/04 06:54:18  csoutheren
 * Migrated updates from OpenH323 1.14.1
 *
 * Revision 2.56  2004/05/09 13:12:38  rjongbloed
 * Fixed issues with non fast start and non-tunnelled connections
 *
 * Revision 2.55  2004/05/01 10:00:51  rjongbloed
 * Fixed ClearCallSynchronous so now is actually signalled when call is destroyed.
 *
 * Revision 2.54  2004/04/29 11:47:54  rjongbloed
 * Fixed problem with trying to open channels already open for session.
 *
 * Revision 2.53  2004/04/26 05:39:27  rjongbloed
 * Fixed missing propagation of OnEstablished() call to endpoint.
 *
 * Revision 2.52  2004/04/26 04:33:06  rjongbloed
 * Move various call progress times from H.323 specific to general conenction.
 *
 * Revision 2.51  2004/04/25 02:53:29  rjongbloed
 * Fixed GNU 3.4 warnings
 *
 * Revision 2.50  2004/04/18 13:38:01  rjongbloed
 * Fixed using correct H.323 port if not specified in address
 *
 * Revision 2.49  2004/04/07 08:21:03  rjongbloed
 * Changes for new RTTI system.
 *
 * Revision 2.48  2004/03/25 11:48:37  rjongbloed
 * Changed PCM-16 from IllegalPayloadType to MaxPayloadType to avoid problems
 *   in other parts of the code.
 *
 * Revision 2.47  2004/03/22 11:32:41  rjongbloed
 * Added new codec type for 16 bit Linear PCM as must distinguish between the internal
 *   format used by such things as the sound card and the RTP payload format which
 *   is always big endian.
 *
 * Revision 2.46  2004/03/13 06:25:52  rjongbloed
 * Slight rearrangement of local party name and alias list to beter match common
 *   behaviour in ancestor.
 * Abstracted local party name for endpoint into ancestor from H.,323.
 *
 * Revision 2.45  2004/03/02 09:57:53  rjongbloed
 * Fixed problems with recent changes to RTP UseSession() function which broke it for H.323
 *
 * Revision 2.44  2004/02/24 11:28:46  rjongbloed
 * Normalised RTP session management across protocols
 *
 * Revision 2.43  2004/02/19 10:47:04  rjongbloed
 * Merged OpenH323 version 1.13.1 changes.
 *
 * Revision 2.42  2004/02/07 00:35:47  rjongbloed
 * Fixed calls GetMediaFormats so no DOES return intersection of all connections formats.
 * Tidied some API elements to make usage more explicit.
 *
 * Revision 2.41  2004/02/03 12:20:44  rjongbloed
 * Fixed setting of Q.931 call reference value in connection object, and thus all outgoing PDU's.
 *
 * Revision 2.40  2003/06/02 03:01:43  rjongbloed
 * Fixed incorrect condition when starting media and already have it started.
 *
 * Revision 2.39  2003/04/08 02:51:23  robertj
 * Fixed incorrect test for already started media, thanks Guilhem Tardy.
 *
 * Revision 2.38  2003/03/17 10:26:59  robertj
 * Added video support.
 *
 * Revision 2.37  2003/03/07 05:51:10  robertj
 * Fixed correct timing of call to OnSetUp() must be after connect PDU
 *   is available.
 * Fixed missing Unlock() in H.450 processing.
 *
 * Revision 2.36  2003/03/06 03:57:47  robertj
 * IVR support (work in progress) requiring large changes everywhere.
 *
 * Revision 2.35  2003/01/07 04:39:53  robertj
 * Updated to OpenH323 v1.11.2
 *
 * Revision 2.34  2002/11/10 22:59:49  robertj
 * Fixed override of SetCallEndReason to have same parameters as base virtual.
 *
 * Revision 2.33  2002/11/10 11:33:18  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.32  2002/09/13 02:27:10  robertj
 * Fixed GNU warning.
 * Fixed accidentally lost function call to set up H.245 in call proceeding.
 *
 * Revision 2.31  2002/09/04 06:01:48  robertj
 * Updated to OpenH323 v1.9.6
 *
 * Revision 2.30  2002/07/01 04:56:32  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 2.29  2002/06/16 02:23:43  robertj
 * Fixed memory leak of media streams in non H323 protocols, thanks Ted Szoczei
 *
 * Revision 2.28  2002/04/15 08:51:23  robertj
 * Fixed correct setting of jitter buffer size in RTP media stream.
 *
 * Revision 2.27  2002/04/10 03:11:29  robertj
 * Moved code for handling media bypass address resolution into ancestor as
 *   now done ths same way in both SIP and H.323.
 *
 * Revision 2.26  2002/04/09 00:17:59  robertj
 * Changed "callAnswered" to better description of "originating".
 *
 * Revision 2.25  2002/03/27 02:21:51  robertj
 * Updated to OpenH323 v1.8.4
 *
 * Revision 2.24  2002/03/26 23:40:05  robertj
 * Added missing call to OnCleared() for backward compatibility.
 *
 * Revision 2.23  2002/03/22 06:57:49  robertj
 * Updated to OpenH323 version 1.8.2
 *
 * Revision 2.22  2002/03/18 00:33:36  robertj
 * Removed duplicate initialBandwidth variable in H.323 class, moved to ancestor.
 *
 * Revision 2.21  2002/03/15 00:24:33  robertj
 * Assure capabilities are set to other connections media list (if not already)
 *    before doing fast start accept in CONNECT response.
 *
 * Revision 2.20  2002/02/19 07:46:36  robertj
 * Restructured media bypass functions to fix problems with RFC2833.
 * Fixes to capability tables to make RFC2833 work properly.
 *
 * Revision 2.19  2002/02/11 09:32:12  robertj
 * Updated to openH323 v1.8.0
 *
 * Revision 2.18  2002/02/11 07:40:46  robertj
 * Added media bypass for streams between compatible protocols.
 *
 * Revision 2.17  2002/01/22 05:25:53  robertj
 * Revamp of user input API triggered by RFC2833 support
 * Update from OpenH323
 *
 * Revision 2.16  2002/01/14 06:35:57  robertj
 * Updated to OpenH323 v1.7.9
 *
 * Revision 2.15  2002/01/14 02:26:23  robertj
 * Changes to support OPAL protocol model (eg SIP gateway).
 *
 * Revision 2.14  2001/11/15 07:01:36  robertj
 * Changed OpalCall::OpenSourceMediaStreams so the connection to not open
 *   a media stream on is optional.
 *
 * Revision 2.13  2001/11/14 01:31:55  robertj
 * Corrected placement of adjusting media format list.
 *
 * Revision 2.12  2001/11/13 04:29:47  robertj
 * Changed OpalTransportAddress CreateTransport and CreateListsner functions
 *   to have extra parameter to control local binding of sockets.
 *
 * Revision 2.11  2001/11/06 05:40:13  robertj
 * Added OpalListenerUDP class to do listener semantics on a UDP socket.
 *
 * Revision 2.10  2001/11/02 10:45:19  robertj
 * Updated to OpenH323 v1.7.3
 *
 * Revision 2.9  2001/10/15 04:36:37  robertj
 * Added delayed start of media patch threads.
 * Removed answerCall signal and replaced with state based functions.
 * Maintained H.323 answerCall API for backward compatibility.
 *
 * Revision 2.8  2001/10/05 00:22:14  robertj
 * Updated to PWLib 1.2.0 and OpenH323 1.7.0
 *
 * Revision 2.7  2001/10/04 00:46:33  robertj
 * Removed streams GetMediaFormats() function as is not useful.
 *
 * Revision 2.6  2001/10/03 05:56:15  robertj
 * Changes abndwidth management API.
 *
 * Revision 2.5  2001/08/22 10:20:09  robertj
 * Changed connection locking to use double mutex to guarantee that
 *   no threads can ever deadlock or access deleted connection.
 *
 * Revision 2.4  2001/08/21 11:05:06  robertj
 * Fixed GNU warning.
 *
 * Revision 2.3  2001/08/17 08:27:44  robertj
 * Update from OpenH323
 * Moved call end reasons enum from OpalConnection to global.
 *
 * Revision 2.2  2001/08/13 05:10:39  robertj
 * Updates from OpenH323 v1.6.0 release.
 *
 * Revision 2.1  2001/08/01 05:15:22  robertj
 * Moved default session ID's to OpalMediaFormat class.
 * Added setting of connections capabilities from other connections in
 *   calls media format list.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.322  2003/12/31 02:29:20  csoutheren
 * Ensure that an ARJ with code securityDenial returns call end reason code EndedBySecurityDenial (for outgoing calls too)
 *
 * Revision 1.321  2003/12/31 02:24:12  csoutheren
 * Ensure that an ARJ with code securityDenial returns call end reason code EndedBySecurityDenial
 *
 * Revision 1.320  2003/12/28 02:37:49  csoutheren
 * Added H323EndPoint::OnOutgoingCall
 *
 * Revision 1.319  2003/12/17 10:25:41  csoutheren
 * Removed misleading error message thanks to Hans Verbeek
 *
 * Revision 1.318  2003/10/27 06:03:39  csoutheren
 * Added support for QoS
 *   Thanks to Henry Harrison of AliceStreet
 *
 * Revision 1.317  2003/10/09 09:47:45  csoutheren
 * Fixed problem with re-opening RTP half-channels under unusual
 * circumstances. Thanks to Damien Sandras
 *
 * Revision 1.316  2003/07/18 10:03:54  csoutheren
 * Fixed mistake in applying previous change where usage of SwapChannel
 * accidentally used ReplaceChannel
 *
 * Revision 1.315  2003/07/16 10:43:13  csoutheren
 * Added SwapChannel function to H323Codec to allow media hold channels
 * to work better. Thanks to Federico Pinna
 *
 * Revision 1.314  2003/05/06 06:23:37  robertj
 * Added continuous DTMF tone support (H.245 UserInputIndication - signalUpdate)
 *   as per header documentation, thanks Auri Vizgaitis
 *
 * Revision 1.313  2003/04/30 00:28:55  robertj
 * Redesigned the alternate credentials in ARQ system as old implementation
 *   was fraught with concurrency issues, most importantly it can cause false
 *   detection of replay attacks taking out an endpoint completely.
 *
 * Revision 1.312  2003/03/11 22:14:18  dereks
 * Fix syntax error in previous commit.
 *
 * Revision 1.311  2003/03/11 22:12:39  dereks
 * Fix so that pause stops video also; Thanks Damien Sandras.
 *
 * Revision 1.310  2003/02/12 23:59:25  robertj
 * Fixed adding missing endpoint identifer in SETUP packet when gatekeeper
 * routed, pointed out by Stefan Klein
 * Also fixed correct rutrn of gk routing in IRR packet.
 *
 * Revision 1.309  2003/02/12 02:21:50  robertj
 * Changed Q.931 release complete cause code to be error value, an impossible
 *   value rather than just strange, until a possible value is received.
 *
 * Revision 1.308  2003/02/09 23:28:33  robertj
 * Fixed problem with left over extension fields from previously received
 *   H.225 PDUs, thanks Gustavo García
 *
 * Revision 1.307  2003/01/29 23:48:34  robertj
 * Removed extraneous code, thanks Chih-Wei Huang
 *
 * Revision 1.306  2003/01/26 02:50:23  craigs
 * Change so SETUP PDU uses conference and callIdentifier from H323Connection,
 * rather than both doing seperately and then overwriting
 *
 * Revision 1.305  2002/12/18 06:56:09  robertj
 * Moved the RAS IRR on UUIE to after UUIE is processed. Means you get
 *   correct state variables such as connectedTime.
 *
 * Revision 1.304  2002/12/12 01:24:38  robertj
 * Changed default user indication input mode to be alphanumeric H.245 if the
 *   remote TCS does not indicate any UII capability (per spec).
 *
 * Revision 1.303  2002/12/10 23:38:33  robertj
 * Moved the unsolicited IRR for sending requested UUIE to inside mutex.
 *
 * Revision 1.302  2002/12/06 03:44:41  robertj
 * Fixed bug in seleting request mode other than zero, thanks Ulrich Findeisen
 *
 * Revision 1.301  2002/11/27 06:54:56  robertj
 * Added Service Control Session management as per Annex K/H.323 via RAS
 *   only at this stage.
 * Added H.248 ASN and very primitive infrastructure for linking into the
 *   Service Control Session management system.
 * Added basic infrastructure for Annex K/H.323 HTTP transport system.
 * Added Call Credit Service Control to display account balances.
 *
 * Revision 1.300  2002/11/15 05:17:26  robertj
 * Added facility redirect support without changing the call token for access
 *   to the call. If it gets redirected a new H323Connection object is
 *   created but it looks like the same thing to an application.
 *
 * Revision 1.299  2002/11/13 04:37:55  robertj
 * Added ability to get (and set) Q.931 release complete cause codes.
 *
 * Revision 1.298  2002/10/31 00:39:21  robertj
 * Enhanced jitter buffer system so operates dynamically between minimum and
 *   maximum values. Altered API to assure app writers note the change!
 *
 * Revision 1.297  2002/10/30 05:53:32  craigs
 * Changed to allow h245InSetup acknowledge to be in packet after CallProceeding
 *
 * Revision 1.296  2002/10/29 00:10:11  robertj
 * Added IsValid() function to indicate that a PTime is set correctly.
 *
 * Revision 1.295  2002/10/16 02:30:51  robertj
 * Fixed incorrect ipv6 symbol for H.245 pdu, thanks Sébastien Josset.
 *
 * Revision 1.294  2002/10/08 13:08:21  robertj
 * Changed for IPv6 support, thanks Sébastien Josset.
 *
 * Revision 1.293  2002/09/03 06:07:54  robertj
 * Removed redundent dump of H.245 pdu.
 *
 * Revision 1.292  2002/08/09 07:22:39  robertj
 * Fixed error trace log message for if had transport.error to gatekeeper.
 *
 * Revision 1.291  2002/08/05 10:03:47  robertj
 * Cosmetic changes to normalise the usage of pragma interface/implementation.
 *
 * Revision 1.290  2002/08/05 05:17:41  robertj
 * Fairly major modifications to support different authentication credentials
 *   in ARQ to the logged in ones on RRQ. For both client and server.
 * Various other H.235 authentication bugs and anomalies fixed on the way.
 *
 * Revision 1.289  2002/07/29 05:31:49  robertj
 * Fixed issues with getting CONNECT but no media causing a EndedByNoAnswer
 *   call clearing. Now causes an EndedByCapabilityExchange clearance.
 *
 * Revision 1.288  2002/07/25 10:55:44  robertj
 * Changes to allow more granularity in PDU dumps, hex output increasing
 *   with increasing trace level.
 *
 * Revision 1.287  2002/07/18 08:31:05  robertj
 * Moved OnSetLocalCapabilities() to before call-proceeding response as it
 *   needs to be before the TCS that might be in the SETUP pdu is processed.
 *
 * Revision 1.286  2002/07/18 05:10:40  robertj
 * Fixed problem with fast start adding "fake" remote capabilities regardless
 *   of that capability already being in the table because of TCS in SETUP.
 *
 * Revision 1.285  2002/07/12 06:09:39  robertj
 * Fixed race conditions due to missing Lock() in AnsweringCall()
 *
 * Revision 1.284  2002/07/07 02:19:17  robertj
 * Fixed sending of IRR's on release complete, thanks Ravelli Rossano
 *
 * Revision 1.283  2002/07/05 02:22:56  robertj
 * Added support for standard and non-standard T.38 mode change.
 *
 * Revision 1.282  2002/07/05 00:54:12  robertj
 * Allowed for backward motion in time (DST) with endSession timeout.
 *
 * Revision 1.281  2002/07/04 02:36:36  robertj
 * Added variable wait time for endSession to be received so do not get
 *   cumulative delay in clean up thread, thanks Federico Pinna
 *
 * Revision 1.280  2002/07/04 00:40:34  robertj
 * More H.450.11 call intrusion implementation, thanks Aleksandar Todorovic
 *
 * Revision 1.279  2002/07/03 09:37:25  robertj
 * Fixed bug from in 1.276 which causes failures to fast start channels.
 *
 * Revision 1.278  2002/07/02 10:02:32  robertj
 * Added H323TransportAddress::GetIpAddress() so don't have to provide port
 *   when you don't need it as in GetIpAndPort(),.
 *
 * Revision 1.277  2002/06/28 03:57:33  robertj
 * Added fix to prevent endSession timeout if remote endpoint crashes or
 *   disconnects TCP without having sent one.
 *
 * Revision 1.276  2002/06/25 08:30:12  robertj
 * Changes to differentiate between stright G.723.1 and G.723.1 Annex A using
 *   the OLC dataType silenceSuppression field so does not send SID frames
 *   to receiver codecs that do not understand them.
 *
 * Revision 1.275  2002/06/24 00:07:31  robertj
 * Fixed bandwidth usage being exactly opposite (adding when it should
 *   be subtracting), thanks Saswat Praharaj.
 *
 * Revision 1.274  2002/06/22 06:11:33  robertj
 * Fixed bug on sometimes missing received endSession causing 10 second
 *   timeout in connection clean up.
 *
 * Revision 1.273  2002/06/22 05:48:42  robertj
 * Added partial implementation for H.450.11 Call Intrusion
 *
 * Revision 1.272  2002/06/15 03:07:40  robertj
 * Fixed correct detect of consultation transfer, thanks Gilles Delcayre
 *
 * Revision 1.271  2002/06/13 06:15:19  robertj
 * Allowed TransferCall() to be used on H323Connection as well as H323EndPoint.
 *
 * Revision 1.270  2002/06/13 02:14:08  robertj
 * Removed automatic clear of call when requesting a facility redirect form of
 *   call forwarding. Causes interop problems with Cisco IOS.
 *
 * Revision 1.269  2002/06/07 02:18:20  robertj
 * Fixed GNU warning
 *
 * Revision 1.268  2002/06/06 03:06:31  robertj
 * Fixed unecessary delay in waiting for endSession if we actually do not
 *   need to send one so don;t get one back (ie no H.245).
 * Fixed crash when receive a tunneled H.245 PDU during connection shut down.
 * Fixed tunnelled endSession in release complete PDU.
 *
 * Revision 1.267  2002/06/05 05:29:50  robertj
 * Added code to detect if remote endpoint is Cisco IOS based gateway and
 *   set remoteApplication variable accordingly.
 * Added code to not do multiple tunnellled H.245 messages if Cisco IOS.
 *
 * Revision 1.266  2002/06/05 02:30:02  robertj
 * Changed round trip time calculation to operate as soon as H.245 allows it
 *   instead of when call is "established".
 *
 * Revision 1.265  2002/05/31 06:10:36  robertj
 * Removed unecessary mutex grab (causes deadlocks), thanks Federico Pinna
 *
 * Revision 1.264  2002/05/29 06:37:57  robertj
 * Changed sending of endSession/ReleaseComplete PDU's to occur immediately
 *   on call clearance and not wait for background thread to do it.
 * Stricter compliance by waiting for reply endSession before closing down.
 *
 * Revision 1.263  2002/05/29 03:55:21  robertj
 * Added protocol version number checking infrastructure, primarily to improve
 *   interoperability with stacks that are unforgiving of new features.
 *
 * Revision 1.262  2002/05/21 09:32:49  robertj
 * Added ability to set multiple alias names ona  connection by connection
 *   basis, defaults to endpoint list, thanks Artis Kugevics
 *
 * Revision 1.261  2002/05/16 00:57:27  robertj
 * Fixed problem where if get early start separate H.245 channel and it take
 *   more than 10 seconds for CONNECT then hangs up call.
 *
 * Revision 1.260  2002/05/16 00:00:37  robertj
 * Added memory management of created T.38 and T.120 handlers.
 * Added ability to initiate a mode change for non-standard T.38
 *
 * Revision 1.259  2002/05/07 03:18:15  robertj
 * Added application info (name/version etc) into registered endpoint data.
 *
 * Revision 1.258  2002/05/03 05:38:19  robertj
 * Added Q.931 Keypad IE mechanism for user indications (DTMF).
 *
 * Revision 1.257  2002/05/03 03:08:40  robertj
 * Added replacementFor field in OLC when resolving conflicting channels.
 *
 * Revision 1.256  2002/05/02 07:56:27  robertj
 * Added automatic clearing of call if no media (RTP data) is transferred in a
 *   configurable (default 5 minutes) amount of time.
 *
 * Revision 1.255  2002/05/02 05:19:29  robertj
 * Fixed processing of tunnelled H.245 in setup if have local tunnelling disabled.
 *
 * Revision 1.254  2002/05/01 05:59:05  robertj
 * Added resolution of simultaneous starth245 messages.
 *
 * Revision 1.253  2002/04/18 00:21:45  robertj
 * Fixed incorrect end call reason when call is abandoned by caller.
 *
 * Revision 1.252  2002/04/17 00:50:12  robertj
 * Added ability to disable the in band DTMF detection.
 *
 * Revision 1.251  2002/03/27 06:04:42  robertj
 * Added Temporary Failure end code for connection, an application may
 *   immediately retry the call if this occurs.
 *
 * Revision 1.250  2002/03/27 00:17:27  robertj
 * Fixed fall back from RFC2833 to H.245 UII if remote does not have capability.
 *
 * Revision 1.249  2002/03/26 08:47:14  robertj
 * Changed setting of RFC2833 payload type so depends on call direction (who
 *   answered) rather than master/slave. Fixed problem with TCS in SETUP packet.
 *
 * Revision 1.248  2002/03/19 05:16:13  robertj
 * Normalised ACF destExtraCallIInfo to be same as other parameters.
 *
 * Revision 1.247  2002/02/25 23:32:20  robertj
 * Fixed problem with "late" H.245 tunnelled messages arriving while switching
 *   to separate H.245 channal, thanks Chih-Wei Huang.
 *
 * Revision 1.246  2002/02/25 06:16:53  robertj
 * Fixed problem where sending a TCS in response to receiving a TCS, waits
 *    until we actually have local capabilities so don't get a false TCS=0
 *
 * Revision 1.245  2002/02/19 06:18:41  robertj
 * Improved tracing, especially re UserInput send modes.
 *
 * Revision 1.244  2002/02/11 04:17:33  robertj
 * Fixed bug where could send DRQ if never received an ACF.
 *
 * Revision 1.243  2002/02/06 06:30:26  craigs
 * Fixed problem whereby MSD/TCS was stalled if H245 was included in
 * SETUP, but other end did not respond
 *
 * Revision 1.242  2002/02/05 23:41:59  craigs
 * Fixed warning caused by invokeId being unsigned but compared to -1
 *
 * Revision 1.241  2002/02/04 07:17:56  robertj
 * Added H.450.2 Consultation Transfer, thanks Norwood Systems.
 *
 * Revision 1.240  2002/01/26 00:26:38  robertj
 * Fixed GNU warnings.
 *
 * Revision 1.239  2002/01/25 05:19:43  robertj
 * Moved static strings for enum printing to inside of function, could crash with DLL's
 *
 * Revision 1.238  2002/01/24 23:12:29  robertj
 * Fixed v4 H.245 start in SETUP pdu, should use parallelH245Control field.
 *
 * Revision 1.237  2002/01/24 06:29:05  robertj
 * Added option to disable H.245 negotiation in SETUP pdu, this required
 *   API change so have a bit mask instead of a series of booleans.
 *
 * Revision 1.236  2002/01/24 00:20:54  robertj
 * Fixed crash if receive release complete without H.323 part.
 *
 * Revision 1.235  2002/01/23 12:45:37  rogerh
 * Add the DTMF decoder. This identifies DTMF tones in an audio stream.
 *
 * Revision 1.234  2002/01/23 07:12:52  robertj
 * Added hooks for in band DTMF detection. Now need the detector!
 *
 * Revision 1.233  2002/01/22 22:48:25  robertj
 * Fixed RFC2833 support (transmitter) requiring large rewrite
 *
 * Revision 1.232  2002/01/22 07:08:26  robertj
 * Added IllegalPayloadType enum as need marker for none set
 *   and MaxPayloadType is a legal value.
 *
 * Revision 1.231  2002/01/20 22:59:53  robertj
 * Fixed compatibility with brain dead NetMeeting
 *
 * Revision 1.230  2002/01/18 06:01:38  robertj
 * Added some H323v4 functions (fastConnectRefused & TCS in SETUP)
 *
 * Revision 1.229  2002/01/17 07:05:03  robertj
 * Added support for RFC2833 embedded DTMF in the RTP stream.
 *
 * Revision 1.228  2002/01/14 00:06:51  robertj
 * Added H.450.6, better H.450.2 error handling and  and Music On Hold.
 * Added destExtraCallInfo field for ARQ.
 * Improved end of call status on ARJ responses.
 *   Thanks Ben Madsen of Norwood Systems
 *
 * Revision 1.227  2002/01/10 23:11:16  robertj
 * Fixed failure to pass session ID in capability create channel function, this
 *   only causes a problem with new external RTP sessions.
 *
 * Revision 1.226  2002/01/10 22:43:05  robertj
 * Added initialisation of T.38 mode change flag, thanks Vyacheslav Frolov
 *
 * Revision 1.225  2002/01/10 22:40:57  robertj
 * Fixed race condition with T.38 mode change variable, thanks Vyacheslav Frolov
 *
 * Revision 1.224  2002/01/10 05:13:54  robertj
 * Added support for external RTP stacks, thanks NuMind Software Systems.
 *
 * Revision 1.223  2002/01/09 00:21:39  robertj
 * Changes to support outgoing H.245 RequstModeChange.
 *
 * Revision 1.222  2002/01/07 01:42:07  robertj
 * Fixed default tunneling flag value when usinf connection constructor wirth
 *    3 parameters (works with 2 and 4 but not 3).
 *
 * Revision 1.221  2002/01/02 04:52:41  craigs
 * Added RequestModeChange thanks to Vyacheslav Frolov
 *
 * Revision 1.220  2001/12/22 03:21:03  robertj
 * Added create protocol function to H323Connection.
 *
 * Revision 1.219  2001/12/22 03:11:03  robertj
 * Changed OnRequstModeChange to return ack, then actually do the change.
 *
 * Revision 1.218  2001/12/22 01:53:24  robertj
 * Added more support for H.245 RequestMode operation.
 *
 * Revision 1.217  2001/12/15 09:10:21  robertj
 * Fixed uninitialised end time preventing correct time from being set in DRQ.
 *
 * Revision 1.216  2001/12/15 08:08:34  robertj
 * Added alerting, connect and end of call times to be sent to RAS server.
 *
 * Revision 1.215  2001/12/14 06:39:28  robertj
 * Broke out conversion of Q.850 and H.225 release complete codes to
 *   OpenH323 call end reasons enum.
 *
 * Revision 1.214  2001/12/13 11:03:00  robertj
 * Added ability to automatically add ACF access token to SETUP pdu.
 *
 * Revision 1.213  2001/11/22 02:30:13  robertj
 * Fixed GetSessionCodecNames so it actually returns the codec names.
 *
 * Revision 1.212  2001/11/09 05:39:54  craigs
 * Added initial T.38 support thanks to Adam Lazur
 *
 * Revision 1.211  2001/11/06 22:57:23  robertj
 * Added missing start of H.245 channel in SETUP PDU.
 *
 * Revision 1.210  2001/11/01 06:11:57  robertj
 * Plugged very small mutex hole that could cause crashes.
 *
 * Revision 1.209  2001/11/01 00:27:35  robertj
 * Added default Fast Start disabled and H.245 tunneling disable flags
 *   to the endpoint instance.
 *
 * Revision 1.208  2001/10/30 23:58:42  robertj
 * Fixed problem with transmit channels not being started
 *   under some circumstances (eg Cisco early start).
 *
 * Revision 1.207  2001/10/24 00:55:49  robertj
 * Made cosmetic changes to H.245 miscellaneous command function.
 *
 * Revision 1.206  2001/10/23 02:17:16  dereks
 * Initial release of cu30 video codec.
 *
 * Revision 1.205  2001/10/04 06:40:55  robertj
 * Fixed GNU warning
 *
 * Revision 1.204  2001/09/26 07:20:42  robertj
 * Fixed correct sending of new digits in overlap dialing, and stopped
 *   sending redundent ARQ's if dialing occurs quickly thanks Chris Purvis.
 *
 * Revision 1.203  2001/09/26 06:20:59  robertj
 * Fixed properly nesting connection locking and unlocking requiring a quite
 *   large change to teh implementation of how calls are answered.
 *
 * Revision 1.202  2001/09/22 01:51:57  robertj
 * Fixed SendMoreDigits() using Info Q.931 PDU to only send new digits.
 *
 * Revision 1.201  2001/09/19 03:30:55  robertj
 * Added some support for overlapped dialing, thanks Chris Purvis & Nick Hoath.
 *
 * Revision 1.200  2001/09/18 10:36:57  robertj
 * Allowed multiple overlapping requests in RAS channel.
 *
 * Revision 1.199  2001/09/13 06:48:15  robertj
 * Added call back functions for remaining Q.931/H.225 messages.
 * Added call back to allow modification of Release Complete,thanks Nick Hoath
 *
 * Revision 1.198  2001/09/12 07:48:05  robertj
 * Fixed various problems with tracing.
 *
 * Revision 1.197  2001/09/12 06:58:00  robertj
 * Added support for iNow Access Token from gk, thanks Nick Hoath
 *
 * Revision 1.196  2001/09/12 06:04:38  robertj
 * Added support for sending UUIE's to gk on request, thanks Nick Hoath
 *
 * Revision 1.195  2001/09/11 01:24:36  robertj
 * Added conditional compilation to remove video and/or audio codecs.
 *
 * Revision 1.194  2001/09/06 01:29:34  robertj
 * Fixed passing of 0 duration to OnUserInputTone if not present in PDU.
 *
 * Revision 1.193  2001/08/27 03:45:59  robertj
 * Added automatic setting of bearer capability transfer mode from H.323
 *    capabilities on connection at time of SETUP PDU.
 *
 * Revision 1.192  2001/08/22 06:54:51  robertj
 * Changed connection locking to use double mutex to guarantee that
 *   no threads can ever deadlock or access deleted connection.
 *
 * Revision 1.191  2001/08/16 07:49:19  robertj
 * Changed the H.450 support to be more extensible. Protocol handlers
 *   are now in separate classes instead of all in H323Connection.
 *
 * Revision 1.190  2001/08/13 01:27:03  robertj
 * Changed GK admission so can return multiple aliases to be used in
 *   setup packet, thanks Nick Hoath.
 *
 * Revision 1.189  2001/08/06 03:08:56  robertj
 * Fission of h323.h to h323ep.h & h323con.h, h323.h now just includes files.
 *
 * Revision 1.188  2001/08/03 03:49:57  craigs
 * Fixed problem with AnswerCallDeferredWithMedia not working with fastStart
 * on some machine
 *
 * Revision 1.187  2001/08/02 04:32:17  robertj
 * Added ability for AdmissionRequest to alter destination alias used in
 *   the outgoing call. Thanks Ben Madsen & Graeme Reid.
 *
 * Revision 1.186  2001/08/01 00:46:16  craigs
 * Added ability to early start without Alerting
 *
 * Revision 1.185  2001/07/19 09:29:47  robertj
 * Adde X.880 invoke reject if get global opcode.
 *
 * Revision 1.184  2001/07/17 04:44:31  robertj
 * Partial implementation of T.120 and T.38 logical channels.
 *
 * Revision 1.183  2001/07/17 04:13:06  robertj
 * Fixed subtle bug when have split codec conflict, did not use the remotes
 *   caabilities (used local) so end up with incorrect frames per packet.
 *
 * Revision 1.182  2001/07/06 02:28:25  robertj
 * Moved initialisation of local capabilities back to constructor for
 *   backward compatibility reasons.
 *
 * Revision 1.181  2001/07/05 04:19:13  robertj
 * Added call back for setting local capabilities.
 *
 * Revision 1.180  2001/07/04 09:02:07  robertj
 * Added more tracing
 *
 * Revision 1.179  2001/06/14 07:10:35  robertj
 * Removed code made redundent by previous change.
 *
 * Revision 1.178  2001/06/14 06:25:16  robertj
 * Added further H.225 PDU build functions.
 * Moved some functionality from connection to PDU class.
 *
 * Revision 1.177  2001/06/13 06:38:25  robertj
 * Added early start (media before connect) functionality.
 *
 * Revision 1.176  2001/06/05 03:14:41  robertj
 * Upgraded H.225 ASN to v4 and H.245 ASN to v7.
 *
 * Revision 1.175  2001/05/31 07:16:56  robertj
 * Improved debug output on decoding internal PDU's, especially if fails.
 * Added trace dump of H4501 PDU's
 *
 * Revision 1.174  2001/05/30 23:34:54  robertj
 * Added functions to send TCS=0 for transmitter side pause.
 *
 * Revision 1.173  2001/05/22 00:40:20  robertj
 * Fixed restart problem when in transmitter paused (TCS=0) state, thanks Paul van de Wijngaard
 *
 * Revision 1.172  2001/05/17 07:11:29  robertj
 * Added more call end types for common transport failure modes.
 *
 * Revision 1.171  2001/05/17 03:30:45  robertj
 * Fixed support for transmiter side paused (TCS=0), thanks Paul van de Wijngaard
 *
 * Revision 1.170  2001/05/10 02:09:55  robertj
 * Added more call end codes from Q.931, no answer and no response.
 * Changed call transfer call end code to be the same as call forward.
 *
 * Revision 1.169  2001/05/09 04:59:04  robertj
 * Bug fixes in H.450.2, thanks Klein Stefan.
 *
 * Revision 1.168  2001/05/09 04:07:55  robertj
 * Added more call end codes for busy and congested.
 *
 * Revision 1.167  2001/05/03 06:49:43  robertj
 * Added bullet proofing for if get decode error in Q.931 User-User data.
 *
 * Revision 1.166  2001/05/02 16:22:21  rogerh
 * Add IsAllow() for a single capability to check if it is in the
 * capabilities set. This fixes the bug where OpenH323 would accept
 * incoming H261 video even when told not to accept it.
 *
 * Revision 1.165  2001/05/01 04:34:11  robertj
 * Changed call transfer API slightly to be consistent with new hold function.
 *
 * Revision 1.164  2001/05/01 02:12:50  robertj
 * Added H.450.4 call hold (Near End only), thanks David M. Cassel.
 *
 * Revision 1.163  2001/04/23 01:31:15  robertj
 * Improved the locking of connections especially at shutdown.
 *
 * Revision 1.162  2001/04/20 02:32:07  robertj
 * Improved logging of bandwith, used more intuitive units.
 *
 * Revision 1.161  2001/04/20 02:16:53  robertj
 * Removed GNU C++ warnings.
 *
 * Revision 1.160  2001/04/11 03:01:29  robertj
 * Added H.450.2 (call transfer), thanks a LOT to Graeme Reid & Norwood Systems
 *
 * Revision 1.159  2001/04/09 08:47:28  robertj
 * Added code to force the bringing up of a separate H.245 channel if not
 *   tunnelling and the call receiver does not start separate channel.
 *
 * Revision 1.158  2001/04/04 03:13:12  robertj
 * Made sure tunnelled H.245 is not used if separate channel is opened.
 * Made sure H.245 TCS & MSD is done if initiated the call and fast started,
 *   do so on receipt of the connect from remote end.
 * Utilised common function for TCS and MSD initiation.
 *
 * Revision 1.157  2001/03/08 07:45:06  robertj
 * Fixed issues with getting media channels started in some early start
 *   regimes, in particular better Cisco compatibility.
 *
 * Revision 1.156  2001/03/06 04:44:47  robertj
 * Fixed problem where could send capability set twice. This should not be
 *   a problem except when talking to another broken stack, eg Cisco routers.
 *
 * Revision 1.155  2001/02/09 05:13:55  craigs
 * Added pragma implementation to (hopefully) reduce the executable image size
 * under Linux
 *
 * Revision 1.154  2001/01/19 06:56:58  robertj
 * Fixed possible nested mutex of H323Connection if get PDU which waiting on answer.
 *
 * Revision 1.153  2001/01/11 06:40:37  craigs
 * Fixed problem with ClearCallSychronous failing because ClearCall was
 * called before it finished
 *
 * Revision 1.152  2001/01/03 05:59:07  robertj
 * Fixed possible deadlock that caused asserts about transport thread
 *    not terminating when clearing calls.
 *
 * Revision 1.151  2000/12/19 22:33:44  dereks
 * Adjust so that the video channel is used for reading/writing raw video
 * data, which better modularizes the video codec.
 *
 * Revision 1.150  2000/12/18 08:59:20  craigs
 * Added ability to set ports
 *
 * Revision 1.149  2000/11/27 02:44:06  craigs
 * Added ClearCall Synchronous to H323Connection and H323Endpoint to
 * avoid race conditions with destroying descendant classes
 *
 * Revision 1.148  2000/11/24 10:53:59  robertj
 * Fixed bug in fast started G.711 codec not working in one direction.
 *
 * Revision 1.147  2000/11/15 00:14:06  robertj
 * Added missing state to FastStartState strings, thanks Ulrich Findeisen
 *
 * Revision 1.146  2000/11/12 23:49:16  craigs
 * Added per connection versions of OnEstablished and OnCleared
 *
 * Revision 1.145  2000/11/08 04:44:02  robertj
 * Added function to be able to alter/remove the call proceeding PDU.
 *
 * Revision 1.144  2000/10/30 03:37:54  robertj
 * Fixed opeing of separate H.245 channel if fast start but not tunneling.
 *
 * Revision 1.143  2000/10/19 04:09:03  robertj
 * Fix for if early start opens channel which fast start still pending.
 *
 * Revision 1.142  2000/10/16 09:51:38  robertj
 * Fixed problem with not opening fast start video receive if do not have transmit enabled.
 *
 * Revision 1.141  2000/10/13 02:16:04  robertj
 * Added support for Progress Indicator Q.931/H.225 message.
 *
 * Revision 1.140  2000/10/12 05:11:54  robertj
 * Added trace log if get transport error on writing PDU.
 *
 * Revision 1.139  2000/10/04 12:21:06  robertj
 * Changed setting of callToken in H323Connection to be as early as possible.
 *
 * Revision 1.138  2000/10/04 05:58:42  robertj
 * Minor reorganisation of the H.245 secondary channel start up to make it simpler
 *    to override its behaviour.
 * Moved OnIncomingCall() so occurs before fast start elements are processed
 *    allowing them to be manipulated before the default processing.
 *
 * Revision 1.137  2000/09/25 06:48:11  robertj
 * Removed use of alias if there is no alias present, ie only have transport address.
 *
 * Revision 1.136  2000/09/22 04:06:12  robertj
 * Fixed race condition when attempting fast start and remote refuses both.
 *
 * Revision 1.135  2000/09/22 01:35:49  robertj
 * Added support for handling LID's that only do symmetric codecs.
 *
 * Revision 1.134  2000/09/22 00:32:33  craigs
 * Added extra logging
 * Fixed problems with no fastConnect with tunelling
 *
 * Revision 1.133  2000/09/20 01:50:21  craigs
 * Added ability to set jitter buffer on a per-connection basis
 *
 * Revision 1.132  2000/09/05 01:16:19  robertj
 * Added "security" call end reason code.
 *
 * Revision 1.131  2000/08/23 05:49:58  robertj
 * Fixed possible deadlock on clean up of connection and error on media channel occurs.
 *
 * Revision 1.130  2000/08/21 12:37:14  robertj
 * Fixed race condition if close call just as slow start media channels are opening, part 2.
 *
 * Revision 1.129  2000/08/21 02:50:28  robertj
 * Fixed race condition if close call just as slow start media channels are opening.
 *
 * Revision 1.128  2000/08/10 02:48:52  craigs
 * Fixed problem with operator precedence causing funny problems with codec selection
 *
 * Revision 1.127  2000/08/01 11:27:36  robertj
 * Added more tracing for user input.
 *
 * Revision 1.126  2000/07/31 14:08:18  robertj
 * Added fast start and H.245 tunneling flags to the H323Connection constructor so can
 *    disabled these features in easier manner to overriding virtuals.
 *
 * Revision 1.125  2000/07/15 09:56:37  robertj
 * Changed adding of fake remote capability to after fast start element has been
 *   checked and new parameters (eg frames per packet) has been parsed out of it.
 *
 * Revision 1.124  2000/07/14 14:03:47  robertj
 * Changed fast start connect so if has bad h245 address does not clear call.
 *
 * Revision 1.123  2000/07/13 16:07:22  robertj
 * Fixed ability to receive a H.245 secondary TCP link in a Connect during a fast start.
 *
 * Revision 1.122  2000/07/13 12:35:13  robertj
 * Fixed problems with fast start frames per packet adjustment.
 * Split autoStartVideo so can select receive and transmit independently
 *
 * Revision 1.121  2000/07/10 16:06:59  robertj
 * Added TCS=0 support.
 * Fixed bug where negotiations hang if not fast start and tunnelled but remot does not tunnel.
 *
 * Revision 1.120  2000/07/09 15:24:42  robertj
 * Removed sending of MSD in Setup PDU, is not to spec.
 * Added start of TCS if received TCS from other end and have not already started.
 *
 * Revision 1.119  2000/07/08 19:49:27  craigs
 * Fixed stupidity in handling of fastStart capabilities
 *
 * Revision 1.118  2000/07/04 09:00:54  robertj
 * Added traces for indicating failure of channel establishment due to capability set rules.
 *
 * Revision 1.117  2000/07/04 04:15:38  robertj
 * Fixed capability check of "combinations" for fast start cases.
 *
 * Revision 1.116  2000/07/04 01:16:49  robertj
 * Added check for capability allowed in "combinations" set, still needs more done yet.
 *
 * Revision 1.115  2000/06/17 09:14:13  robertj
 * Moved sending of DRQ to after release complete to be closer to spec, thanks Thien Nguyen
 *
 * Revision 1.114  2000/06/07 05:48:06  robertj
 * Added call forwarding.
 *
 * Revision 1.113  2000/06/05 06:33:08  robertj
 * Fixed problem with roud trip time statistic not being calculated if constant traffic.
 *
 * Revision 1.112  2000/06/03 03:16:39  robertj
 * Fixed using the wrong capability table (should be connections) for some operations.
 *
 * Revision 1.111  2000/05/23 11:32:36  robertj
 * Rewrite of capability table to combine 2 structures into one and move functionality into that class
 *    allowing some normalisation of usage across several applications.
 * Changed H323Connection so gets a copy of capabilities instead of using endponts, allows adjustments
 *    to be done depending on the remote client application.
 *
 * Revision 1.110  2000/05/22 05:21:36  robertj
 * Fixed race condition where controlChannel variable could be used before set.
 *
 * Revision 1.109  2000/05/16 08:14:30  robertj
 * Added function to find channel by session ID, supporting H323Connection::FindChannel() with mutex.
 * Added function to get a logical channel by channel number.
 *
 * Revision 1.108  2000/05/16 02:06:00  craigs
 * Added access functions for particular sessions
 *
 * Revision 1.107  2000/05/09 12:19:31  robertj
 * Added ability to get and set "distinctive ring" Q.931 functionality.
 *
 * Revision 1.106  2000/05/09 04:12:36  robertj
 * Changed fast start to offer multiple codecs instead of only the preferred one.
 *
 * Revision 1.105  2000/05/08 14:32:26  robertj
 * Fixed GNU compiler compatibility problems.
 *
 * Revision 1.104  2000/05/08 14:07:35  robertj
 * Improved the provision and detection of calling and caller numbers, aliases and hostnames.
 *
 * Revision 1.103  2000/05/05 05:05:55  robertj
 * Removed warning in GNU compiles.
 *
 * Revision 1.102  2000/05/02 04:32:26  robertj
 * Fixed copyright notice comment.
 *
 * Revision 1.101  2000/04/28 13:01:44  robertj
 * Fixed problem with adjusting tx/rx frame counts in capabilities during fast start.
 *
 * Revision 1.100  2000/04/18 23:10:49  robertj
 * Fixed bug where fast starts two video recievers instread of receiver and transmitter.
 *
 * Revision 1.99  2000/04/14 21:09:51  robertj
 * Work around for compatibility problem wth broken Altigen AltaServ-IG PBX.
 *
 * Revision 1.98  2000/04/14 20:03:05  robertj
 * Added function to get remote endpoints application name.
 *
 * Revision 1.97  2000/04/14 17:31:05  robertj
 * Fixed bug where checking the error code on the wrong channel. Caused hang ups.
 *
 * Revision 1.96  2000/04/13 18:08:46  robertj
 * Fixed error in SendUserInput() check for value string, previous change was seriously broken.
 *
 * Revision 1.95  2000/04/11 20:07:01  robertj
 * Added missing trace strings for new call end reasons on gatekeeper denied calls.
 *
 * Revision 1.94  2000/04/11 04:02:48  robertj
 * Improved call initiation with gatekeeper, no longer require @address as
 *    will default to gk alias if no @ and registered with gk.
 * Added new call end reasons for gatekeeper denied calls.
 *
 * Revision 1.93  2000/04/10 20:37:33  robertj
 * Added support for more sophisticated DTMF and hook flash user indication.
 *
 * Revision 1.92  2000/04/06 17:50:16  robertj
 * Added auto-start (including fast start) of video channels, selectable via boolean on the endpoint.
 *
 * Revision 1.91  2000/04/05 03:17:31  robertj
 * Added more RTP statistics gathering and H.245 round trip delay calculation.
 *
 * Revision 1.90  2000/03/29 04:42:19  robertj
 * Improved some trace logging messages.
 *
 * Revision 1.89  2000/03/29 02:14:45  robertj
 * Changed TerminationReason to CallEndReason to use correct telephony nomenclature.
 * Added CallEndReason for capability exchange failure.
 *
 * Revision 1.88  2000/03/27 22:43:11  robertj
 * Fixed possible write to closed channel on shutdown when control channel separate.
 *
 * Revision 1.87  2000/03/25 02:01:50  robertj
 * Added adjustable caller name on connection by connection basis.
 *
 * Revision 1.86  2000/03/22 01:30:34  robertj
 * Fixed race condition in accelerated start (Cisco compatibility).
 *
 * Revision 1.85  2000/03/21 01:43:59  robertj
 * Fixed (faint) race condition when starting separate H.245 channel.
 *
 * Revision 1.84  2000/03/21 01:08:10  robertj
 * Fixed incorrect call reference code being used in originated call.
 *
 * Revision 1.83  2000/03/07 13:54:28  robertj
 * Fixed assert when cancelling call during TCP connect, thanks Yura Ershov.
 *
 * Revision 1.82  2000/03/02 02:18:13  robertj
 * Further fixes for early H245 establishment confusing the fast start code.
 *
 * Revision 1.81  2000/03/01 02:09:51  robertj
 * Fixed problem if H245 channel established before H225 connect.
 *
 * Revision 1.80  2000/01/07 08:21:32  robertj
 * Added status functions for connection and tidied up the answer call function
 *
 * Revision 1.79  2000/01/04 01:06:06  robertj
 * Removed redundent code, thanks Dave Harvey.
 *
 * Revision 1.78  2000/01/04 00:14:53  craigs
 * Added extra states to AnswerCall
 *
 * Revision 1.77  1999/12/23 23:02:35  robertj
 * File reorganision for separating RTP from H.323 and creation of LID for VPB support.
 *
 * Revision 1.76  1999/12/11 02:21:00  robertj
 * Added ability to have multiple aliases on local endpoint.
 *
 * Revision 1.75  1999/12/09 20:30:54  robertj
 * Fixed problem with receiving multiple fast start open fields in multiple PDU's.
 *
 * Revision 1.74  1999/11/23 03:38:51  robertj
 * Fixed yet another call termination reason code error.
 *
 * Revision 1.73  1999/11/22 10:07:23  robertj
 * Fixed some errors in correct termination states.
 *
 * Revision 1.72  1999/11/20 04:36:56  robertj
 * Fixed setting of transmitter channel numbers on receiving fast start.
 *
 * Revision 1.71  1999/11/19 13:00:25  robertj
 * Added call token into traces so can tell which connection is being cleaned up.
 *
 * Revision 1.70  1999/11/19 08:16:08  craigs
 * Added connectionStartTime
 *
 * Revision 1.69  1999/11/17 04:22:59  robertj
 * Fixed bug in incorrect termination state when making a fast start call.
 *
 * Revision 1.68  1999/11/17 00:01:12  robertj
 * Improved determination of caller name, thanks Ian MacDonald
 *
 * Revision 1.67  1999/11/14 11:25:34  robertj
 * Fixed bug with channel close callback being called twice when fast starting.
 *
 * Revision 1.66  1999/11/13 14:11:24  robertj
 * Fixed incorrect state on fast start receive.
 *
 * Revision 1.65  1999/11/10 23:29:45  robertj
 * Changed OnAnswerCall() call back function  to allow for asyncronous response.
 *
 * Revision 1.64  1999/11/06 11:58:24  robertj
 * Changed clean up to delete logical channels before channel destructor is called.
 *
 * Revision 1.63  1999/11/06 11:03:58  robertj
 * Fixed bug in fast start with H245 channel opening multiple channels.
 * Fixed bug in clean up, write of release complete if TCP connection failed.
 *
 * Revision 1.62  1999/11/06 05:37:45  robertj
 * Complete rewrite of termination of connection to avoid numerous race conditions.
 *
 * Revision 1.61  1999/11/05 08:24:43  robertj
 * Fixed bug in receiver refusing fast start, then not being able to start normally.
 *
 * Revision 1.60  1999/10/30 12:34:46  robertj
 * Added information callback for closed logical channel on H323EndPoint.
 *
 * Revision 1.59  1999/10/29 14:19:14  robertj
 * Fixed incorrect termination code when connection closed locally.
 *
 * Revision 1.58  1999/10/29 02:26:18  robertj
 * Added reason for termination code to H323Connection.
 *
 * Revision 1.57  1999/10/19 00:04:57  robertj
 * Changed OpenAudioChannel and OpenVideoChannel to allow a codec AttachChannel with no autodelete.
 *
 * Revision 1.56  1999/10/16 03:47:49  robertj
 * Fixed termination of gatekeeper RAS thread problem
 *
 * Revision 1.55  1999/10/14 12:05:03  robertj
 * Fixed deadlock possibilities in clearing calls.
 *
 * Revision 1.54  1999/10/10 08:59:47  robertj
 * no message
 *
 * Revision 1.53  1999/10/09 01:18:23  craigs
 * Added codecs to OpenAudioChannel and OpenVideoDevice functions
 *
 * Revision 1.52  1999/10/08 08:31:18  robertj
 * Fixed problem with fast start fall back to capability exchange
 *
 * Revision 1.51  1999/10/07 03:26:58  robertj
 * Fixed some fast-start compatbility problems.
 *
 * Revision 1.50  1999/09/23 07:33:53  robertj
 * Fixed some fast start/245 tunnelling bugs, some odd cases.
 *
 * Revision 1.49  1999/09/23 07:25:12  robertj
 * Added open audio and video function to connection and started multi-frame codec send functionality.
 *
 * Revision 1.48  1999/09/21 14:09:39  robertj
 * Removed warnings when no tracing enabled.
 *
 * Revision 1.47  1999/09/15 01:26:27  robertj
 * Changed capability set call backs to have more specific class as parameter.
 *
 * Revision 1.46  1999/09/14 14:26:17  robertj
 * Added more debug tracing.
 *
 * Revision 1.45  1999/09/10 03:36:48  robertj
 * Added simple Q.931 Status response to Q.931 Status Enquiry
 *
 * Revision 1.44  1999/09/08 04:05:49  robertj
 * Added support for video capabilities & codec, still needs the actual codec itself!
 *
 * Revision 1.43  1999/08/31 12:34:19  robertj
 * Added gatekeeper support.
 *
 * Revision 1.42  1999/08/27 15:42:44  craigs
 * Fixed problem with local call tokens using ambiguous interface names, and connect timeouts not changing connection state
 *
 * Revision 1.41  1999/08/25 05:11:22  robertj
 * File fission (critical mass reached).
 * Improved way in which remote capabilities are created, removed case statement!
 * Changed MakeCall, so immediately spawns thread, no black on TCP connect.
 *
 * Revision 1.40  1999/08/14 03:26:45  robertj
 * Compiler compatibility problems.
 *
 * Revision 1.39  1999/08/10 04:20:26  robertj
 * Fixed constness problems in some PASN_Choice casts.
 *
 * Revision 1.38  1999/08/08 10:03:33  robertj
 * Fixed capability selection to honor local table priority order.
 *
 * Revision 1.37  1999/07/23 02:37:53  robertj
 * Fixed problems with hang ups and crash closes of connections.
 *
 * Revision 1.36  1999/07/19 02:01:02  robertj
 * Fixeed memory leask on connection termination.
 * Fixed connection "orderly close" on endpoint termination.
 *
 * Revision 1.35  1999/07/18 14:57:29  robertj
 * Fixed bugs in slow start with H245 tunnelling, part 4.
 *
 * Revision 1.34  1999/07/18 14:29:31  robertj
 * Fixed bugs in slow start with H245 tunnelling, part 3.
 *
 * Revision 1.33  1999/07/18 13:59:12  robertj
 * Fixed bugs in slow start with H245 tunnelling, part 2.
 *
 * Revision 1.32  1999/07/18 13:25:40  robertj
 * Fixed bugs in slow start with H245 tunnelling.
 *
 * Revision 1.31  1999/07/18 10:19:39  robertj
 * Fixed CreateCapability function: missing break's in case!
 *
 * Revision 1.30  1999/07/17 06:22:32  robertj
 * Fixed bug in setting up control channel when initiating call (recently introduced)
 *
 * Revision 1.29  1999/07/16 16:04:41  robertj
 * Bullet proofed local capability table data entry.
 *
 * Revision 1.28  1999/07/16 14:04:47  robertj
 * Fixed bug that caused signal channel to be closed, forgot to disable connect time.
 *
 * Revision 1.27  1999/07/16 06:15:59  robertj
 * Corrected semantics for tunnelled master/slave determination in fast start.
 *
 * Revision 1.26  1999/07/16 02:15:20  robertj
 * Fixed more tunneling problems.
 * Fixed fastStart initiator matching response to correct channels.
 *
 * Revision 1.25  1999/07/16 00:51:03  robertj
 * Some more debugging of fast start.
 *
 * Revision 1.24  1999/07/15 14:45:36  robertj
 * Added propagation of codec open error to shut down logical channel.
 * Fixed control channel start up bug introduced with tunnelling.
 *
 * Revision 1.23  1999/07/15 09:04:31  robertj
 * Fixed some fast start bugs
 *
 * Revision 1.22  1999/07/14 06:06:14  robertj
 * Fixed termination problems (race conditions) with deleting connection object.
 *
 * Revision 1.21  1999/07/13 09:53:24  robertj
 * Fixed some problems with jitter buffer and added more debugging.
 *
 * Revision 1.20  1999/07/13 02:50:58  craigs
 * Changed semantics of SetPlayDevice/SetRecordDevice, only descendent
 *    endpoint assumes PSoundChannel devices for audio codec.
 *
 * Revision 1.19  1999/07/10 02:59:26  robertj
 * Fixed ability to hang up incoming connection.
 *
 * Revision 1.18  1999/07/10 02:51:36  robertj
 * Added mutexing in H245 procedures. Also fixed MSD state bug.
 *
 * Revision 1.17  1999/07/09 14:59:59  robertj
 * Fixed GNU C++ compatibility.
 *
 * Revision 1.16  1999/07/09 06:09:49  robertj
 * Major implementation. An ENORMOUS amount of stuff added everywhere.
 *
 * Revision 1.15  1999/06/25 10:25:35  robertj
 * Added maintentance of callIdentifier variable in H.225 channel.
 *
 * Revision 1.14  1999/06/22 13:45:05  robertj
 * Added user question on listener version to accept incoming calls.
 *
 * Revision 1.13  1999/06/14 06:39:08  robertj
 * Fixed problem with getting transmit flag to channel from PDU negotiator
 *
 * Revision 1.12  1999/06/14 05:15:55  robertj
 * Changes for using RTP sessions correctly in H323 Logical Channel context
 *
 * Revision 1.11  1999/06/13 12:41:14  robertj
 * Implement logical channel transmitter.
 * Fixed H245 connect on receiving call.
 *
 * Revision 1.10  1999/06/09 06:18:00  robertj
 * GCC compatibiltiy.
 *
 * Revision 1.9  1999/06/09 05:26:19  robertj
 * Major restructuring of classes.
 *
 * Revision 1.8  1999/06/07 00:37:05  robertj
 * Allowed for reuseaddr on listen when in debug version
 *
 * Revision 1.7  1999/06/06 06:06:36  robertj
 * Changes for new ASN compiler and v2 protocol ASN files.
 *
 * Revision 1.6  1999/04/26 06:20:22  robertj
 * Fixed bugs in protocol
 *
 * Revision 1.5  1999/04/26 06:14:46  craigs
 * Initial implementation for RTP decoding and lots of stuff
 * As a whole, these changes are called "First Noise"
 *
 * Revision 1.4  1999/02/23 11:04:28  robertj
 * Added capability to make outgoing call.
 *
 * Revision 1.3  1999/01/16 01:31:33  robertj
 * Major implementation.
 *
 * Revision 1.2  1999/01/02 04:00:56  robertj
 * Added higher level protocol negotiations.
 *
 * Revision 1.1  1998/12/14 09:13:09  robertj
 * Initial revision
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "h323con.h"
#endif

#include <h323/h323con.h>

#include <h323/h323ep.h>
#include <h323/h323neg.h>
#include <h323/h323rtp.h>
#include <h323/gkclient.h>
#include <h323/h450pdu.h>
#include <h323/transaddr.h>
#include <opal/call.h>
#include <opal/patch.h>
#include <codec/rfc2833.h>


const PTimeInterval MonitorCallStatusTime(0, 10); // Seconds

#define new PNEW


#if !PTRACING // Stuff to remove unised parameters warning
#define PTRACE_pdu
#endif


/////////////////////////////////////////////////////////////////////////////

#if PTRACING

ostream & operator<<(ostream & o, H323Connection::SendUserInputModes m)
{
  static const char * const SendUserInputModeNames[H323Connection::NumSendUserInputModes] = {
    "SendUserInputAsQ931",
    "SendUserInputAsString",
    "SendUserInputAsTone",
    "SendUserInputAsRFC2833",
    "SendUserInputAsSeparateRFC2833"
  };

  if ((PINDEX)m >= PARRAYSIZE(SendUserInputModeNames))
    o << "InvalidSendUserInputMode<" << (unsigned)m << '>';
  else if (SendUserInputModeNames[m] == NULL)
    o << "SendUserInputMode<" << (unsigned)m << '>';
  else
    o << SendUserInputModeNames[m];
  return o;
}


const char * const H323Connection::ConnectionStatesNames[NumConnectionStates] = {
  "NoConnectionActive",
  "AwaitingGatekeeperAdmission",
  "AwaitingTransportConnect",
  "AwaitingSignalConnect",
  "AwaitingLocalAnswer",
  "HasExecutedSignalConnect",
  "EstablishedConnection",
  "ShuttingDownConnection"
};

const char * const H323Connection::FastStartStateNames[NumFastStartStates] = {
  "FastStartDisabled",
  "FastStartInitiate",
  "FastStartResponse",
  "FastStartAcknowledged"
};
#endif


H323Connection::H323Connection(OpalCall & call,
                               H323EndPoint & ep,
                               const PString & token,
                               const PString & alias,
                               const H323TransportAddress & address,
                               unsigned options)
  : OpalConnection(call, ep, token),
    endpoint(ep),
    gkAccessTokenOID(ep.GetGkAccessTokenOID())
{
  if (alias.IsEmpty())
    remotePartyName = remotePartyAddress = address;
  else {
    remotePartyName = alias;
    remotePartyAddress = alias + '@' + address;
  }

  mediaStreams.DisallowDeleteObjects();

  gatekeeperRouted = FALSE;
  distinctiveRing = 0;
  callReference = token.Mid(token.Find('/')+1).AsUnsigned();
  remoteCallWaiting = -1;

  h225version = H225_PROTOCOL_VERSION;
  h245version = H245_PROTOCOL_VERSION;
  h245versionSet = FALSE;

  signallingChannel = NULL;
  controlChannel = NULL;
  controlListener = NULL;
  holdMediaChannel = NULL;
  isConsultationTransfer = FALSE;
  isCallIntrusion = FALSE;
  callIntrusionProtectionLevel = endpoint.GetCallIntrusionProtectionLevel();

  switch (options&H245TunnelingOptionMask) {
    case H245TunnelingOptionDisable :
      h245Tunneling = FALSE;
      break;

    case H245TunnelingOptionEnable :
      h245Tunneling = TRUE;
      break;

    default :
      h245Tunneling = !ep.IsH245TunnelingDisabled();
      break;
  }

  h245TunnelTxPDU = NULL;
  h245TunnelRxPDU = NULL;
  alertingPDU = NULL;
  connectPDU = NULL;

  connectionState = NoConnectionActive;
  q931Cause = Q931::ErrorInCauseIE;

  uuiesRequested = 0; // Empty set
  addAccessTokenToSetup = TRUE; // Automatic inclusion of ACF access token in SETUP
  sendUserInputMode = endpoint.GetSendUserInputMode();

  mediaWaitForConnect = FALSE;
  transmitterSidePaused = FALSE;
  transmitterMediaStream = NULL;

  switch (options&FastStartOptionMask) {
    case FastStartOptionDisable :
      fastStartState = FastStartDisabled;
      break;

    case FastStartOptionEnable :
      fastStartState = FastStartInitiate;
      break;

    default :
      fastStartState = ep.IsFastStartDisabled() ? FastStartDisabled : FastStartInitiate;
      break;
  }

  mustSendDRQ = FALSE;
  earlyStart = FALSE;
  startT120 = TRUE;
  lastPDUWasH245inSETUP = FALSE;
  endSessionNeeded = FALSE;

  switch (options&H245inSetupOptionMask) {
    case H245inSetupOptionDisable :
      doH245inSETUP = FALSE;
      break;

    case H245inSetupOptionEnable :
      doH245inSETUP = TRUE;
      break;

    default :
      doH245inSETUP = !ep.IsH245inSetupDisabled();
      break;
  }

  remoteMaxAudioDelayJitter = 0;

  switch (options&DetectInBandDTMFOptionMask) {
    case DetectInBandDTMFOptionDisable :
      detectInBandDTMF = FALSE;
      break;

    case DetectInBandDTMFOptionEnable :
      detectInBandDTMF = TRUE;
      break;

    default :
      break;
  }

  masterSlaveDeterminationProcedure = new H245NegMasterSlaveDetermination(endpoint, *this);
  capabilityExchangeProcedure = new H245NegTerminalCapabilitySet(endpoint, *this);
  logicalChannels = new H245NegLogicalChannels(endpoint, *this);
  requestModeProcedure = new H245NegRequestMode(endpoint, *this);
  roundTripDelayProcedure = new H245NegRoundTripDelay(endpoint, *this);

  h450dispatcher = new H450xDispatcher(*this);
  h4502handler = new H4502Handler(*this, *h450dispatcher);
  h4504handler = new H4504Handler(*this, *h450dispatcher);
  h4506handler = new H4506Handler(*this, *h450dispatcher);
  h45011handler = new H45011Handler(*this, *h450dispatcher);

  remoteIsNAT = FALSE;
}


H323Connection::~H323Connection()
{
  delete masterSlaveDeterminationProcedure;
  delete capabilityExchangeProcedure;
  delete logicalChannels;
  delete requestModeProcedure;
  delete roundTripDelayProcedure;
  delete h450dispatcher;
  delete signallingChannel;
  delete controlChannel;
  delete alertingPDU;
  delete connectPDU;
  delete holdMediaChannel;

  PTRACE(3, "H323\tConnection " << callToken << " deleted.");
}


void H323Connection::OnReleased()
{
  OpalConnection::OnReleased();
  CleanUpOnCallEnd();
  OnCleared();
}


void H323Connection::CleanUpOnCallEnd()
{
  PTRACE(3, "H323\tConnection " << callToken << " closing: connectionState=" << connectionState);

  connectionState = ShuttingDownConnection;

  if (!callEndTime.IsValid())
    callEndTime = PTime();

  PTRACE(2, "H225\tSending release complete PDU: callRef=" << callReference);
  H323SignalPDU rcPDU;
  rcPDU.BuildReleaseComplete(*this);
  h450dispatcher->AttachToReleaseComplete(rcPDU);

  BOOL sendingReleaseComplete = OnSendReleaseComplete(rcPDU);

  if (endSessionNeeded) {
    if (sendingReleaseComplete)
      h245TunnelTxPDU = &rcPDU; // Piggy back H245 on this reply

    // Send an H.245 end session to the remote endpoint.
    H323ControlPDU pdu;
    pdu.BuildEndSessionCommand(H245_EndSessionCommand::e_disconnect);
    WriteControlPDU(pdu);
  }

  if (sendingReleaseComplete) {
    h245TunnelTxPDU = NULL;
    WriteSignalPDU(rcPDU);
  }

  // Check for gatekeeper and do disengage if have one
  if (mustSendDRQ) {
    H323Gatekeeper * gatekeeper = endpoint.GetGatekeeper();
    if (gatekeeper != NULL)
      gatekeeper->DisengageRequest(*this, H225_DisengageReason::e_normalDrop);
  }

  // Unblock sync points
  digitsWaitFlag.Signal();

  // Clean up any fast start "pending" channels we may have running.
  PINDEX i;
  for (i = 0; i < fastStartChannels.GetSize(); i++)
    fastStartChannels[i].Close();
  fastStartChannels.RemoveAll();

  // Dispose of all the logical channels
  logicalChannels->RemoveAll();

  if (endSessionNeeded) {
    // Calculate time since we sent the end session command so we do not actually
    // wait for returned endSession if it has already been that long
    PTimeInterval waitTime = endpoint.GetEndSessionTimeout();
    if (callEndTime.IsValid()) {
      PTime now;
      if (now > callEndTime) { // Allow for backward motion in time (DST change)
        waitTime -= now - callEndTime;
        if (waitTime < 0)
          waitTime = 0;
      }
    }

    // Wait a while for the remote to send an endSession
    PTRACE(4, "H323\tAwaiting end session from remote for " << waitTime << " seconds");
    if (!endSessionReceived.Wait(waitTime)) {
      PTRACE(3, "H323\tDid not receive an end session from remote.");
    }
  }

  phase = ReleasedPhase;

  // Wait for control channel to be cleaned up (thread ended).
  if (controlChannel != NULL)
    controlChannel->CloseWait();

  // Wait for signalling channel to be cleaned up (thread ended).
  if (signallingChannel != NULL)
    signallingChannel->CloseWait();

  PTRACE(1, "H323\tConnection " << callToken << " terminated.");
}


PString H323Connection::GetDestinationAddress()
{
  if (!localDestinationAddress)
    return localDestinationAddress;

  return OpalConnection::GetDestinationAddress();
}


void H323Connection::AttachSignalChannel(const PString & token,
                                         H323Transport * channel,
                                         BOOL answeringCall)
{
  originating = !answeringCall;

  if (signallingChannel != NULL && signallingChannel->IsOpen()) {
    PAssertAlways(PLogicError);
    return;
  }

  delete signallingChannel;
  signallingChannel = channel;

  // Set our call token for identification in endpoint dictionary
  callToken = token;
}


BOOL H323Connection::WriteSignalPDU(H323SignalPDU & pdu)
{
  PAssert(signallingChannel != NULL, PLogicError);

  lastPDUWasH245inSETUP = FALSE;

  if (signallingChannel != NULL && signallingChannel->IsOpen()) {
    pdu.m_h323_uu_pdu.m_h245Tunneling = h245Tunneling;

    H323Gatekeeper * gk = endpoint.GetGatekeeper();
    if (gk != NULL)
      gk->InfoRequestResponse(*this, pdu.m_h323_uu_pdu, TRUE);

    if (pdu.Write(*signallingChannel))
      return TRUE;
  }

  Release(EndedByTransportFail);
  return FALSE;
}


void H323Connection::HandleSignallingChannel()
{
  PAssert(signallingChannel != NULL, PLogicError);

  PTRACE(2, "H225\tReading PDUs: callRef=" << callReference);

  while (signallingChannel->IsOpen()) {
    H323SignalPDU pdu;
    if (pdu.Read(*signallingChannel)) {
      if (!HandleSignalPDU(pdu)) {
        Release(EndedByTransportFail);
        break;
      }
    }
    else if (signallingChannel->GetErrorCode() != PChannel::Timeout) {
      if (controlChannel == NULL || !controlChannel->IsOpen())
        Release(EndedByTransportFail);
      signallingChannel->Close();
      break;
    }
    else {
      switch (connectionState) {
        case AwaitingSignalConnect :
          // Had time out waiting for remote to send a CONNECT
          ClearCall(EndedByNoAnswer);
          break;
        case HasExecutedSignalConnect :
          // Have had minimum MonitorCallStatusTime delay since CONNECT but
          // still no media to move it to EstablishedConnection state. Must
          // thus not have any common codecs to use!
          ClearCall(EndedByCapabilityExchange);
          break;
        default :
          break;
      }
    }

    if (controlChannel == NULL)
      MonitorCallStatus();
  }

  // If we are the only link to the far end then indicate that we have
  // received endSession even if we hadn't, because we are now never going
  // to get one so there is no point in having CleanUpOnCallEnd wait.
  if (controlChannel == NULL)
    endSessionReceived.Signal();

  PTRACE(2, "H225\tSignal channel closed.");
}


BOOL H323Connection::HandleSignalPDU(H323SignalPDU & pdu)
{
  // Process the PDU.
  const Q931 & q931 = pdu.GetQ931();

  PTRACE(3, "H225\tHandling PDU: " << q931.GetMessageTypeName()
                    << " callRef=" << q931.GetCallReference());

  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked())
    return FALSE;

  if (phase >= ReleasingPhase) {
    // Continue to look for endSession/releaseComplete pdus
    if (pdu.m_h323_uu_pdu.m_h245Tunneling) {
      for (PINDEX i = 0; i < pdu.m_h323_uu_pdu.m_h245Control.GetSize(); i++) {
        PPER_Stream strm = pdu.m_h323_uu_pdu.m_h245Control[i].GetValue();
        if (!InternalEndSessionCheck(strm))
          break;
      }
    }
    if (q931.GetMessageType() == Q931::ReleaseCompleteMsg)
      endSessionReceived.Signal();
    return FALSE;
  }

  // If remote does not do tunneling, so we don't either. Note that if it
  // gets turned off once, it stays off for good.
  if (h245Tunneling && !pdu.m_h323_uu_pdu.m_h245Tunneling) {
    masterSlaveDeterminationProcedure->Stop();
    capabilityExchangeProcedure->Stop();
    h245Tunneling = FALSE;
  }

  h245TunnelRxPDU = &pdu;

  // Check for presence of supplementary services
  if (pdu.m_h323_uu_pdu.HasOptionalField(H225_H323_UU_PDU::e_h4501SupplementaryService)) {
    if (!h450dispatcher->HandlePDU(pdu)) { // Process H4501SupplementaryService APDU
      return FALSE;
    }
  }

  // Add special code to detect if call is from a Cisco and remoteApplication needs setting
  if (remoteApplication.IsEmpty() && pdu.m_h323_uu_pdu.HasOptionalField(H225_H323_UU_PDU::e_nonStandardControl)) {
    for (PINDEX i = 0; i < pdu.m_h323_uu_pdu.m_nonStandardControl.GetSize(); i++) {
      const H225_NonStandardIdentifier & id = pdu.m_h323_uu_pdu.m_nonStandardControl[i].m_nonStandardIdentifier;
      if (id.GetTag() == H225_NonStandardIdentifier::e_h221NonStandard) {
        const H225_H221NonStandard & h221 = id;
        if (h221.m_t35CountryCode == 181 && h221.m_t35Extension == 0 && h221.m_manufacturerCode == 18) {
          remoteApplication = "Cisco IOS\t12.x\t181/18";
          PTRACE(2, "H225\tSet remote application name: \"" << remoteApplication << '"');
          break;
        }
      }
    }
  }

  BOOL ok;
  switch (q931.GetMessageType()) {
    case Q931::SetupMsg :
      ok = OnReceivedSignalSetup(pdu);
      break;

    case Q931::CallProceedingMsg :
      ok = OnReceivedCallProceeding(pdu);
      break;

    case Q931::ProgressMsg :
      ok = OnReceivedProgress(pdu);
      break;

    case Q931::AlertingMsg :
      ok = OnReceivedAlerting(pdu);
      break;

    case Q931::ConnectMsg :
      ok = OnReceivedSignalConnect(pdu);
      break;

    case Q931::FacilityMsg :
      ok = OnReceivedFacility(pdu);
      break;

    case Q931::SetupAckMsg :
      ok = OnReceivedSignalSetupAck(pdu);
      break;

    case Q931::InformationMsg :
      ok = OnReceivedSignalInformation(pdu);
      break;

    case Q931::NotifyMsg :
      ok = OnReceivedSignalNotify(pdu);
      break;

    case Q931::StatusMsg :
      ok = OnReceivedSignalStatus(pdu);
      break;

    case Q931::StatusEnquiryMsg :
      ok = OnReceivedStatusEnquiry(pdu);
      break;

    case Q931::ReleaseCompleteMsg :
      OnReceivedReleaseComplete(pdu);
      ok = FALSE;
      break;

    default :
      ok = OnUnknownSignalPDU(pdu);
  }

  if (ok) {
    // Process tunnelled H245 PDU, if present.
    HandleTunnelPDU(NULL);

    // Check for establishment criteria met
    InternalEstablishedConnectionCheck();
  }

  h245TunnelRxPDU = NULL;

  PString digits = pdu.GetQ931().GetKeypad();
  if (!digits)
    OnUserInputString(digits);

  H323Gatekeeper * gk = endpoint.GetGatekeeper();
  if (gk != NULL)
    gk->InfoRequestResponse(*this, pdu.m_h323_uu_pdu, FALSE);

  return ok;
}


void H323Connection::HandleTunnelPDU(H323SignalPDU * txPDU)
{
  if (h245TunnelRxPDU == NULL || !h245TunnelRxPDU->m_h323_uu_pdu.m_h245Tunneling)
    return;

  if (!h245Tunneling && h245TunnelRxPDU->m_h323_uu_pdu.m_h323_message_body.GetTag() == H225_H323_UU_PDU_h323_message_body::e_setup)
    return;

  H323SignalPDU localTunnelPDU;
  if (txPDU != NULL)
    h245TunnelTxPDU = txPDU;
  else {
    /* Compensate for Cisco bug. IOS cannot seem to accept multiple tunnelled
       H.245 PDUs insode the same facility message */
    if (remoteApplication.Find("Cisco IOS") == P_MAX_INDEX) {
      // Not Cisco, so OK to tunnel multiple PDUs
      localTunnelPDU.BuildFacility(*this, TRUE);
      h245TunnelTxPDU = &localTunnelPDU;
    }
  }

  // if a response to a SETUP PDU containing TCS/MSD was ignored, then shutdown negotiations
  PINDEX i;
  if (lastPDUWasH245inSETUP && 
      (h245TunnelRxPDU->m_h323_uu_pdu.m_h245Control.GetSize() == 0) &&
      (h245TunnelRxPDU->GetQ931().GetMessageType() != Q931::CallProceedingMsg)) {
    PTRACE(4, "H225\tH.245 in SETUP ignored - resetting H.245 negotiations");
    masterSlaveDeterminationProcedure->Stop();
    lastPDUWasH245inSETUP = FALSE;
    capabilityExchangeProcedure->Stop();
  } else {
    for (i = 0; i < h245TunnelRxPDU->m_h323_uu_pdu.m_h245Control.GetSize(); i++) {
      PPER_Stream strm = h245TunnelRxPDU->m_h323_uu_pdu.m_h245Control[i].GetValue();
      HandleControlData(strm);
    }
  }

  // Make sure does not get repeated, clear tunnelled H.245 PDU's
  h245TunnelRxPDU->m_h323_uu_pdu.m_h245Control.SetSize(0);

  if (h245TunnelRxPDU->m_h323_uu_pdu.m_h323_message_body.GetTag() == H225_H323_UU_PDU_h323_message_body::e_setup) {
    H225_Setup_UUIE & setup = h245TunnelRxPDU->m_h323_uu_pdu.m_h323_message_body;

    if (setup.HasOptionalField(H225_Setup_UUIE::e_parallelH245Control)) {
      for (i = 0; i < setup.m_parallelH245Control.GetSize(); i++) {
        PPER_Stream strm = setup.m_parallelH245Control[i].GetValue();
        HandleControlData(strm);
      }

      // Make sure does not get repeated, clear tunnelled H.245 PDU's
      setup.m_parallelH245Control.SetSize(0);
    }
  }

  h245TunnelTxPDU = NULL;

  // If had replies, then send them off in their own packet
  if (txPDU == NULL && localTunnelPDU.m_h323_uu_pdu.m_h245Control.GetSize() > 0)
    WriteSignalPDU(localTunnelPDU);
}


static BOOL BuildFastStartList(const H323Channel & channel,
                               H225_ArrayOf_PASN_OctetString & array,
                               H323Channel::Directions reverseDirection)
{
  H245_OpenLogicalChannel open;
  const H323Capability & capability = channel.GetCapability();

  if (channel.GetDirection() != reverseDirection) {
    if (!capability.OnSendingPDU(open.m_forwardLogicalChannelParameters.m_dataType))
      return FALSE;
  }
  else {
    if (!capability.OnSendingPDU(open.m_reverseLogicalChannelParameters.m_dataType))
      return FALSE;

    open.m_forwardLogicalChannelParameters.m_multiplexParameters.SetTag(
                H245_OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters::e_none);
    open.m_forwardLogicalChannelParameters.m_dataType.SetTag(H245_DataType::e_nullData);
    open.IncludeOptionalField(H245_OpenLogicalChannel::e_reverseLogicalChannelParameters);
  }

  if (!channel.OnSendingPDU(open))
    return FALSE;

  PTRACE(4, "H225\tBuild fastStart:\n  " << setprecision(2) << open);
  PINDEX last = array.GetSize();
  array.SetSize(last+1);
  array[last].EncodeSubType(open);

  PTRACE(3, "H225\tBuilt fastStart for " << capability);
  return TRUE;
}

void H323Connection::OnEstablished()
{
  endpoint.OnConnectionEstablished(*this, callToken);
  endpoint.OnEstablished(*this);
}

void H323Connection::OnCleared()
{
  endpoint.OnConnectionCleared(*this, callToken);
}


void H323Connection::SetRemoteVersions(const H225_ProtocolIdentifier & protocolIdentifier)
{
  if (protocolIdentifier.GetSize() < 6)
    return;

  h225version = protocolIdentifier[5];

  if (h245versionSet) {
    PTRACE(3, "H225\tSet protocol version to " << h225version);
    return;
  }

  // If has not been told explicitly what the H.245 version use, make an
  // assumption based on the H.225 version
  switch (h225version) {
    case 1 :
      h245version = 2;  // H.323 version 1
      break;
    case 2 :
      h245version = 3;  // H.323 version 2
      break;
    case 3 :
      h245version = 5;  // H.323 version 3
      break;
    default :
      h245version = 7;  // H.323 version 4 (all we do so far)
  }
  PTRACE(3, "H225\tSet protocol version to " << h225version
         << " and implying H.245 version " << h245version);
}


BOOL H323Connection::OnReceivedSignalSetup(const H323SignalPDU & setupPDU)
{
  PINDEX i;

  if (setupPDU.m_h323_uu_pdu.m_h323_message_body.GetTag() != H225_H323_UU_PDU_h323_message_body::e_setup)
    return FALSE;

  const H225_Setup_UUIE & setup = setupPDU.m_h323_uu_pdu.m_h323_message_body;

  switch (setup.m_conferenceGoal.GetTag()) {
    case H225_Setup_UUIE_conferenceGoal::e_create:
    case H225_Setup_UUIE_conferenceGoal::e_join:
      break;

    case H225_Setup_UUIE_conferenceGoal::e_invite:
      return endpoint.OnConferenceInvite(setupPDU);

    case H225_Setup_UUIE_conferenceGoal::e_callIndependentSupplementaryService:
      return endpoint.OnCallIndependentSupplementaryService(setupPDU);

    case H225_Setup_UUIE_conferenceGoal::e_capability_negotiation:
      return endpoint.OnNegotiateConferenceCapabilities(setupPDU);
  }

  SetRemoteVersions(setup.m_protocolIdentifier);

  // Get the ring pattern
  distinctiveRing = setupPDU.GetDistinctiveRing();

  // Save the identifiers sent by caller
  if (setup.HasOptionalField(H225_Setup_UUIE::e_callIdentifier))
    callIdentifier = setup.m_callIdentifier.m_guid;
  conferenceIdentifier = setup.m_conferenceID;
  SetRemoteApplication(setup.m_sourceInfo);

  // Determine the remote parties name/number/address as best we can
  setupPDU.GetQ931().GetCallingPartyNumber(remotePartyNumber);
  remotePartyName = setupPDU.GetSourceAliases(signallingChannel);

  // get the peer address
  remotePartyAddress = signallingChannel->GetRemoteAddress();
  if (setup.m_sourceAddress.GetSize() > 0)
    remotePartyAddress = H323GetAliasAddressString(setup.m_sourceAddress[0]) + '@' + signallingChannel->GetRemoteAddress();

  // compare the source call signalling address
  if (setup.HasOptionalField(H225_Setup_UUIE::e_sourceCallSignalAddress)) {

    H323TransportAddress sourceAddress(setup.m_sourceCallSignalAddress);

    // if the source address is different from the peer address, then the remote endpoint is most likely behind a NAT
    // but make sure we don't have an external address set before enabling it
    PIPSocket::Address srcAddr, sigAddr;
    sourceAddress.GetIpAddress(srcAddr);
    signallingChannel->GetRemoteAddress().GetIpAddress(sigAddr);
    if (!sigAddr.IsRFC1918() && srcAddr.IsRFC1918()) {
      PIPSocket::Address localAddress = signallingChannel->GetLocalAddress();
      PIPSocket::Address ourAddress = localAddress;
      endpoint.TranslateTCPAddress(localAddress, sigAddr);
      if (localAddress == ourAddress) {
        PTRACE(3, "H225\tSource signal address " << srcAddr << " and TCP peer address " << sigAddr << " indicate remote endpoint is behind NAT");
        remoteIsNAT = TRUE;
      }
    }
  }

  // Anything else we need from setup PDU
  mediaWaitForConnect = setup.m_mediaWaitForConnect;
  if (!setupPDU.GetQ931().GetCalledPartyNumber(localDestinationAddress)) {
    localDestinationAddress = setupPDU.GetDestinationAlias(TRUE);
    if (signallingChannel->GetLocalAddress().IsEquivalent(localDestinationAddress))
      localDestinationAddress = '*';
  }

  // Send back a H323 Call Proceeding PDU in case OnIncomingCall() takes a while
  PTRACE(3, "H225\tSending call proceeding PDU");
  H323SignalPDU callProceedingPDU;
  H225_CallProceeding_UUIE & callProceeding = callProceedingPDU.BuildCallProceeding(*this);

  if (!isConsultationTransfer) {
    if (OnSendCallProceeding(callProceedingPDU)) {
      if (fastStartState == FastStartDisabled)
        callProceeding.IncludeOptionalField(H225_CallProceeding_UUIE::e_fastConnectRefused);

      if (!WriteSignalPDU(callProceedingPDU))
        return FALSE;
    }

    /** Here is a spot where we should wait in case of Call Intrusion
	for CIPL from other endpoints 
	if (isCallIntrusion) return TRUE;
    */

    // if the application indicates not to contine, then send a Q931 Release Complete PDU
    alertingPDU = new H323SignalPDU;
    alertingPDU->BuildAlerting(*this);

    /** If we have a case of incoming call intrusion we should not Clear the Call*/
    if (!OnIncomingCall(setupPDU, *alertingPDU) && (!isCallIntrusion)) {
      Release(EndedByNoAccept);
      PTRACE(1, "H225\tApplication not accepting calls");
      return FALSE;
    }

    // send Q931 Alerting PDU
    PTRACE(3, "H225\tIncoming call accepted");

    // Check for gatekeeper and do admission check if have one
    H323Gatekeeper * gatekeeper = endpoint.GetGatekeeper();
    if (gatekeeper != NULL) {
      H225_ArrayOf_AliasAddress destExtraCallInfoArray;
      H323Gatekeeper::AdmissionResponse response;
      response.destExtraCallInfo = &destExtraCallInfoArray;
      if (!gatekeeper->AdmissionRequest(*this, response)) {
        PTRACE(1, "H225\tGatekeeper refused admission: "
               << (response.rejectReason == UINT_MAX
                    ? PString("Transport error")
                    : H225_AdmissionRejectReason(response.rejectReason).GetTagName()));
        switch (response.rejectReason) {
          case H225_AdmissionRejectReason::e_calledPartyNotRegistered :
            Release(EndedByNoUser);
            break;
          case H225_AdmissionRejectReason::e_requestDenied :
            Release(EndedByNoBandwidth);
            break;
          case H225_AdmissionRejectReason::e_invalidPermission :
          case H225_AdmissionRejectReason::e_securityDenial :
            ClearCall(EndedBySecurityDenial);
            break;
          case H225_AdmissionRejectReason::e_resourceUnavailable :
            Release(EndedByRemoteBusy);
            break;
          default :
            Release(EndedByGatekeeper);
        }
        return FALSE;
      }

      if (destExtraCallInfoArray.GetSize() > 0)
        destExtraCallInfo = H323GetAliasAddressString(destExtraCallInfoArray[0]);
      mustSendDRQ = TRUE;
      gatekeeperRouted = response.gatekeeperRouted;
    }
  }

  // Get the local capabilities before fast start or tunnelled TCS is handled
  OnSetLocalCapabilities();

  // Check that it has the H.245 channel connection info
  if (setup.HasOptionalField(H225_Setup_UUIE::e_h245Address))
    if (!CreateOutgoingControlChannel(setup.m_h245Address))
      return FALSE;

  // See if remote endpoint wants to start fast
  if ((fastStartState != FastStartDisabled) && setup.HasOptionalField(H225_Setup_UUIE::e_fastStart)) {
    PTRACE(3, "H225\tFast start detected");

    // If we have not received caps from remote, we are going to build a
    // fake one from the fast connect data.
    if (!capabilityExchangeProcedure->HasReceivedCapabilities())
      remoteCapabilities.RemoveAll();

    // Extract capabilities from the fast start OpenLogicalChannel structures
    for (i = 0; i < setup.m_fastStart.GetSize(); i++) {
      H245_OpenLogicalChannel open;
      if (setup.m_fastStart[i].DecodeSubType(open)) {
        PTRACE(4, "H225\tFast start open:\n  " << setprecision(2) << open);
        unsigned error;
        H323Channel * channel = CreateLogicalChannel(open, TRUE, error);
        if (channel != NULL) {
          if (channel->GetDirection() == H323Channel::IsTransmitter)
            channel->SetNumber(logicalChannels->GetNextChannelNumber());
          fastStartChannels.Append(channel);
        }
      }
      else {
        PTRACE(1, "H225\tInvalid fast start PDU decode:\n  " << open);
      }
    }

    PTRACE(3, "H225\tOpened " << fastStartChannels.GetSize() << " fast start channels");

    // If we are incapable of ANY of the fast start channels, don't do fast start
    if (!fastStartChannels.IsEmpty())
      fastStartState = FastStartResponse;
  }

  // Build the reply with the channels we are actually using
  connectPDU = new H323SignalPDU;
  connectPDU->BuildConnect(*this);

  // OK are now ready to send SETUP to remote protocol
  ownerCall.OnSetUp(*this);

  if (connectionState == NoConnectionActive) {
    /** If Call Intrusion is allowed we must answer the call*/
    if (IsCallIntrusion()) {
      AnsweringCall(AnswerCallDeferred);
    }
    else {
      if (isConsultationTransfer)
        AnsweringCall(AnswerCallNow);
      else {
        // call the application callback to determine if to answer the call or not
        connectionState = AwaitingLocalAnswer;
        phase = AlertingPhase;
        AnsweringCall(OnAnswerCall(remotePartyName, setupPDU, *connectPDU));
      }
    }
  }

  return connectionState != ShuttingDownConnection;
}


void H323Connection::SetLocalPartyName(const PString & name)
{
  localPartyName = name;

  if (!name.IsEmpty()) {
    localAliasNames.RemoveAll();
    localAliasNames.AppendString(name);
  }
}


void H323Connection::SetRemotePartyInfo(const H323SignalPDU & pdu)
{
  PString newNumber;
  if (pdu.GetQ931().GetCalledPartyNumber(newNumber))
    remotePartyNumber = newNumber;

  PString remoteHostName = signallingChannel->GetRemoteAddress().GetHostName();

  PString newRemotePartyName = pdu.GetQ931().GetDisplayName();
  if (newRemotePartyName.IsEmpty() || newRemotePartyName == remoteHostName)
    remotePartyName = remoteHostName;
  else
    remotePartyName = newRemotePartyName + " [" + remoteHostName + ']';

  PTRACE(2, "H225\tSet remote party name: \"" << remotePartyName << '"');
}


void H323Connection::SetRemoteApplication(const H225_EndpointType & pdu)
{
  if (pdu.HasOptionalField(H225_EndpointType::e_vendor)) {
    remoteApplication = H323GetApplicationInfo(pdu.m_vendor);
    PTRACE(2, "H225\tSet remote application name: \"" << remoteApplication << '"');
  }
}


const PString H323Connection::GetRemotePartyCallbackURL() const
{
  PString remote;
  PINDEX j;
  
  if (IsGatekeeperRouted()) {

    remote = GetRemotePartyNumber();
  }
  
  if (remote.IsEmpty() && signallingChannel) {
  
    remote = signallingChannel->GetRemoteAddress();

    /* The address is formated the H.323 way */
    j = remote.FindLast(":");
    if (j != P_MAX_INDEX)
      remote = remote.Mid (j+1);
  }

  remote = "h323:" + remote;
  
  return remote;
}


BOOL H323Connection::OnReceivedSignalSetupAck(const H323SignalPDU & /*setupackPDU*/)
{
  OnInsufficientDigits();
  return TRUE;
}


BOOL H323Connection::OnReceivedSignalInformation(const H323SignalPDU & /*infoPDU*/)
{
  return TRUE;
}


BOOL H323Connection::OnReceivedCallProceeding(const H323SignalPDU & pdu)
{
  if (pdu.m_h323_uu_pdu.m_h323_message_body.GetTag() != H225_H323_UU_PDU_h323_message_body::e_callProceeding)
    return FALSE;
  const H225_CallProceeding_UUIE & call = pdu.m_h323_uu_pdu.m_h323_message_body;

  SetRemoteVersions(call.m_protocolIdentifier);
  SetRemotePartyInfo(pdu);
  SetRemoteApplication(call.m_destinationInfo);

  // Check for fastStart data and start fast
  if (call.HasOptionalField(H225_CallProceeding_UUIE::e_fastStart))
    HandleFastStartAcknowledge(call.m_fastStart);

  // Check that it has the H.245 channel connection info
  if (call.HasOptionalField(H225_CallProceeding_UUIE::e_h245Address))
    CreateOutgoingControlChannel(call.m_h245Address);

  return TRUE;
}


BOOL H323Connection::OnReceivedProgress(const H323SignalPDU & pdu)
{
  if (pdu.m_h323_uu_pdu.m_h323_message_body.GetTag() != H225_H323_UU_PDU_h323_message_body::e_progress)
    return FALSE;
  const H225_Progress_UUIE & progress = pdu.m_h323_uu_pdu.m_h323_message_body;

  SetRemoteVersions(progress.m_protocolIdentifier);
  SetRemotePartyInfo(pdu);
  SetRemoteApplication(progress.m_destinationInfo);

  // Check for fastStart data and start fast
  if (progress.HasOptionalField(H225_Progress_UUIE::e_fastStart))
    HandleFastStartAcknowledge(progress.m_fastStart);

  // Check that it has the H.245 channel connection info
  if (progress.HasOptionalField(H225_Progress_UUIE::e_h245Address))
    return CreateOutgoingControlChannel(progress.m_h245Address);

  return TRUE;
}


BOOL H323Connection::OnReceivedAlerting(const H323SignalPDU & pdu)
{
  if (pdu.m_h323_uu_pdu.m_h323_message_body.GetTag() != H225_H323_UU_PDU_h323_message_body::e_alerting)
    return FALSE;
  const H225_Alerting_UUIE & alert = pdu.m_h323_uu_pdu.m_h323_message_body;

  SetRemoteVersions(alert.m_protocolIdentifier);
  SetRemotePartyInfo(pdu);
  SetRemoteApplication(alert.m_destinationInfo);

  // Check for fastStart data and start fast
  if (alert.HasOptionalField(H225_Alerting_UUIE::e_fastStart))
    HandleFastStartAcknowledge(alert.m_fastStart);

  // Check that it has the H.245 channel connection info
  if (alert.HasOptionalField(H225_Alerting_UUIE::e_h245Address))
    if (!CreateOutgoingControlChannel(alert.m_h245Address))
      return FALSE;

  alertingTime = PTime();
  return OnAlerting(pdu, remotePartyName);
}


BOOL H323Connection::OnReceivedSignalConnect(const H323SignalPDU & pdu)
{
  if (connectionState == ShuttingDownConnection)
    return FALSE;
  connectionState = HasExecutedSignalConnect;
  phase = ConnectedPhase;

  if (pdu.m_h323_uu_pdu.m_h323_message_body.GetTag() != H225_H323_UU_PDU_h323_message_body::e_connect)
    return FALSE;
  const H225_Connect_UUIE & connect = pdu.m_h323_uu_pdu.m_h323_message_body;

  SetRemoteVersions(connect.m_protocolIdentifier);
  SetRemotePartyInfo(pdu);
  SetRemoteApplication(connect.m_destinationInfo);

  if (!OnOutgoingCall(pdu)) {
    Release(EndedByNoAccept);
    return FALSE;
  }

  // Are we involved in a transfer with a non H.450.2 compatible transferred-to endpoint?
  if (h4502handler->GetState() == H4502Handler::e_ctAwaitSetupResponse &&
      h4502handler->IsctTimerRunning())
  {
    PTRACE(4, "H4502\tRemote Endpoint does not support H.450.2.");
    h4502handler->OnReceivedSetupReturnResult();
  }

  // have answer, so set timeout to interval for monitoring calls health
  signallingChannel->SetReadTimeout(MonitorCallStatusTime);

  // Check for fastStart data and start fast
  if (connect.HasOptionalField(H225_Connect_UUIE::e_fastStart))
    HandleFastStartAcknowledge(connect.m_fastStart);

  // Check that it has the H.245 channel connection info
  if (connect.HasOptionalField(H225_Connect_UUIE::e_h245Address)) {
    if (!CreateOutgoingControlChannel(connect.m_h245Address)) {
      if (fastStartState != FastStartAcknowledged)
        return FALSE;
    }
  }

  // If didn't get fast start channels accepted by remote then clear our
  // proposed channels
  if (fastStartState != FastStartAcknowledged) {
    fastStartState = FastStartDisabled;
    fastStartChannels.RemoveAll();
  }
  else if (mediaWaitForConnect) {
    // Otherwise start fast started channels if we were waiting for CONNECT
    for (PINDEX i = 0; i < fastStartChannels.GetSize(); i++)
      fastStartChannels[i].Start();
  }

  connectedTime = PTime();
  OnConnected();
  InternalEstablishedConnectionCheck();

  // If we have a H.245 channel available, bring it up. We either have media
  // and this is just so user indications work, or we don't have media and
  // desperately need it!
  if (h245Tunneling || controlChannel != NULL)
    return StartControlNegotiations();

  // We have no tunnelling and not separate channel, but we really want one
  // so we will start one using a facility message
  PTRACE(2, "H225\tNo H245 address provided by remote, starting control channel");

  H323SignalPDU want245PDU;
  H225_Facility_UUIE * fac = want245PDU.BuildFacility(*this, FALSE);
  fac->m_reason.SetTag(H225_FacilityReason::e_startH245);
  fac->IncludeOptionalField(H225_Facility_UUIE::e_h245Address);

  if (!CreateIncomingControlChannel(fac->m_h245Address))
    return FALSE;

  return WriteSignalPDU(want245PDU);
}


BOOL H323Connection::OnReceivedFacility(const H323SignalPDU & pdu)
{
  if (pdu.m_h323_uu_pdu.m_h323_message_body.GetTag() == H225_H323_UU_PDU_h323_message_body::e_empty)
    return TRUE;

  if (pdu.m_h323_uu_pdu.m_h323_message_body.GetTag() != H225_H323_UU_PDU_h323_message_body::e_facility)
    return FALSE;
  const H225_Facility_UUIE & fac = pdu.m_h323_uu_pdu.m_h323_message_body;

  SetRemoteVersions(fac.m_protocolIdentifier);

  // Check for fastStart data and start fast
  if (fac.HasOptionalField(H225_Facility_UUIE::e_fastStart))
    HandleFastStartAcknowledge(fac.m_fastStart);

  // Check that it has the H.245 channel connection info
  if (fac.HasOptionalField(H225_Facility_UUIE::e_h245Address)) {
    if (controlChannel != NULL) {
      // Fix race condition where both side want to open H.245 channel. we have
      // channel bit it is not open (ie we are listening) and the remote has
      // sent us an address to connect to. To resolve we compare the addresses.

      H323TransportAddress h323Address = controlChannel->GetLocalAddress();
      H225_TransportAddress myAddress;
      h323Address.SetPDU(myAddress);
      PPER_Stream myBuffer;
      myAddress.Encode(myBuffer);

      PPER_Stream otherBuffer;
      fac.m_h245Address.Encode(otherBuffer);

      if (myBuffer < otherBuffer) {
        PTRACE(2, "H225\tSimultaneous start of H.245 channel, connecting to remote.");
        controlChannel->CloseWait();
        delete controlChannel;
        controlChannel = NULL;
      }
      else {
        PTRACE(2, "H225\tSimultaneous start of H.245 channel, using local listener.");
      }
    }

    return CreateOutgoingControlChannel(fac.m_h245Address);
  }

  if (fac.m_reason.GetTag() != H225_FacilityReason::e_callForwarded)
    return TRUE;

  PString address;
  if (fac.HasOptionalField(H225_Facility_UUIE::e_alternativeAliasAddress) &&
      fac.m_alternativeAliasAddress.GetSize() > 0)
    address = H323GetAliasAddressString(fac.m_alternativeAliasAddress[0]);

  if (fac.HasOptionalField(H225_Facility_UUIE::e_alternativeAddress)) {
    if (!address)
      address += '@';
    address += H323TransportAddress(fac.m_alternativeAddress);
  }

  if (endpoint.OnConnectionForwarded(*this, address, pdu)) {
    Release(EndedByCallForwarded);
    return FALSE;
  }

  if (!endpoint.CanAutoCallForward())
    return TRUE;

  if (!endpoint.ForwardConnection(*this, address, pdu))
    return TRUE;

  // This connection is on the way out and a new one has the same token now
  // so change our token to make sure no accidents can happen clearing the
  // wrong call
  callToken += "-forwarded";
  return FALSE;
}


BOOL H323Connection::OnReceivedSignalNotify(const H323SignalPDU & pdu)
{
  if (pdu.m_h323_uu_pdu.m_h323_message_body.GetTag() == H225_H323_UU_PDU_h323_message_body::e_notify) {
    const H225_Notify_UUIE & notify = pdu.m_h323_uu_pdu.m_h323_message_body;
    SetRemoteVersions(notify.m_protocolIdentifier);
  }
  return TRUE;
}


BOOL H323Connection::OnReceivedSignalStatus(const H323SignalPDU & pdu)
{
  if (pdu.m_h323_uu_pdu.m_h323_message_body.GetTag() == H225_H323_UU_PDU_h323_message_body::e_status) {
    const H225_Status_UUIE & status = pdu.m_h323_uu_pdu.m_h323_message_body;
    SetRemoteVersions(status.m_protocolIdentifier);
  }
  return TRUE;
}


BOOL H323Connection::OnReceivedStatusEnquiry(const H323SignalPDU & pdu)
{
  if (pdu.m_h323_uu_pdu.m_h323_message_body.GetTag() == H225_H323_UU_PDU_h323_message_body::e_statusInquiry) {
    const H225_StatusInquiry_UUIE & status = pdu.m_h323_uu_pdu.m_h323_message_body;
    SetRemoteVersions(status.m_protocolIdentifier);
  }

  H323SignalPDU reply;
  reply.BuildStatus(*this);
  return reply.Write(*signallingChannel);
}


void H323Connection::OnReceivedReleaseComplete(const H323SignalPDU & pdu)
{
  if (!callEndTime.IsValid())
    callEndTime = PTime();

  endSessionReceived.Signal();

  if (q931Cause == Q931::ErrorInCauseIE)
    q931Cause = pdu.GetQ931().GetCause();

  switch (connectionState) {
    case EstablishedConnection :
      Release(EndedByRemoteUser);
      break;

    case AwaitingLocalAnswer :
      Release(EndedByCallerAbort);
      break;

    default :
      if (callEndReason == EndedByRefusal)
        callEndReason = NumCallEndReasons;
      
      // Are we involved in a transfer with a non H.450.2 compatible transferred-to endpoint?
      if (h4502handler->GetState() == H4502Handler::e_ctAwaitSetupResponse &&
          h4502handler->IsctTimerRunning())
      {
        PTRACE(4, "H4502\tThe Remote Endpoint has rejected our transfer request and does not support H.450.2.");
        h4502handler->OnReceivedSetupReturnError(H4501_GeneralErrorList::e_notAvailable);
      }

      if (pdu.m_h323_uu_pdu.m_h323_message_body.GetTag() != H225_H323_UU_PDU_h323_message_body::e_releaseComplete)
        Release(EndedByRefusal);
      else {
        const H225_ReleaseComplete_UUIE & rc = pdu.m_h323_uu_pdu.m_h323_message_body;
        SetRemoteVersions(rc.m_protocolIdentifier);
        Release(H323TranslateToCallEndReason(pdu.GetQ931().GetCause(), rc.m_reason));
      }
  }
}


BOOL H323Connection::OnIncomingCall(const H323SignalPDU & setupPDU,
                                    H323SignalPDU & alertingPDU)
{
  return endpoint.OnIncomingCall(*this, setupPDU, alertingPDU);
}


BOOL H323Connection::ForwardCall(const PString & forwardParty)
{
  if (forwardParty.IsEmpty())
    return FALSE;

  PString alias;
  H323TransportAddress address;
  endpoint.ParsePartyName(forwardParty, alias, address);

  H323SignalPDU redirectPDU;
  H225_Facility_UUIE * fac = redirectPDU.BuildFacility(*this, FALSE);

  fac->m_reason.SetTag(H225_FacilityReason::e_callForwarded);

  if (!address) {
    fac->IncludeOptionalField(H225_Facility_UUIE::e_alternativeAddress);
    address.SetPDU(fac->m_alternativeAddress);
  }

  if (!alias) {
    fac->IncludeOptionalField(H225_Facility_UUIE::e_alternativeAliasAddress);
    fac->m_alternativeAliasAddress.SetSize(1);
    H323SetAliasAddress(alias, fac->m_alternativeAliasAddress[0]);
  }

  if (WriteSignalPDU(redirectPDU))
    Release(EndedByCallForwarded);

  return TRUE;
}


H323Connection::AnswerCallResponse
     H323Connection::OnAnswerCall(const PString & caller,
                                  const H323SignalPDU & setupPDU,
                                  H323SignalPDU & connectPDU)
{
  return endpoint.OnAnswerCall(*this, caller, setupPDU, connectPDU);
}

H323Connection::AnswerCallResponse
     H323Connection::OnAnswerCall(const PString & caller)
{
  return OpalConnection::OnAnswerCall(caller);
}

void H323Connection::AnsweringCall(AnswerCallResponse response)
{
  PTRACE(2, "H323\tAnswering call: " << response);

  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked() || phase >= ReleasingPhase)
    return;

  switch (response) {
    default : // AnswerCallDeferred
      break;

    case AnswerCallDeferredWithMedia :
      if (!mediaWaitForConnect) {
        // create a new facility PDU if doing AnswerDeferredWithMedia
        H323SignalPDU want245PDU;
        H225_Progress_UUIE & prog = want245PDU.BuildProgress(*this);

        BOOL sendPDU = TRUE;

        if (SendFastStartAcknowledge(prog.m_fastStart))
          prog.IncludeOptionalField(H225_Progress_UUIE::e_fastStart);
        else {
          // See if aborted call
          if (connectionState == ShuttingDownConnection)
            break;

          // Do early H.245 start
          H225_Facility_UUIE & fac = *want245PDU.BuildFacility(*this, FALSE);
          fac.m_reason.SetTag(H225_FacilityReason::e_startH245);
          earlyStart = TRUE;
          if (!h245Tunneling && (controlChannel == NULL)) {
            if (!CreateIncomingControlChannel(fac.m_h245Address))
              break;

            fac.IncludeOptionalField(H225_Facility_UUIE::e_h245Address);
          } 
          else
            sendPDU = FALSE;
        }

        if (sendPDU) {
          HandleTunnelPDU(&want245PDU);
          WriteSignalPDU(want245PDU);
        }
      }
      break;

    case AnswerCallAlertWithMedia :
      SetAlerting(localPartyName, TRUE);
      break;

    case AnswerCallPending :
      SetAlerting(localPartyName, FALSE);
      break;

    case AnswerCallDenied :
      // If response is denied, abort the call
      PTRACE(1, "H225\tApplication has declined to answer incoming call");
      Release(EndedByAnswerDenied);
      break;

    case AnswerCallNow :
      SetConnected();
  }

  InternalEstablishedConnectionCheck();
}


BOOL H323Connection::SetUpConnection()
{
  signallingChannel->AttachThread(PThread::Create(PCREATE_NOTIFIER(StartOutgoing), 0,
                                  PThread::NoAutoDeleteThread,
                                  PThread::NormalPriority,
                                  "H225 Caller:%x"));
  return TRUE;
}


void H323Connection::StartOutgoing(PThread &, INT)
{
  PTRACE(3, "H225\tStarted call thread");

  if (!SafeReference())
    return;

  PString alias, address;
  PINDEX at = remotePartyAddress.Find('@');
  if (at == P_MAX_INDEX)
    address = remotePartyAddress;
  else {
    alias = remotePartyAddress.Left(at);
    address = remotePartyAddress.Mid(at+1);
  }

  H323TransportAddress h323addr(address, endpoint.GetDefaultSignalPort());
  CallEndReason reason = SendSignalSetup(alias, h323addr);

  // Check if had an error, clear call if so
  if (reason != NumCallEndReasons)
    Release(reason);
  else
    HandleSignallingChannel();

  SafeDereference();
}


OpalConnection::CallEndReason H323Connection::SendSignalSetup(const PString & alias,
                                                              const H323TransportAddress & address)
{
  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked())
    return EndedByCallerAbort;

  // Start the call, first state is asking gatekeeper
  connectionState = AwaitingGatekeeperAdmission;
  phase = SetUpPhase;

  // Start building the setup PDU to get various ID's
  H323SignalPDU setupPDU;
  H225_Setup_UUIE & setup = setupPDU.BuildSetup(*this, address);

  h450dispatcher->AttachToSetup(setupPDU);

  // Save the identifiers generated by BuildSetup
  callReference = setupPDU.GetQ931().GetCallReference();
  conferenceIdentifier = setup.m_conferenceID;
  setupPDU.GetQ931().GetCalledPartyNumber(remotePartyNumber);

  H323TransportAddress gatekeeperRoute = address;

  // Check for gatekeeper and do admission check if have one
  H323Gatekeeper * gatekeeper = endpoint.GetGatekeeper();
  H225_ArrayOf_AliasAddress newAliasAddresses;
  if (gatekeeper != NULL) {
    H323Gatekeeper::AdmissionResponse response;
    response.transportAddress = &gatekeeperRoute;
    response.aliasAddresses = &newAliasAddresses;
    if (!gkAccessTokenOID)
      response.accessTokenData = &gkAccessTokenData;
    while (!gatekeeper->AdmissionRequest(*this, response, alias.IsEmpty())) {
      PTRACE(1, "H225\tGatekeeper refused admission: "
             << (response.rejectReason == UINT_MAX
                  ? PString("Transport error")
                  : H225_AdmissionRejectReason(response.rejectReason).GetTagName()));
      h4502handler->onReceivedAdmissionReject(H4501_GeneralErrorList::e_notAvailable);

      switch (response.rejectReason) {
        case H225_AdmissionRejectReason::e_calledPartyNotRegistered :
          return EndedByNoUser;
        case H225_AdmissionRejectReason::e_requestDenied :
          return EndedByNoBandwidth;
        case H225_AdmissionRejectReason::e_invalidPermission :
        case H225_AdmissionRejectReason::e_securityDenial :
          return EndedBySecurityDenial;
        case H225_AdmissionRejectReason::e_resourceUnavailable :
          return EndedByRemoteBusy;
        case H225_AdmissionRejectReason::e_incompleteAddress :
          if (OnInsufficientDigits())
            break;
          // Then default case
        default :
          return EndedByGatekeeper;
      }

      PString lastRemotePartyName = remotePartyName;
      while (lastRemotePartyName == remotePartyName) {
        UnlockReadWrite(); // Release the mutex as can deadlock trying to clear call during connect.
        digitsWaitFlag.Wait();
        if (!LockReadWrite()) // Lock while checking for shutting down.
          return EndedByCallerAbort;
        if (phase >= ReleasingPhase)
          return EndedByCallerAbort;
      }
    }
    mustSendDRQ = TRUE;
    if (response.gatekeeperRouted) {
      setup.IncludeOptionalField(H225_Setup_UUIE::e_endpointIdentifier);
      setup.m_endpointIdentifier = gatekeeper->GetEndpointIdentifier();
      gatekeeperRouted = TRUE;
    }
  }

  // Update the field e_destinationAddress in the SETUP PDU to reflect the new 
  // alias received in the ACF (m_destinationInfo).
  if (newAliasAddresses.GetSize() > 0) {
    setup.IncludeOptionalField(H225_Setup_UUIE::e_destinationAddress);
    setup.m_destinationAddress = newAliasAddresses;

    // Update the Q.931 Information Element (if is an E.164 address)
    PString e164 = H323GetAliasAddressE164(newAliasAddresses);
    if (!e164)
      remotePartyNumber = e164;
  }

  if (addAccessTokenToSetup && !gkAccessTokenOID && !gkAccessTokenData.IsEmpty()) {
    PString oid1, oid2;
    PINDEX comma = gkAccessTokenOID.Find(',');
    if (comma == P_MAX_INDEX)
      oid1 = oid2 = gkAccessTokenOID;
    else {
      oid1 = gkAccessTokenOID.Left(comma);
      oid2 = gkAccessTokenOID.Mid(comma+1);
    }
    setup.IncludeOptionalField(H225_Setup_UUIE::e_tokens);
    PINDEX last = setup.m_tokens.GetSize();
    setup.m_tokens.SetSize(last+1);
    setup.m_tokens[last].m_tokenOID = oid1;
    setup.m_tokens[last].IncludeOptionalField(H235_ClearToken::e_nonStandard);
    setup.m_tokens[last].m_nonStandard.m_nonStandardIdentifier = oid2;
    setup.m_tokens[last].m_nonStandard.m_data = gkAccessTokenData;
  }

  if (!signallingChannel->SetRemoteAddress(gatekeeperRoute)) {
    PTRACE(1, "H225\tInvalid "
           << (gatekeeperRoute != address ? "gatekeeper" : "user")
           << " supplied address: \"" << gatekeeperRoute << '"');
    connectionState = AwaitingTransportConnect;
    return EndedByConnectFail;
  }

  // Do the transport connect
  connectionState = AwaitingTransportConnect;
  phase = SetUpPhase;

  // Release the mutex as can deadlock trying to clear call during connect.
  safeLock.Unlock();

  BOOL connectFailed = !signallingChannel->Connect();

    // Lock while checking for shutting down.
  if (!safeLock.Lock())
    return EndedByCallerAbort;

  if (phase >= ReleasingPhase)
    return EndedByCallerAbort;

  // See if transport connect failed, abort if so.
  if (connectFailed) {
    connectionState = NoConnectionActive;
    phase = UninitialisedPhase;
    switch (signallingChannel->GetErrorNumber()) {
      case ENETUNREACH :
        return EndedByUnreachable;
      case ECONNREFUSED :
        return EndedByNoEndPoint;
      case ETIMEDOUT :
        return EndedByHostOffline;
    }
    return EndedByConnectFail;
  }

  PTRACE(3, "H225\tSending Setup PDU");
  connectionState = AwaitingSignalConnect;

  // Put in all the signalling addresses for link
  H323TransportAddress transportAddress = signallingChannel->GetLocalAddress();
  setup.IncludeOptionalField(H225_Setup_UUIE::e_sourceCallSignalAddress);
  transportAddress.SetPDU(setup.m_sourceCallSignalAddress);
  if (!setup.HasOptionalField(H225_Setup_UUIE::e_destCallSignalAddress)) {
    transportAddress = signallingChannel->GetRemoteAddress();
    setup.IncludeOptionalField(H225_Setup_UUIE::e_destCallSignalAddress);
    transportAddress.SetPDU(setup.m_destCallSignalAddress);
  }

  // Get the local capabilities before fast start is handled
  OnSetLocalCapabilities();

  // Ask the application what channels to open
  PTRACE(3, "H225\tCheck for Fast start by local endpoint");
  fastStartChannels.RemoveAll();
  OnSelectLogicalChannels();

  // If application called OpenLogicalChannel, put in the fastStart field
  if (!fastStartChannels.IsEmpty()) {
    PTRACE(3, "H225\tFast start begun by local endpoint");
    for (PINDEX i = 0; i < fastStartChannels.GetSize(); i++)
      BuildFastStartList(fastStartChannels[i], setup.m_fastStart, H323Channel::IsReceiver);
    if (setup.m_fastStart.GetSize() > 0)
      setup.IncludeOptionalField(H225_Setup_UUIE::e_fastStart);
  }

  // Search the capability set and see if we have video capability
  BOOL hasVideoOrData = FALSE;
  for (PINDEX i = 0; i < localCapabilities.GetSize(); i++) {
    if (!PIsDescendant(&localCapabilities[i], H323AudioCapability) &&
        !PIsDescendant(&localCapabilities[i], H323_UserInputCapability)) {
      hasVideoOrData = TRUE;
      break;
    }
  }
  if (hasVideoOrData)
    setupPDU.GetQ931().SetBearerCapabilities(Q931::TransferUnrestrictedDigital, 6);

  if (!OnSendSignalSetup(setupPDU))
    return EndedByNoAccept;

  // Do this again (was done when PDU was constructed) in case
  // OnSendSignalSetup() changed something.
  setupPDU.SetQ931Fields(*this, TRUE);
  setupPDU.GetQ931().GetCalledPartyNumber(remotePartyNumber);

  fastStartState = FastStartDisabled;
  BOOL set_lastPDUWasH245inSETUP = FALSE;

  if (h245Tunneling && doH245inSETUP) {
    h245TunnelTxPDU = &setupPDU;

    // Try and start the master/slave and capability exchange through the tunnel
    // Note: this used to be disallowed but is now allowed as of H323v4
    BOOL ok = StartControlNegotiations();

    h245TunnelTxPDU = NULL;

    if (!ok)
      return EndedByTransportFail;

    if (setup.m_fastStart.GetSize() > 0) {
      // Now if fast start as well need to put this in setup specific field
      // and not the generic H.245 tunneling field
      setup.IncludeOptionalField(H225_Setup_UUIE::e_parallelH245Control);
      setup.m_parallelH245Control = setupPDU.m_h323_uu_pdu.m_h245Control;
      setupPDU.m_h323_uu_pdu.RemoveOptionalField(H225_H323_UU_PDU::e_h245Control);
      set_lastPDUWasH245inSETUP = TRUE;
    }
  }

  // Send the initial PDU
  if (!WriteSignalPDU(setupPDU))
    return EndedByTransportFail;

  // WriteSignalPDU always resets lastPDUWasH245inSETUP.
  // So set it here if required
  if (set_lastPDUWasH245inSETUP)
    lastPDUWasH245inSETUP = TRUE;

  // Set timeout for remote party to answer the call
  signallingChannel->SetReadTimeout(endpoint.GetSignallingChannelCallTimeout());

  connectionState = AwaitingSignalConnect;
  phase = AlertingPhase;

  return NumCallEndReasons;
}


BOOL H323Connection::OnSendSignalSetup(H323SignalPDU & /*setupPDU*/)
{
  return TRUE;
}


BOOL H323Connection::OnSendCallProceeding(H323SignalPDU & /*callProceedingPDU*/)
{
  return TRUE;
}


BOOL H323Connection::OnSendReleaseComplete(H323SignalPDU & /*releaseCompletePDU*/)
{
  return TRUE;
}


BOOL H323Connection::OnAlerting(const H323SignalPDU & alertingPDU,
                                const PString & username)
{
  return endpoint.OnAlerting(*this, alertingPDU, username);
}


BOOL H323Connection::SetAlerting(const PString & /*calleeName*/, BOOL withMedia)
{
  PTRACE(3, "H323\tSetAlerting " << *this);
  if (alertingPDU == NULL)
    return FALSE;

  if (withMedia && !mediaWaitForConnect) {
    H225_Alerting_UUIE & alerting = alertingPDU->m_h323_uu_pdu.m_h323_message_body;
    if (SendFastStartAcknowledge(alerting.m_fastStart))
      alerting.IncludeOptionalField(H225_Alerting_UUIE::e_fastStart);
    else {
      // See if aborted call
      if (connectionState == ShuttingDownConnection)
        return FALSE;

      // Do early H.245 start
      earlyStart = TRUE;
      if (!h245Tunneling && (controlChannel == NULL)) {
        if (!CreateIncomingControlChannel(alerting.m_h245Address))
          return FALSE;
        alerting.IncludeOptionalField(H225_Alerting_UUIE::e_h245Address);
      }
    }
  }

  alertingTime = PTime();

  HandleTunnelPDU(alertingPDU);

  h450dispatcher->AttachToAlerting(*alertingPDU);

  // send Q931 Alerting PDU
  PTRACE(3, "H225\tSending Alerting PDU");
  return WriteSignalPDU(*alertingPDU);
}


BOOL H323Connection::SetConnected()
{
  mediaWaitForConnect = FALSE;

  PTRACE(3, "H323\tSetConnected " << *this);
  if (connectPDU == NULL)
    return FALSE;

  // Assure capabilities are set to other connections media list (if not already)
  OnSetLocalCapabilities();

  H225_Connect_UUIE & connect = connectPDU->m_h323_uu_pdu.m_h323_message_body;
  // Now ask the application to select which channels to start
  if (SendFastStartAcknowledge(connect.m_fastStart))
    connect.IncludeOptionalField(H225_Connect_UUIE::e_fastStart);

  // See if aborted call
  if (connectionState == ShuttingDownConnection)
    return FALSE;

  // Set flag that we are up to CONNECT stage
  connectionState = HasExecutedSignalConnect;
  phase = ConnectedPhase;

  h450dispatcher->AttachToConnect(*connectPDU);

  if (h245Tunneling) {
    HandleTunnelPDU(connectPDU);

    // If no channels selected (or never provided) do traditional H245 start
    if (fastStartState == FastStartDisabled) {
      h245TunnelTxPDU = connectPDU; // Piggy back H245 on this reply
      BOOL ok = StartControlNegotiations();
      h245TunnelTxPDU = NULL;
      if (!ok)
        return FALSE;
    }
  }
  else if (!controlChannel) { // Start separate H.245 channel if not tunneling.
    if (!CreateIncomingControlChannel(connect.m_h245Address))
      return FALSE;
    connect.IncludeOptionalField(H225_Connect_UUIE::e_h245Address);
  }

  if (!WriteSignalPDU(*connectPDU)) // Send H323 Connect PDU
    return FALSE;

  delete connectPDU;
  connectPDU = NULL;

  delete alertingPDU;
  alertingPDU = NULL;

  connectedTime = PTime();

  InternalEstablishedConnectionCheck();
  return TRUE;
}


BOOL H323Connection::OnInsufficientDigits()
{
  return FALSE;
}


void H323Connection::SendMoreDigits(const PString & digits)
{
  remotePartyNumber += digits;
  remotePartyName = remotePartyNumber;
  if (connectionState == AwaitingGatekeeperAdmission)
    digitsWaitFlag.Signal();
  else {
    H323SignalPDU infoPDU;
    infoPDU.BuildInformation(*this);
    infoPDU.GetQ931().SetCalledPartyNumber(digits);
    if (!WriteSignalPDU(infoPDU))
      Release(EndedByTransportFail);
  }
}


BOOL H323Connection::OnOutgoingCall(const H323SignalPDU & connectPDU)
{
  return endpoint.OnOutgoingCall(*this, connectPDU);
}


BOOL H323Connection::SendFastStartAcknowledge(H225_ArrayOf_PASN_OctetString & array)
{
  PINDEX i;

  // See if we have already added the fast start OLC's
  if (array.GetSize() > 0)
    return TRUE;

  // See if we need to select our fast start channels
  if (fastStartState == FastStartResponse)
    OnSelectLogicalChannels();

  // Remove any channels that were not started by OnSelectLogicalChannels(),
  // those that were started are put into the logical channel dictionary
  for (i = 0; i < fastStartChannels.GetSize(); i++) {
    if (fastStartChannels[i].IsOpen())
      logicalChannels->Add(fastStartChannels[i]);
    else
      fastStartChannels.RemoveAt(i--);
  }

  // None left, so didn't open any channels fast
  if (fastStartChannels.IsEmpty()) {
    fastStartState = FastStartDisabled;
    return FALSE;
  }

  // The channels we just transferred to the logical channels dictionary
  // should not be deleted via this structure now.
  fastStartChannels.DisallowDeleteObjects();

  PTRACE(3, "H225\tAccepting fastStart for " << fastStartChannels.GetSize() << " channels");

  for (i = 0; i < fastStartChannels.GetSize(); i++)
    BuildFastStartList(fastStartChannels[i], array, H323Channel::IsTransmitter);

  // Have moved open channels to logicalChannels structure, remove all others.
  fastStartChannels.RemoveAll();

  // Set flag so internal establishment check does not require H.245
  fastStartState = FastStartAcknowledged;

  return TRUE;
}


BOOL H323Connection::HandleFastStartAcknowledge(const H225_ArrayOf_PASN_OctetString & array)
{
  if (fastStartChannels.IsEmpty()) {
    PTRACE(3, "H225\tFast start response with no channels to open");
    return FALSE;
  }

  PTRACE(3, "H225\tFast start accepted by remote endpoint");

  PINDEX i;

  // Go through provided list of structures, if can decode it and match it up
  // with a channel we requested AND it has all the information needed in the
  // m_multiplexParameters, then we can start the channel.
  for (i = 0; i < array.GetSize(); i++) {
    H245_OpenLogicalChannel open;
    if (array[i].DecodeSubType(open)) {
      PTRACE(4, "H225\tFast start open:\n  " << setprecision(2) << open);
      BOOL reverse = open.HasOptionalField(H245_OpenLogicalChannel::e_reverseLogicalChannelParameters);
      const H245_DataType & dataType = reverse ? open.m_reverseLogicalChannelParameters.m_dataType
                                               : open.m_forwardLogicalChannelParameters.m_dataType;
      H323Capability * replyCapability = localCapabilities.FindCapability(dataType);
      if (replyCapability != NULL) {
        for (PINDEX ch = 0; ch < fastStartChannels.GetSize(); ch++) {
          H323Channel & channelToStart = fastStartChannels[ch];
          H323Channel::Directions dir = channelToStart.GetDirection();
          if ((dir == H323Channel::IsReceiver) == reverse &&
               channelToStart.GetCapability() == *replyCapability) {
            unsigned error = 1000;
            if (channelToStart.OnReceivedPDU(open, error)) {
              H323Capability * channelCapability;
              if (dir == H323Channel::IsReceiver)
                channelCapability = replyCapability;
              else {
                // For transmitter, need to fake a capability into the remote table
                channelCapability = remoteCapabilities.FindCapability(channelToStart.GetCapability());
                if (channelCapability == NULL) {
                  channelCapability = remoteCapabilities.Copy(channelToStart.GetCapability());
                  remoteCapabilities.SetCapability(0, channelCapability->GetDefaultSessionID()-1, channelCapability);
                }
              }
              // Must use the actual capability instance from the
              // localCapability or remoteCapability structures.
              if (OnCreateLogicalChannel(*channelCapability, dir, error)) {
                if (channelToStart.SetInitialBandwidth()) {
                  if (channelToStart.Open()) {
                    if (channelToStart.GetDirection() == H323Channel::IsTransmitter) {
                      transmitterMediaStream = ((H323UnidirectionalChannel &)channelToStart).GetMediaStream();
                      if (GetCall().OpenSourceMediaStreams(*this, transmitterMediaStream->GetMediaFormat(), channelToStart.GetSessionID())) {
                        if (!mediaWaitForConnect)
                          channelToStart.Start();
                      }
                      else {
                        transmitterMediaStream = NULL;
                        channelToStart.Close();
                      }
                    }
                    else
                      channelToStart.Start();
                    if (channelToStart.IsOpen())
                      break;
                  }
                }
                else
                  PTRACE(2, "H225\tFast start channel open fail: insufficent bandwidth");
              }
              else
                PTRACE(2, "H225\tFast start channel open error: " << error);
            }
            else
              PTRACE(2, "H225\tFast start capability error: " << error);
          }
        }
      }
    }
    else {
      PTRACE(1, "H225\tInvalid fast start PDU decode:\n  " << setprecision(2) << open);
    }
  }

  // Remove any channels that were not started by above, those that were
  // started are put into the logical channel dictionary
  for (i = 0; i < fastStartChannels.GetSize(); i++) {
    if (fastStartChannels[i].IsOpen())
      logicalChannels->Add(fastStartChannels[i]);
    else
      fastStartChannels.RemoveAt(i--);
  }

  // The channels we just transferred to the logical channels dictionary
  // should not be deleted via this structure now.
  fastStartChannels.DisallowDeleteObjects();

  PTRACE(2, "H225\tFast starting " << fastStartChannels.GetSize() << " channels");
  if (fastStartChannels.IsEmpty())
    return FALSE;

  // Have moved open channels to logicalChannels structure, remove them now.
  fastStartChannels.RemoveAll();

  fastStartState = FastStartAcknowledged;

  return TRUE;
}


BOOL H323Connection::OnUnknownSignalPDU(const H323SignalPDU & PTRACE_pdu)
{
  PTRACE(2, "H225\tUnknown signalling PDU: " << PTRACE_pdu);
  return TRUE;
}


BOOL H323Connection::CreateOutgoingControlChannel(const H225_TransportAddress & h245Address)
{
  // Already have the H245 channel up.
  if (controlChannel != NULL)
    return TRUE;

  // Check that it is an IP address, all we support at the moment
  controlChannel = signallingChannel->GetLocalAddress().CreateTransport(
                                  endpoint, OpalTransportAddress::HostOnly);
  if (controlChannel == NULL) {
    PTRACE(1, "H225\tConnect of H245 failed: Unsupported transport");
    return FALSE;
  }

  if (!controlChannel->SetRemoteAddress(H323TransportAddress(h245Address))) {
    PTRACE(1, "H225\tCould not extract H245 address");
    delete controlChannel;
    controlChannel = NULL;
    return FALSE;
  }

  if (!controlChannel->Connect()) {
    PTRACE(1, "H225\tConnect of H245 failed: " << controlChannel->GetErrorText());
    delete controlChannel;
    controlChannel = NULL;
    return FALSE;
  }

  controlChannel->AttachThread(PThread::Create(PCREATE_NOTIFIER(NewOutgoingControlChannel), 0,
                                               PThread::NoAutoDeleteThread,
                                               PThread::NormalPriority,
                                               "H.245 Handler"));
  return TRUE;
}


void H323Connection::NewOutgoingControlChannel(PThread &, INT)
{
  if (PAssertNULL(controlChannel) == NULL)
    return;

  if (!SafeReference())
    return;

  HandleControlChannel();
  SafeDereference();
}


BOOL H323Connection::CreateIncomingControlChannel(H225_TransportAddress & h245Address)
{
  PAssert(controlChannel == NULL, PLogicError);

  H323TransportAddress localSignallingInterface = signallingChannel->GetLocalAddress();
  if (controlListener == NULL) {
    controlListener = localSignallingInterface.CreateListener(
                            endpoint, OpalTransportAddress::HostOnly);
    if (controlListener == NULL)
      return FALSE;

    if (!controlListener->Open(PCREATE_NOTIFIER(NewIncomingControlChannel), TRUE)) {
      delete controlListener;
      controlListener = NULL;
      return FALSE;
    }
  }

  H323TransportAddress listeningAddress = controlListener->GetLocalAddress(localSignallingInterface);
  return listeningAddress.SetPDU(h245Address);
}


void H323Connection::NewIncomingControlChannel(PThread & listener, INT param)
{
  ((OpalListener&)listener).Close();

  if (param == 0) {
    // If H.245 channel failed to connect and have no media (no fast start)
    // then clear the call as it is useless.
    if (mediaStreams.IsEmpty())
      Release(EndedByTransportFail);
    return;
  }

  if (!SafeReference())
    return;

  controlChannel = (H323Transport *)param;
  HandleControlChannel();
  SafeDereference();
}


BOOL H323Connection::WriteControlPDU(const H323ControlPDU & pdu)
{
  PPER_Stream strm;
  pdu.Encode(strm);
  strm.CompleteEncoding();

  H323TraceDumpPDU("H245", TRUE, strm, pdu, pdu, 0);

  if (!h245Tunneling) {
    if (controlChannel == NULL) {
      PTRACE(1, "H245\tWrite PDU fail: no control channel.");
      return FALSE;
    }

    if (controlChannel->IsOpen() && controlChannel->WritePDU(strm))
      return TRUE;

    PTRACE(1, "H245\tWrite PDU fail: " << controlChannel->GetErrorText(PChannel::LastWriteError));
    return FALSE;
  }

  // If have a pending signalling PDU, use it rather than separate write
  H323SignalPDU localTunnelPDU;
  H323SignalPDU * tunnelPDU;
  if (h245TunnelTxPDU != NULL)
    tunnelPDU = h245TunnelTxPDU;
  else {
    localTunnelPDU.BuildFacility(*this, TRUE);
    tunnelPDU = &localTunnelPDU;
  }

  tunnelPDU->m_h323_uu_pdu.IncludeOptionalField(H225_H323_UU_PDU::e_h245Control);
  PINDEX last = tunnelPDU->m_h323_uu_pdu.m_h245Control.GetSize();
  tunnelPDU->m_h323_uu_pdu.m_h245Control.SetSize(last+1);
  tunnelPDU->m_h323_uu_pdu.m_h245Control[last] = strm;

  if (h245TunnelTxPDU != NULL)
    return TRUE;

  return WriteSignalPDU(localTunnelPDU);
}


BOOL H323Connection::StartControlNegotiations()
{
  PTRACE(2, "H245\tStarted control channel");

  // Get the local capabilities before fast start is handled
  OnSetLocalCapabilities();

  // Begin the capability exchange procedure
  if (!capabilityExchangeProcedure->Start(FALSE)) {
    PTRACE(1, "H245\tStart of Capability Exchange failed");
    return FALSE;
  }

  // Begin the Master/Slave determination procedure
  if (!masterSlaveDeterminationProcedure->Start(FALSE)) {
    PTRACE(1, "H245\tStart of Master/Slave determination failed");
    return FALSE;
  }

  endSessionNeeded = TRUE;
  return TRUE;
}


void H323Connection::HandleControlChannel()
{
  // If have started separate H.245 channel then don't tunnel any more
  h245Tunneling = FALSE;

  if (LockReadWrite()) {
    // Start the TCS and MSD operations on new H.245 channel.
    if (!StartControlNegotiations()) {
      UnlockReadWrite();
      return;
    }
    UnlockReadWrite();
  }

  
  // Disable the signalling channels timeout for monitoring call status and
  // start up one in this thread instead. Then the Q.931 channel can be closed
  // without affecting the call.
  signallingChannel->SetReadTimeout(PMaxTimeInterval);
  controlChannel->SetReadTimeout(MonitorCallStatusTime);

  BOOL ok = TRUE;
  while (ok) {
    MonitorCallStatus();

    PPER_Stream strm;
    if (controlChannel->ReadPDU(strm)) {
      // Lock while checking for shutting down.
      ok = LockReadWrite();
      if (ok) {
        // Process the received PDU
        PTRACE(4, "H245\tReceived TPKT: " << strm);
        if (phase < ReleasingPhase)
          ok = HandleControlData(strm);
        else
          ok = InternalEndSessionCheck(strm);
        UnlockReadWrite(); // Unlock connection
      }
    }
    else if (controlChannel->GetErrorCode() != PChannel::Timeout) {
      PTRACE(1, "H245\tRead error: " << controlChannel->GetErrorText(PChannel::LastReadError));
      Release(EndedByTransportFail);
      ok = FALSE;
    }
  }

  // If we are the only link to the far end then indicate that we have
  // received endSession even if we hadn't, because we are now never going
  // to get one so there is no point in having CleanUpOnCallEnd wait.
  if (signallingChannel == NULL)
    endSessionReceived.Signal();

  PTRACE(2, "H245\tControl channel closed.");
}


BOOL H323Connection::InternalEndSessionCheck(PPER_Stream & strm)
{
  H323ControlPDU pdu;

  if (!pdu.Decode(strm)) {
    PTRACE(1, "H245\tInvalid PDU decode:\n  " << setprecision(2) << pdu);
    return FALSE;
  }

  PTRACE(3, "H245\tChecking for end session on PDU: " << pdu.GetTagName()
         << ' ' << ((PASN_Choice &)pdu.GetObject()).GetTagName());

  if (pdu.GetTag() != H245_MultimediaSystemControlMessage::e_command)
    return TRUE;

  H245_CommandMessage & command = pdu;
  if (command.GetTag() == H245_CommandMessage::e_endSessionCommand)
    endSessionReceived.Signal();
  return FALSE;
}


BOOL H323Connection::HandleControlData(PPER_Stream & strm)
{
  while (!strm.IsAtEnd()) {
    H323ControlPDU pdu;
    if (!pdu.Decode(strm)) {
      PTRACE(1, "H245\tInvalid PDU decode!"
                "\nRaw PDU:\n" << hex << setfill('0')
                               << setprecision(2) << strm
                               << dec << setfill(' ') <<
                "\nPartial PDU:\n  " << setprecision(2) << pdu);
      return TRUE;
    }

    H323TraceDumpPDU("H245", FALSE, strm, pdu, pdu, 0);

    if (!HandleControlPDU(pdu))
      return FALSE;

    InternalEstablishedConnectionCheck();

    strm.ByteAlign();
  }

  return TRUE;
}


BOOL H323Connection::HandleControlPDU(const H323ControlPDU & pdu)
{
  switch (pdu.GetTag()) {
    case H245_MultimediaSystemControlMessage::e_request :
      return OnH245Request(pdu);

    case H245_MultimediaSystemControlMessage::e_response :
      return OnH245Response(pdu);

    case H245_MultimediaSystemControlMessage::e_command :
      return OnH245Command(pdu);

    case H245_MultimediaSystemControlMessage::e_indication :
      return OnH245Indication(pdu);
  }

  return OnUnknownControlPDU(pdu);
}


BOOL H323Connection::OnUnknownControlPDU(const H323ControlPDU & pdu)
{
  PTRACE(2, "H245\tUnknown Control PDU: " << pdu);

  H323ControlPDU reply;
  reply.BuildFunctionNotUnderstood(pdu);
  return WriteControlPDU(reply);
}


BOOL H323Connection::OnH245Request(const H323ControlPDU & pdu)
{
  const H245_RequestMessage & request = pdu;

  switch (request.GetTag()) {
    case H245_RequestMessage::e_masterSlaveDetermination :
      return masterSlaveDeterminationProcedure->HandleIncoming(request);

    case H245_RequestMessage::e_terminalCapabilitySet :
    {
      const H245_TerminalCapabilitySet & tcs = request;
      if (tcs.m_protocolIdentifier.GetSize() >= 6) {
        h245version = tcs.m_protocolIdentifier[5];
        h245versionSet = TRUE;
        PTRACE(3, "H245\tSet protocol version to " << h245version);
      }
      return capabilityExchangeProcedure->HandleIncoming(tcs);
    }

    case H245_RequestMessage::e_openLogicalChannel :
      return logicalChannels->HandleOpen(request);

    case H245_RequestMessage::e_closeLogicalChannel :
      return logicalChannels->HandleClose(request);

    case H245_RequestMessage::e_requestChannelClose :
      return logicalChannels->HandleRequestClose(request);

    case H245_RequestMessage::e_requestMode :
      return requestModeProcedure->HandleRequest(request);

    case H245_RequestMessage::e_roundTripDelayRequest :
      return roundTripDelayProcedure->HandleRequest(request);
  }

  return OnUnknownControlPDU(pdu);
}


BOOL H323Connection::OnH245Response(const H323ControlPDU & pdu)
{
  const H245_ResponseMessage & response = pdu;

  switch (response.GetTag()) {
    case H245_ResponseMessage::e_masterSlaveDeterminationAck :
      return masterSlaveDeterminationProcedure->HandleAck(response);

    case H245_ResponseMessage::e_masterSlaveDeterminationReject :
      return masterSlaveDeterminationProcedure->HandleReject(response);

    case H245_ResponseMessage::e_terminalCapabilitySetAck :
      return capabilityExchangeProcedure->HandleAck(response);

    case H245_ResponseMessage::e_terminalCapabilitySetReject :
      return capabilityExchangeProcedure->HandleReject(response);

    case H245_ResponseMessage::e_openLogicalChannelAck :
      return logicalChannels->HandleOpenAck(response);

    case H245_ResponseMessage::e_openLogicalChannelReject :
      return logicalChannels->HandleReject(response);

    case H245_ResponseMessage::e_closeLogicalChannelAck :
      return logicalChannels->HandleCloseAck(response);

    case H245_ResponseMessage::e_requestChannelCloseAck :
      return logicalChannels->HandleRequestCloseAck(response);

    case H245_ResponseMessage::e_requestChannelCloseReject :
      return logicalChannels->HandleRequestCloseReject(response);

    case H245_ResponseMessage::e_requestModeAck :
      return requestModeProcedure->HandleAck(response);

    case H245_ResponseMessage::e_requestModeReject :
      return requestModeProcedure->HandleReject(response);

    case H245_ResponseMessage::e_roundTripDelayResponse :
      return roundTripDelayProcedure->HandleResponse(response);
  }

  return OnUnknownControlPDU(pdu);
}


BOOL H323Connection::OnH245Command(const H323ControlPDU & pdu)
{
  const H245_CommandMessage & command = pdu;

  switch (command.GetTag()) {
    case H245_CommandMessage::e_sendTerminalCapabilitySet :
      return OnH245_SendTerminalCapabilitySet(command);

    case H245_CommandMessage::e_flowControlCommand :
      return OnH245_FlowControlCommand(command);

    case H245_CommandMessage::e_miscellaneousCommand :
      return OnH245_MiscellaneousCommand(command);

    case H245_CommandMessage::e_endSessionCommand :
      endSessionNeeded = TRUE;
      endSessionReceived.Signal();
      switch (connectionState) {
        case EstablishedConnection :
          Release(EndedByRemoteUser);
          break;
        case AwaitingLocalAnswer :
          Release(EndedByCallerAbort);
          break;
        default :
          Release(EndedByRefusal);
      }
      return FALSE;
  }

  return OnUnknownControlPDU(pdu);
}


BOOL H323Connection::OnH245Indication(const H323ControlPDU & pdu)
{
  const H245_IndicationMessage & indication = pdu;

  switch (indication.GetTag()) {
    case H245_IndicationMessage::e_masterSlaveDeterminationRelease :
      return masterSlaveDeterminationProcedure->HandleRelease(indication);

    case H245_IndicationMessage::e_terminalCapabilitySetRelease :
      return capabilityExchangeProcedure->HandleRelease(indication);

    case H245_IndicationMessage::e_openLogicalChannelConfirm :
      return logicalChannels->HandleOpenConfirm(indication);

    case H245_IndicationMessage::e_requestChannelCloseRelease :
      return logicalChannels->HandleRequestCloseRelease(indication);

    case H245_IndicationMessage::e_requestModeRelease :
      return requestModeProcedure->HandleRelease(indication);

    case H245_IndicationMessage::e_miscellaneousIndication :
      return OnH245_MiscellaneousIndication(indication);

    case H245_IndicationMessage::e_jitterIndication :
      return OnH245_JitterIndication(indication);

    case H245_IndicationMessage::e_userInput :
      OnUserInputIndication(indication);
      break;
  }

  return TRUE; // Do NOT call OnUnknownControlPDU for indications
}


BOOL H323Connection::OnH245_SendTerminalCapabilitySet(
                 const H245_SendTerminalCapabilitySet & pdu)
{
  if (pdu.GetTag() == H245_SendTerminalCapabilitySet::e_genericRequest)
    return capabilityExchangeProcedure->Start(TRUE);

  PTRACE(2, "H245\tUnhandled SendTerminalCapabilitySet: " << pdu);
  return TRUE;
}


BOOL H323Connection::OnH245_FlowControlCommand(
                 const H245_FlowControlCommand & pdu)
{
  PTRACE(3, "H245\tFlowControlCommand: scope=" << pdu.m_scope.GetTagName());

  long restriction;
  if (pdu.m_restriction.GetTag() == H245_FlowControlCommand_restriction::e_maximumBitRate)
    restriction = (const PASN_Integer &)pdu.m_restriction;
  else
    restriction = -1; // H245_FlowControlCommand_restriction::e_noRestriction

  switch (pdu.m_scope.GetTag()) {
    case H245_FlowControlCommand_scope::e_wholeMultiplex :
      OnLogicalChannelFlowControl(NULL, restriction);
      break;

    case H245_FlowControlCommand_scope::e_logicalChannelNumber :
    {
      H323Channel * chan = logicalChannels->FindChannel((unsigned)(const H245_LogicalChannelNumber &)pdu.m_scope, FALSE);
      if (chan != NULL)
        OnLogicalChannelFlowControl(chan, restriction);
    }
  }

  return TRUE;
}


BOOL H323Connection::OnH245_MiscellaneousCommand(
                 const H245_MiscellaneousCommand & pdu)
{
  H323Channel * chan = logicalChannels->FindChannel((unsigned)pdu.m_logicalChannelNumber, FALSE);
  if (chan != NULL)
    chan->OnMiscellaneousCommand(pdu.m_type);
  else
    PTRACE(3, "H245\tMiscellaneousCommand: is ignored chan=" << pdu.m_logicalChannelNumber
           << ", type=" << pdu.m_type.GetTagName());

  return TRUE;
}


BOOL H323Connection::OnH245_MiscellaneousIndication(
                 const H245_MiscellaneousIndication & pdu)
{
  H323Channel * chan = logicalChannels->FindChannel((unsigned)pdu.m_logicalChannelNumber, TRUE);
  if (chan != NULL)
    chan->OnMiscellaneousIndication(pdu.m_type);
  else
    PTRACE(3, "H245\tMiscellaneousIndication is ignored. chan=" << pdu.m_logicalChannelNumber
           << ", type=" << pdu.m_type.GetTagName());

  return TRUE;
}


BOOL H323Connection::OnH245_JitterIndication(
                 const H245_JitterIndication & pdu)
{
  PTRACE(3, "H245\tJitterIndication: scope=" << pdu.m_scope.GetTagName());

  static const DWORD mantissas[8] = { 0, 1, 10, 100, 1000, 10000, 100000, 1000000 };
  static const DWORD exponents[8] = { 10, 25, 50, 75 };
  DWORD jitter = mantissas[pdu.m_estimatedReceivedJitterMantissa]*
                 exponents[pdu.m_estimatedReceivedJitterExponent]/10;

  int skippedFrameCount = -1;
  if (pdu.HasOptionalField(H245_JitterIndication::e_skippedFrameCount))
    skippedFrameCount = pdu.m_skippedFrameCount;

  int additionalBuffer = -1;
  if (pdu.HasOptionalField(H245_JitterIndication::e_additionalDecoderBuffer))
    additionalBuffer = pdu.m_additionalDecoderBuffer;

  switch (pdu.m_scope.GetTag()) {
    case H245_JitterIndication_scope::e_wholeMultiplex :
      OnLogicalChannelJitter(NULL, jitter, skippedFrameCount, additionalBuffer);
      break;

    case H245_JitterIndication_scope::e_logicalChannelNumber :
    {
      H323Channel * chan = logicalChannels->FindChannel((unsigned)(const H245_LogicalChannelNumber &)pdu.m_scope, FALSE);
      if (chan != NULL)
        OnLogicalChannelJitter(chan, jitter, skippedFrameCount, additionalBuffer);
    }
  }

  return TRUE;
}


H323Channel * H323Connection::GetLogicalChannel(unsigned number, BOOL fromRemote) const
{
  return logicalChannels->FindChannel(number, fromRemote);
}


H323Channel * H323Connection::FindChannel(unsigned rtpSessionId, BOOL fromRemote) const
{
  return logicalChannels->FindChannelBySession(rtpSessionId, fromRemote);
}


void H323Connection::TransferConnection(const PString & remoteParty,
					const PString & callIdentity)
{
  TransferCall(remoteParty, callIdentity);
}


void H323Connection::TransferCall(const PString & remoteParty,
                                  const PString & callIdentity)
{
  // According to H.450.4, if prior to consultation the primary call has been put on hold, the 
  // transferring endpoint shall first retrieve the call before Call Transfer is invoked.
  if (!callIdentity.IsEmpty() && IsLocalHold())
    RetrieveCall();
  h4502handler->TransferCall(remoteParty, callIdentity);
}


void H323Connection::ConsultationTransfer(const PString & primaryCallToken)
{
  h4502handler->ConsultationTransfer(primaryCallToken);
}


void H323Connection::HandleConsultationTransfer(const PString & callIdentity,
                                                H323Connection& incoming)
{
  h4502handler->HandleConsultationTransfer(callIdentity, incoming);
}


BOOL H323Connection::IsTransferringCall() const
{
  switch (h4502handler->GetState()) {
    case H4502Handler::e_ctAwaitIdentifyResponse :
    case H4502Handler::e_ctAwaitInitiateResponse :
    case H4502Handler::e_ctAwaitSetupResponse :
      return TRUE;

    default :
      return FALSE;
  }
}


BOOL H323Connection::IsTransferredCall() const
{
   return (h4502handler->GetInvokeId() != 0 &&
           h4502handler->GetState() == H4502Handler::e_ctIdle) ||
           h4502handler->isConsultationTransferSuccess();
}

void H323Connection::HandleTransferCall(const PString & token,
                                        const PString & identity)
{
  if (!token.IsEmpty() || !identity)
    h4502handler->AwaitSetupResponse(token, identity);
}


int H323Connection::GetCallTransferInvokeId()
{
  return h4502handler->GetInvokeId();
}


void H323Connection::HandleCallTransferFailure(const int returnError)
{
  h4502handler->HandleCallTransferFailure(returnError);
}


void H323Connection::SetAssociatedCallToken(const PString& token)
{
  h4502handler->SetAssociatedCallToken(token);
}


void H323Connection::OnConsultationTransferSuccess(H323Connection& /*secondaryCall*/)
{
   h4502handler->SetConsultationTransferSuccess();
}


void H323Connection::HoldConnection()
{
  HoldCall(TRUE);
}


void H323Connection::RetrieveConnection()
{
  RetrieveCall();
}


BOOL H323Connection::IsConnectionOnHold() 
{
  return IsCallOnHold ();
}


void H323Connection::HoldCall(BOOL localHold)
{
  h4504handler->HoldCall(localHold);
  holdMediaChannel = SwapHoldMediaChannels(holdMediaChannel);
}


void H323Connection::RetrieveCall()
{
  // Is the current call on hold?
  if (IsLocalHold()) {
    h4504handler->RetrieveCall();
    holdMediaChannel = SwapHoldMediaChannels(holdMediaChannel);
  }
  else if (IsRemoteHold()) {
    PTRACE(4, "H4504\tRemote-end Call Hold not implemented.");
  }
  else {
    PTRACE(4, "H4504\tCall is not on Hold.");
  }
}


void H323Connection::SetHoldMedia(PChannel * audioChannel)
{
  holdMediaChannel = PAssertNULL(audioChannel);
}


BOOL H323Connection::IsMediaOnHold() const
{
  return holdMediaChannel != NULL;
}


PChannel * H323Connection::SwapHoldMediaChannels(PChannel * newChannel)
{
  if (IsMediaOnHold()) {
    if (PAssertNULL(newChannel) == NULL)
      return NULL;
  }

  PChannel * existingTransmitChannel = NULL;

  PINDEX count = logicalChannels->GetSize();

  for (PINDEX i = 0; i < count; ++i) {
    H323Channel* channel = logicalChannels->GetChannelAt(i);
    if (!channel)
      return NULL;

    unsigned int session_id = channel->GetSessionID();
    if (session_id == OpalMediaFormat::DefaultAudioSessionID || session_id == OpalMediaFormat::DefaultVideoSessionID) {
      const H323ChannelNumber & channelNumber = channel->GetNumber();

      H323_RTPChannel * chan2 = reinterpret_cast<H323_RTPChannel*>(channel);

      if (!channelNumber.IsFromRemote()) { // Transmit channel
        if (IsMediaOnHold()) {
//          H323Codec & codec = *channel->GetCodec();
//          existingTransmitChannel = codec.GetRawDataChannel();
        }
        else {
          // Enable/mute the transmit channel depending on whether the remote end is held
          chan2->SetPause(IsLocalHold());
        }
      }
      else {
        // Enable/mute the receive channel depending on whether the remote endis held
        chan2->SetPause(IsLocalHold());
      }
    }
  }

  return existingTransmitChannel;
}


BOOL H323Connection::IsLocalHold() const
{
  return h4504handler->GetState() == H4504Handler::e_ch_NE_Held;
}


BOOL H323Connection::IsRemoteHold() const
{
  return h4504handler->GetState() == H4504Handler::e_ch_RE_Held;
}


BOOL H323Connection::IsCallOnHold() const
{
  return h4504handler->GetState() != H4504Handler::e_ch_Idle;
}


void H323Connection::IntrudeCall(unsigned capabilityLevel)
{
  h45011handler->IntrudeCall(capabilityLevel);
}


void H323Connection::HandleIntrudeCall(const PString & token,
                                       const PString & identity)
{
  if (!token.IsEmpty() || !identity)
    h45011handler->AwaitSetupResponse(token, identity);
}


BOOL H323Connection::GetRemoteCallIntrusionProtectionLevel(const PString & intrusionCallToken,
                                                           unsigned intrusionCICL)
{
  return h45011handler->GetRemoteCallIntrusionProtectionLevel(intrusionCallToken, intrusionCICL);
}


void H323Connection::SetIntrusionImpending()
{
  h45011handler->SetIntrusionImpending();
}


void H323Connection::SetForcedReleaseAccepted()
{
  h45011handler->SetForcedReleaseAccepted();
}


void H323Connection::SetIntrusionNotAuthorized()
{
  h45011handler->SetIntrusionNotAuthorized();
}


void H323Connection::SendCallWaitingIndication(const unsigned nbOfAddWaitingCalls)
{
  h4506handler->AttachToAlerting(*alertingPDU, nbOfAddWaitingCalls);
}


BOOL H323Connection::OnControlProtocolError(ControlProtocolErrors /*errorSource*/,
                                            const void * /*errorData*/)
{
  return TRUE;
}


static void SetRFC2833PayloadType(H323Capabilities & capabilities,
                                  OpalRFC2833Proto & rfc2833handler)
{
  H323Capability * capability = capabilities.FindCapability(H323_UserInputCapability::SubTypeNames[H323_UserInputCapability::SignalToneRFC2833]);
  if (capability != NULL) {
    RTP_DataFrame::PayloadTypes pt = ((H323_UserInputCapability*)capability)->GetPayloadType();
    if (rfc2833handler.GetPayloadType() != pt) {
      PTRACE(2, "H323\tUser Input RFC2833 payload type set to " << pt);
      rfc2833handler.SetPayloadType(pt);
    }
  }
}


void H323Connection::OnSendCapabilitySet(H245_TerminalCapabilitySet & /*pdu*/)
{
  // If we originated call, then check for RFC2833 capability and set payload type
  if (!HadAnsweredCall())
    SetRFC2833PayloadType(localCapabilities, *rfc2833Handler);
}


BOOL H323Connection::OnReceivedCapabilitySet(const H323Capabilities & remoteCaps,
                                             const H245_MultiplexCapability * muxCap,
                                             H245_TerminalCapabilitySetReject & /*rejectPDU*/)
{
  if (muxCap != NULL) {
    if (muxCap->GetTag() != H245_MultiplexCapability::e_h2250Capability) {
      PTRACE(1, "H323\tCapabilitySet contains unsupported multiplex.");
      return FALSE;
    }

    const H245_H2250Capability & h225_0 = *muxCap;
    remoteMaxAudioDelayJitter = h225_0.m_maximumAudioDelayJitter;
  }

  if (remoteCaps.GetSize() == 0) {
    // Received empty TCS, so close all transmit channels
    for (PINDEX i = 0; i < logicalChannels->GetSize(); i++) {
      H245NegLogicalChannel & negChannel = logicalChannels->GetNegLogicalChannelAt(i);
      H323Channel * channel = negChannel.GetChannel();
      if (channel != NULL && !channel->GetNumber().IsFromRemote())
        negChannel.Close();
    }
    transmitterSidePaused = TRUE;
  }
  else {
    /* Received non-empty TCS, if was in paused state or this is the first TCS
       received so we should kill the fake table created by fast start kill
       the remote capabilities table so Merge() becomes a simple assignment */
    if (transmitterSidePaused || !capabilityExchangeProcedure->HasReceivedCapabilities())
      remoteCapabilities.RemoveAll();

    if (!remoteCapabilities.Merge(remoteCaps))
      return FALSE;

    if (transmitterSidePaused) {
      transmitterSidePaused = FALSE;
      connectionState = HasExecutedSignalConnect;
      phase = ConnectedPhase;
      capabilityExchangeProcedure->Start(TRUE);
    }
    else {
      if (localCapabilities.GetSize() > 0)
        capabilityExchangeProcedure->Start(FALSE);

      // If we terminated call, then check for RFC2833 capability and set payload type
      if (HadAnsweredCall())
        SetRFC2833PayloadType(remoteCapabilities, *rfc2833Handler);
    }
  }

  return TRUE;
}


void H323Connection::SendCapabilitySet(BOOL empty)
{
  capabilityExchangeProcedure->Start(TRUE, empty);
}


void H323Connection::OnSetLocalCapabilities()
{
  if (capabilityExchangeProcedure->HasSentCapabilities())
    return;

  OpalMediaFormatList formats = ownerCall.GetMediaFormats(*this, FALSE);
  if (formats.IsEmpty()) {
    PTRACE(2, "H323\tSetLocalCapabilities - no formats from other connection(s) in call");
    return;
  }

  // Remove those things not in the other parties media format list
  for (PINDEX c = 0; c < localCapabilities.GetSize(); c++) {
    H323Capability & capability = localCapabilities[c];
    if (formats.FindFormat(capability.GetMediaFormat()) == P_MAX_INDEX) {
      localCapabilities.Remove(&capability);
      c--;
    }
  }

  // Add those things that are in the other parties media format list
  static unsigned sessionOrder[] = {
    OpalMediaFormat::DefaultAudioSessionID,
    OpalMediaFormat::DefaultVideoSessionID,
    OpalMediaFormat::DefaultDataSessionID,
    0
  };
  PINDEX simultaneous;

  for (PINDEX s = 0; s < PARRAYSIZE(sessionOrder); s++) {
    simultaneous = P_MAX_INDEX;
    for (PINDEX i = 0; i < formats.GetSize(); i++) {
      OpalMediaFormat format = formats[i];
      if (format.GetDefaultSessionID() == sessionOrder[s] &&
          format.GetPayloadType() < RTP_DataFrame::MaxPayloadType)
        simultaneous = localCapabilities.AddAllCapabilities(endpoint, 0, simultaneous, format);
    }
  }

  H323_UserInputCapability::AddAllCapabilities(localCapabilities, 0, P_MAX_INDEX);

  // Special test for the RFC2833 capability to get the correct dynamic payload type
  H323Capability * capability = localCapabilities.FindCapability(OpalRFC2833);
  if (capability != NULL) {
    MediaInformation info;
    OpalConnection * otherParty = GetCall().GetOtherPartyConnection(*this);
    if (otherParty != NULL &&
        otherParty->GetMediaInformation(OpalMediaFormat::DefaultAudioSessionID, info))
      capability->SetPayloadType(info.rfc2833);
    else
      localCapabilities.Remove(capability);
  }

  PTRACE(2, "H323\tSetLocalCapabilities:\n" << setprecision(2) << localCapabilities);
}


BOOL H323Connection::IsH245Master() const
{
  return masterSlaveDeterminationProcedure->IsMaster();
}


void H323Connection::StartRoundTripDelay()
{
  if (LockReadWrite()) {
    if (phase < ReleasingPhase &&
        masterSlaveDeterminationProcedure->IsDetermined() &&
        capabilityExchangeProcedure->HasSentCapabilities()) {
      if (roundTripDelayProcedure->IsRemoteOffline()) {
        PTRACE(2, "H245\tRemote failed to respond to PDU.");
        if (endpoint.ShouldClearCallOnRoundTripFail())
          Release(EndedByTransportFail);
      }
      else
        roundTripDelayProcedure->StartRequest();
    }
    UnlockReadWrite();
  }
}


PTimeInterval H323Connection::GetRoundTripDelay() const
{
  return roundTripDelayProcedure->GetRoundTripDelay();
}


void H323Connection::InternalEstablishedConnectionCheck()
{
  PTRACE(3, "H323\tInternalEstablishedConnectionCheck: "
            "connectionState=" << connectionState << " "
            "fastStartState=" << fastStartState);

  BOOL h245_available = masterSlaveDeterminationProcedure->IsDetermined() &&
                        capabilityExchangeProcedure->HasSentCapabilities() &&
                        capabilityExchangeProcedure->HasReceivedCapabilities();

  if (h245_available)
    endSessionNeeded = TRUE;

  // Check for if all the 245 conditions are met so can start up logical
  // channels and complete the connection establishment.
  if (fastStartState != FastStartAcknowledged) {
    if (!h245_available)
      return;

    // If we are early starting, start channels as soon as possible instead of
    // waiting for connect PDU
    if (earlyStart && IsH245Master() && FindChannel(OpalMediaFormat::DefaultAudioSessionID, FALSE) == NULL)
      OnSelectLogicalChannels();
  }

  if (h245_available && startT120) {
    if (remoteCapabilities.FindCapability("T.120") != NULL) {
      H323Capability * capability = localCapabilities.FindCapability("T.120");
      if (capability != NULL)
        OpenLogicalChannel(*capability, 3, H323Channel::IsBidirectional);
    }
    startT120 = FALSE;
  }

  switch (phase) {
    case ConnectedPhase :
      // Check if we have already got a transmitter running, select one if not
      if (FindChannel(OpalMediaFormat::DefaultAudioSessionID, FALSE) == NULL)
        OnSelectLogicalChannels();

      connectionState = EstablishedConnection;
      phase = EstablishedPhase;
      break;

    case EstablishedPhase :
      connectionState = EstablishedConnection; // Keep in sync
      break;

    default :
      break;
  }
}


OpalMediaFormatList H323Connection::GetMediaFormats() const
{
  OpalMediaFormatList list;
  
  list = remoteCapabilities.GetMediaFormats();
  AdjustMediaFormats(list);

  return list;
}


BOOL H323Connection::OpenSourceMediaStream(const OpalMediaFormatList & /*mediaFormats*/,
                                           unsigned sessionID)
{
  // Check if we have already got a transmitter running, select one if not
  if ((fastStartState == FastStartDisabled ||
       fastStartState == FastStartAcknowledged) &&
      FindChannel(sessionID, FALSE) != NULL)
    return FALSE;

  PTRACE(1, "H323\tOpenSourceMediaStream called: session " << sessionID);
  return TRUE;
}


OpalMediaStream * H323Connection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                    unsigned sessionID,
                                                    BOOL isSource)
{
  if (ownerCall.IsMediaBypassPossible(*this, sessionID))
    return new OpalNullMediaStream(mediaFormat, sessionID, isSource);

  if (!isSource) {
    if (transmitterMediaStream == NULL)
      return NULL;
    OpalMediaStream * stream = transmitterMediaStream;
    transmitterMediaStream = NULL;
    return stream;
  }

  RTP_Session * session = GetSession(sessionID);
  if (session == NULL) {
    PTRACE(1, "H323\tCreateMediaStream could not find session " << sessionID);
    return NULL;
  }

  return new OpalRTPMediaStream(mediaFormat, isSource, *session,
                                endpoint.GetManager().GetMinAudioJitterDelay(),
                                endpoint.GetManager().GetMaxAudioJitterDelay());
}


BOOL H323Connection::IsMediaBypassPossible(unsigned sessionID) const
{
  PTRACE(3, "H323\tIsMediaBypassPossible: session " << sessionID);

  return sessionID == OpalMediaFormat::DefaultAudioSessionID ||
         sessionID == OpalMediaFormat::DefaultVideoSessionID;
}


BOOL H323Connection::GetMediaInformation(unsigned sessionID,
                                         MediaInformation & info) const
{
  if (!OpalConnection::GetMediaInformation(sessionID, info))
    return FALSE;

  H323Capability * capability = remoteCapabilities.FindCapability(OpalRFC2833);
  if (capability != NULL)
    info.rfc2833 = capability->GetPayloadType();

  PTRACE(3, "H323\tGetMediaInformation for session " << sessionID
         << " data=" << info.data << " rfc2833=" << info.rfc2833);
  return TRUE;
}


void H323Connection::StartFastStartChannel(unsigned sessionID, H323Channel::Directions direction)
{
  for (PINDEX i = 0; i < fastStartChannels.GetSize(); i++) {
    H323Channel & channel = fastStartChannels[i];
    if (channel.GetSessionID() == sessionID && channel.GetDirection() == direction) {
      if (channel.Open()) {
        if (direction == H323Channel::IsTransmitter) {
          transmitterMediaStream = ((H323UnidirectionalChannel &)channel).GetMediaStream();
          if (GetCall().OpenSourceMediaStreams(*this, transmitterMediaStream->GetMediaFormat(), channel.GetSessionID())) {
            if (!mediaWaitForConnect)
              channel.Start();
          }
          else {
            transmitterMediaStream = NULL;
            channel.Close();
          }
        }
        else
          channel.Start();
        if (channel.IsOpen())
          break;
      }
    }
  }
}


void H323Connection::OnSelectLogicalChannels()
{
  PTRACE(2, "H245\tDefault OnSelectLogicalChannels, " << fastStartState);

  // Select the first codec that uses the "standard" audio session.
  switch (fastStartState) {
    default : //FastStartDisabled :
      SelectDefaultLogicalChannel(OpalMediaFormat::DefaultAudioSessionID);
      if (endpoint.CanAutoStartTransmitVideo())
        SelectDefaultLogicalChannel(OpalMediaFormat::DefaultVideoSessionID);
      if (endpoint.CanAutoStartTransmitFax())
        SelectDefaultLogicalChannel(OpalMediaFormat::DefaultDataSessionID);
      break;

    case FastStartInitiate :
      SelectFastStartChannels(OpalMediaFormat::DefaultAudioSessionID, TRUE, TRUE);
      SelectFastStartChannels(OpalMediaFormat::DefaultVideoSessionID,
                              endpoint.CanAutoStartTransmitVideo(),
                              endpoint.CanAutoStartReceiveVideo());
      SelectFastStartChannels(OpalMediaFormat::DefaultDataSessionID,
                              endpoint.CanAutoStartTransmitFax(),
                              endpoint.CanAutoStartReceiveFax());
      break;

    case FastStartResponse :
      StartFastStartChannel(OpalMediaFormat::DefaultAudioSessionID, H323Channel::IsTransmitter);
      StartFastStartChannel(OpalMediaFormat::DefaultAudioSessionID, H323Channel::IsReceiver);
      if (endpoint.CanAutoStartTransmitVideo())
        StartFastStartChannel(OpalMediaFormat::DefaultVideoSessionID, H323Channel::IsTransmitter);
      if (endpoint.CanAutoStartReceiveVideo())
        StartFastStartChannel(OpalMediaFormat::DefaultVideoSessionID, H323Channel::IsReceiver);
      if (endpoint.CanAutoStartTransmitFax())
        StartFastStartChannel(OpalMediaFormat::DefaultDataSessionID, H323Channel::IsTransmitter);
      if (endpoint.CanAutoStartReceiveFax())
        StartFastStartChannel(OpalMediaFormat::DefaultDataSessionID, H323Channel::IsReceiver);
      break;
  }
}


void H323Connection::SelectDefaultLogicalChannel(unsigned sessionID)
{
  if (FindChannel(sessionID, FALSE))
    return; 

  for (PINDEX i = 0; i < localCapabilities.GetSize(); i++) {
    H323Capability & localCapability = localCapabilities[i];
    if (localCapability.GetDefaultSessionID() == sessionID) {
      H323Capability * remoteCapability = remoteCapabilities.FindCapability(localCapability);
      if (remoteCapability != NULL) {
        PTRACE(3, "H323\tSelecting " << *remoteCapability);
        if (OpenLogicalChannel(*remoteCapability, sessionID, H323Channel::IsTransmitter))
          break;
        PTRACE(2, "H323\tOnSelectLogicalChannels, OpenLogicalChannel failed: "
               << *remoteCapability);
      }
    }
  }
}


void H323Connection::SelectFastStartChannels(unsigned sessionID,
                                             BOOL transmitter,
                                             BOOL receiver)
{
  // Select all of the fast start channels to offer to the remote when initiating a call.
  for (PINDEX i = 0; i < localCapabilities.GetSize(); i++) {
    H323Capability & capability = localCapabilities[i];
    if (capability.GetDefaultSessionID() == sessionID) {
      if (receiver) {
        if (!OpenLogicalChannel(capability, sessionID, H323Channel::IsReceiver)) {
          PTRACE(2, "H323\tOnSelectLogicalChannels, OpenLogicalChannel rx failed: " << capability);
        }
      }
      if (transmitter) {
        if (!OpenLogicalChannel(capability, sessionID, H323Channel::IsTransmitter)) {
          PTRACE(2, "H323\tOnSelectLogicalChannels, OpenLogicalChannel tx failed: " << capability);
        }
      }
    }
  }
}


BOOL H323Connection::OpenLogicalChannel(const H323Capability & capability,
                                        unsigned sessionID,
                                        H323Channel::Directions dir)
{
  switch (fastStartState) {
    default : // FastStartDisabled
      if (dir == H323Channel::IsReceiver)
        return FALSE;

      // Traditional H245 handshake
      if (!logicalChannels->Open(capability, sessionID))
        return FALSE;
      transmitterMediaStream = logicalChannels->FindChannelBySession(sessionID, FALSE)->GetMediaStream();
      if (GetCall().OpenSourceMediaStreams(*this, capability.GetMediaFormat(), sessionID))
        return TRUE;
      PTRACE(2, "H323\tOpenLogicalChannel, OpenSourceMediaStreams failed: " << capability);
      return FALSE;

    case FastStartResponse :
      // Do not use OpenLogicalChannel for starting these.
      return FALSE;

    case FastStartInitiate :
      break;
  }

  /*If starting a receiver channel and are initiating the fast start call,
    indicated by the remoteCapabilities being empty, we do a "trial"
    listen on the channel. That is, for example, the UDP sockets are created
    to receive data in the RTP session, but no thread is started to read the
    packets and pass them to the codec. This is because at this point in time,
    we do not know which of the codecs is to be used, and more than one thread
    cannot read from the RTP ports at the same time.
  */
  H323Channel * channel = capability.CreateChannel(*this, dir, sessionID, NULL);
  if (channel == NULL)
    return FALSE;

  if (dir != H323Channel::IsReceiver)
    channel->SetNumber(logicalChannels->GetNextChannelNumber());

  fastStartChannels.Append(channel);
  return TRUE;
}


BOOL H323Connection::OnOpenLogicalChannel(const H245_OpenLogicalChannel & /*openPDU*/,
                                          H245_OpenLogicalChannelAck & /*ackPDU*/,
                                          unsigned & /*errorCode*/)
{
  // If get a OLC via H.245 stop trying to do fast start
  fastStartState = FastStartDisabled;
  if (!fastStartChannels.IsEmpty()) {
    fastStartChannels.RemoveAll();
    PTRACE(1, "H245\tReceived early start OLC, aborting fast start");
  }

  //errorCode = H245_OpenLogicalChannelReject_cause::e_unspecified;
  return TRUE;
}


BOOL H323Connection::OnConflictingLogicalChannel(H323Channel & conflictingChannel)
{
  unsigned session = conflictingChannel.GetSessionID();
  PTRACE(2, "H323\tLogical channel " << conflictingChannel
         << " conflict on session " << session
         << ", codec: " << conflictingChannel.GetCapability());

  /* Matrix of conflicts:
       Local EP is master and conflicting channel from remote (OLC)
          Reject remote transmitter (function is not called)
       Local EP is master and conflicting channel to remote (OLCAck)
          Should not happen (function is not called)
       Local EP is slave and conflicting channel from remote (OLC)
          Close sessions reverse channel from remote
          Start new reverse channel using codec in conflicting channel
          Accept the OLC for masters transmitter
       Local EP is slave and conflicting channel to remote (OLCRej)
          Start transmitter channel using codec in sessions reverse channel

      Upshot is this is only called if a slave and require a restart of
      some channel. Possibly closing channels as master has precedence.
   */

  BOOL fromRemote = conflictingChannel.GetNumber().IsFromRemote();
  H323Channel * channel = FindChannel(session, !fromRemote);
  if (channel == NULL) {
    PTRACE(1, "H323\tCould not resolve conflict, no reverse channel.");
    return FALSE;
  }

  if (!fromRemote) {
    conflictingChannel.Close();
    H323Capability * capability = remoteCapabilities.FindCapability(channel->GetCapability());
    if (capability == NULL) {
      PTRACE(1, "H323\tCould not resolve conflict, capability not available on remote.");
      return FALSE;
    }
    OpenLogicalChannel(*capability, session, H323Channel::IsTransmitter);
    return TRUE;
  }

  // Close the conflicting channel that got in before our transmitter
  channel->Close();
  CloseLogicalChannelNumber(channel->GetNumber());

  // Get the conflisting channel number to close
  H323ChannelNumber number = channel->GetNumber();

  // Must be slave and conflict from something we are sending, so try starting a
  // new channel using the master endpoints transmitter codec.
  logicalChannels->Open(conflictingChannel.GetCapability(), session, number);

  // Now close the conflicting channel
  CloseLogicalChannelNumber(number);
  return TRUE;
}


H323Channel * H323Connection::CreateLogicalChannel(const H245_OpenLogicalChannel & open,
                                                   BOOL startingFast,
                                                   unsigned & errorCode)
{
  const H245_H2250LogicalChannelParameters * param;
  const H245_DataType * dataType;
  H323Channel::Directions direction;
  H323Capability * capability;

  if (startingFast && open.HasOptionalField(H245_OpenLogicalChannel::e_reverseLogicalChannelParameters)) {
    if (open.m_reverseLogicalChannelParameters.m_multiplexParameters.GetTag() !=
              H245_OpenLogicalChannel_reverseLogicalChannelParameters_multiplexParameters
                                                      ::e_h2250LogicalChannelParameters) {
      errorCode = H245_OpenLogicalChannelReject_cause::e_unsuitableReverseParameters;
      PTRACE(2, "H323\tCreateLogicalChannel - reverse channel, H225.0 only supported");
      return NULL;
    }

    PTRACE(3, "H323\tCreateLogicalChannel - reverse channel");
    dataType = &open.m_reverseLogicalChannelParameters.m_dataType;
    param = &(const H245_H2250LogicalChannelParameters &)
                      open.m_reverseLogicalChannelParameters.m_multiplexParameters;
    direction = H323Channel::IsTransmitter;

    capability = remoteCapabilities.FindCapability(*dataType);
    if (capability == NULL) {
      // If we actually have the remote capabilities then the remote (very oddly)
      // had a fast connect entry it could not do. If we have not yet got a remote
      // cap table then build one using all possible caps.
      if (capabilityExchangeProcedure->HasReceivedCapabilities() ||
                (capability = endpoint.FindCapability(*dataType)) == NULL) {
        errorCode = H245_OpenLogicalChannelReject_cause::e_unknownDataType;
        PTRACE(2, "H323\tCreateLogicalChannel - unknown data type");
        return NULL; // If codec not supported, return error
      }
      
      capability = remoteCapabilities.Copy(*capability);
      remoteCapabilities.SetCapability(0, capability->GetDefaultSessionID(), capability);
    }
  }
  else {
    if (open.m_forwardLogicalChannelParameters.m_multiplexParameters.GetTag() !=
              H245_OpenLogicalChannel_forwardLogicalChannelParameters_multiplexParameters
                                                      ::e_h2250LogicalChannelParameters) {
      PTRACE(2, "H323\tCreateLogicalChannel - forward channel, H225.0 only supported");
      errorCode = H245_OpenLogicalChannelReject_cause::e_unspecified;
      return NULL;
    }

    PTRACE(3, "H323\tCreateLogicalChannel - forward channel");
    dataType = &open.m_forwardLogicalChannelParameters.m_dataType;
    param = &(const H245_H2250LogicalChannelParameters &)
                      open.m_forwardLogicalChannelParameters.m_multiplexParameters;
    direction = H323Channel::IsReceiver;

    // See if datatype is supported
    capability = localCapabilities.FindCapability(*dataType);
    if (capability == NULL) {
      errorCode = H245_OpenLogicalChannelReject_cause::e_unknownDataType;
      PTRACE(2, "H323\tCreateLogicalChannel - unknown data type");
      return NULL; // If codec not supported, return error
    }
  }

  if (!capability->OnReceivedPDU(*dataType, direction == H323Channel::IsReceiver)) {
    errorCode = H245_OpenLogicalChannelReject_cause::e_dataTypeNotSupported;
    PTRACE(2, "H323\tCreateLogicalChannel - data type not supported");
    return NULL; // If codec not supported, return error
  }

  if (!OnCreateLogicalChannel(*capability, direction, errorCode))
    return NULL; // If codec combination not supported, return error

  H323Channel * channel = capability->CreateChannel(*this, direction, param->m_sessionID, param);
  if (channel == NULL) {
    errorCode = H245_OpenLogicalChannelReject_cause::e_dataTypeNotAvailable;
    PTRACE(2, "H323\tCreateLogicalChannel - data type not available");
    return NULL;
  }

  if (!channel->SetInitialBandwidth())
    errorCode = H245_OpenLogicalChannelReject_cause::e_insufficientBandwidth;
  else if (channel->OnReceivedPDU(open, errorCode))
    return channel;

  PTRACE(2, "H323\tOnReceivedPDU gave error " << errorCode);
  delete channel;
  return NULL;
}


H323Channel * H323Connection::CreateRealTimeLogicalChannel(const H323Capability & capability,
                                                          H323Channel::Directions dir,
                                                                         unsigned sessionID,
							                         const H245_H2250LogicalChannelParameters * param,
                                                                        RTP_QOS * rtpqos)
{
  if (ownerCall.IsMediaBypassPossible(*this, sessionID)) {
    OpalTransportAddress data;
    OpalTransportAddress control;
    if (!GetExternalRTPAddress(sessionID, data, control))
      return NULL;

    //if (IsOriginating())
      return new H323_ExternalRTPChannel(*this, capability, dir, sessionID, data, control);

    //return new H323_ExternalRTPChannel(*this, capability, dir, sessionID);
  }

  RTP_Session * session;

  if (param != NULL) {
    // We only support unicast IP at this time.
    if (param->m_mediaControlChannel.GetTag() != H245_TransportAddress::e_unicastAddress)
      return NULL;

    const H245_UnicastAddress & uaddr = param->m_mediaControlChannel;
    if (uaddr.GetTag() != H245_UnicastAddress::e_iPAddress)
      return NULL;

    sessionID = param->m_sessionID;
  }

  session = UseSession(GetControlChannel(), sessionID, rtpqos);
  if (session == NULL)
    return NULL;

  ((RTP_UDP *) session)->Reopen(dir == H323Channel::IsReceiver);
  return new H323_RTPChannel(*this, capability, dir, *session);
}


BOOL H323Connection::OnCreateLogicalChannel(const H323Capability & capability,
                                            H323Channel::Directions dir,
                                            unsigned & errorCode)
{
  if (connectionState == ShuttingDownConnection) {
    errorCode = H245_OpenLogicalChannelReject_cause::e_unspecified;
    return FALSE;
  }

  // Default error if returns FALSE
  errorCode = H245_OpenLogicalChannelReject_cause::e_dataTypeALCombinationNotSupported;

  // Check if in set at all
  if (dir != H323Channel::IsReceiver) {
    if (!remoteCapabilities.IsAllowed(capability)) {
      PTRACE(2, "H323\tOnCreateLogicalChannel - transmit capability " << capability << " not allowed.");
      return FALSE;
    }
  }
  else {
    if (!localCapabilities.IsAllowed(capability)) {
      PTRACE(2, "H323\tOnCreateLogicalChannel - receive capability " << capability << " not allowed.");
      return FALSE;
    }
  }

  // Check all running channels, and if new one can't run with it return FALSE
  for (PINDEX i = 0; i < logicalChannels->GetSize(); i++) {
    H323Channel * channel = logicalChannels->GetChannelAt(i);
    if (channel != NULL && channel->GetDirection() == dir) {
      if (dir != H323Channel::IsReceiver) {
        if (!remoteCapabilities.IsAllowed(capability, channel->GetCapability())) {
          PTRACE(2, "H323\tOnCreateLogicalChannel - transmit capability " << capability
                 << " and " << channel->GetCapability() << " incompatible.");
          return FALSE;
        }
      }
      else {
        if (!localCapabilities.IsAllowed(capability, channel->GetCapability())) {
          PTRACE(2, "H323\tOnCreateLogicalChannel - transmit capability " << capability
                 << " and " << channel->GetCapability() << " incompatible.");
          return FALSE;
        }
      }
    }
  }

  return TRUE;
}


BOOL H323Connection::OnStartLogicalChannel(H323Channel & channel)
{
  if (channel.GetSessionID() == OpalMediaFormat::DefaultAudioSessionID &&
      PIsDescendant(&channel, H323_RTPChannel)) {
    OpalMediaPatch * patch = channel.GetMediaStream()->GetPatch();
    if (patch != NULL) {
      if (channel.GetNumber().IsFromRemote()) {
        patch->AddFilter(rfc2833Handler->GetReceiveHandler());
        if (detectInBandDTMF)
          patch->AddFilter(PCREATE_NOTIFIER(OnUserInputInBandDTMF), OPAL_PCM16);
      }
      else
        patch->AddFilter(rfc2833Handler->GetTransmitHandler());
    }
  }
  return endpoint.OnStartLogicalChannel(*this, channel);
}


void H323Connection::CloseLogicalChannel(unsigned number, BOOL fromRemote)
{
  if (connectionState != ShuttingDownConnection)
    logicalChannels->Close(number, fromRemote);
}


void H323Connection::CloseLogicalChannelNumber(const H323ChannelNumber & number)
{
  CloseLogicalChannel(number, number.IsFromRemote());
}


void H323Connection::CloseAllLogicalChannels(BOOL fromRemote)
{
  for (PINDEX i = 0; i < logicalChannels->GetSize(); i++) {
    H245NegLogicalChannel & negChannel = logicalChannels->GetNegLogicalChannelAt(i);
    H323Channel * channel = negChannel.GetChannel();
    if (channel != NULL && channel->GetNumber().IsFromRemote() == fromRemote)
      negChannel.Close();
  }
}


BOOL H323Connection::OnClosingLogicalChannel(H323Channel & /*channel*/)
{
  return TRUE;
}


void H323Connection::OnClosedLogicalChannel(const H323Channel & channel)
{
  mediaStreams.Remove(channel.GetMediaStream());
  endpoint.OnClosedLogicalChannel(*this, channel);
}


void H323Connection::OnLogicalChannelFlowControl(H323Channel * channel,
                                                 long bitRateRestriction)
{
  if (channel != NULL)
    channel->OnFlowControl(bitRateRestriction);
}


void H323Connection::OnLogicalChannelJitter(H323Channel * channel,
                                            DWORD jitter,
                                            int skippedFrameCount,
                                            int additionalBuffer)
{
  if (channel != NULL)
    channel->OnJitterIndication(jitter, skippedFrameCount, additionalBuffer);
}


unsigned H323Connection::GetBandwidthUsed() const
{
  unsigned used = 0;

  for (PINDEX i = 0; i < logicalChannels->GetSize(); i++) {
    H323Channel * channel = logicalChannels->GetChannelAt(i);
    if (channel != NULL)
      used += channel->GetBandwidthUsed();
  }

  PTRACE(3, "H323\tBandwidth used: " << used);

  return used;
}


BOOL H323Connection::SetBandwidthAvailable(unsigned newBandwidth, BOOL force)
{
  unsigned used = GetBandwidthUsed();
  if (used > newBandwidth) {
    if (!force)
      return FALSE;

    // Go through logical channels and close down some.
    PINDEX chanIdx = logicalChannels->GetSize();
    while (used > newBandwidth && chanIdx-- > 0) {
      H323Channel * channel = logicalChannels->GetChannelAt(chanIdx);
      if (channel != NULL) {
        used -= channel->GetBandwidthUsed();
        CloseLogicalChannelNumber(channel->GetNumber());
      }
    }
  }

  bandwidthAvailable = newBandwidth - used;
  return TRUE;
}


void H323Connection::SetSendUserInputMode(SendUserInputModes mode)
{
  PAssert(mode != SendUserInputAsSeparateRFC2833, PUnimplementedFunction);

  PTRACE(2, "H323\tSetting default User Input send mode to " << mode);
  sendUserInputMode = mode;
}


static BOOL CheckSendUserInputMode(const H323Capabilities & caps,
                                   H323Connection::SendUserInputModes mode)
{
  // If have remote capabilities, then verify we can send selected mode,
  // otherwise just return and accept it for future validation
  static const H323_UserInputCapability::SubTypes types[H323Connection::NumSendUserInputModes] = {
    H323_UserInputCapability::NumSubTypes,
    H323_UserInputCapability::BasicString,
    H323_UserInputCapability::SignalToneH245,
    H323_UserInputCapability::SignalToneRFC2833,
    H323_UserInputCapability::NumSubTypes,
  };

  if (types[mode] == H323_UserInputCapability::NumSubTypes)
    return mode == H323Connection::SendUserInputAsQ931;

  return caps.FindCapability(H323_UserInputCapability::SubTypeNames[types[mode]]) != NULL;
}


H323Connection::SendUserInputModes H323Connection::GetRealSendUserInputMode() const
{
  // If have not yet exchanged capabilities (ie not finished setting up the
  // H.245 channel) then the only thing we can do is Q.931
  if (!capabilityExchangeProcedure->HasReceivedCapabilities())
    return SendUserInputAsQ931;

  // First try recommended mode
  if (CheckSendUserInputMode(remoteCapabilities, sendUserInputMode))
    return sendUserInputMode;

  // Then try H.245 tones
  if (CheckSendUserInputMode(remoteCapabilities, SendUserInputAsTone))
    return SendUserInputAsTone;

  // Then try H.245 strings
  if (CheckSendUserInputMode(remoteCapabilities, SendUserInputAsString))
    return SendUserInputAsString;

  // Finally if is H.245 alphanumeric or does not indicate it could do other
  // modes we use H.245 alphanumeric as per spec.
  return SendUserInputAsString;
}


BOOL H323Connection::SendUserInputString(const PString & value)
{
  SendUserInputModes mode = GetRealSendUserInputMode();

  PTRACE(2, "H323\tSendUserInput(\"" << value << "\"), using mode " << mode);

  if (mode == SendUserInputAsString)
    return SendUserInputIndicationString(value);

  return OpalConnection::SendUserInputString(value);
}


BOOL H323Connection::SendUserInputTone(char tone, unsigned duration)
{
  SendUserInputModes mode = GetRealSendUserInputMode();

  PTRACE(2, "H323\tSendUserInputTime('" << tone << "', " << duration << "), using mode " << mode);

  switch (mode) {
    case SendUserInputAsQ931 :
      SendUserInputIndicationQ931(PString(tone));
      break;

    case SendUserInputAsString :
      SendUserInputIndicationString(PString(tone));
      break;

    case SendUserInputAsTone :
      return SendUserInputIndicationTone(tone, duration);

    default :
      ;
  }

  return OpalConnection::SendUserInputTone(tone, duration);
}


BOOL H323Connection::SendUserInputIndicationQ931(const PString & value)
{
  PTRACE(2, "H323\tSendUserInputIndicationQ931(\"" << value << "\")");

  H323SignalPDU pdu;
  pdu.BuildInformation(*this);
  pdu.GetQ931().SetKeypad(value);
  if (WriteSignalPDU(pdu))
    return TRUE;

  ClearCall(EndedByTransportFail);
  return FALSE;
}


BOOL H323Connection::SendUserInputIndicationString(const PString & value)
{
  PTRACE(2, "H323\tSendUserInputIndicationString(\"" << value << "\")");

  H323ControlPDU pdu;
  PASN_GeneralString & str = pdu.BuildUserInputIndication(value);
  if (!str.GetValue())
    return WriteControlPDU(pdu);

  PTRACE(1, "H323\tInvalid characters for UserInputIndication");
  return FALSE;
}


BOOL H323Connection::SendUserInputIndicationTone(char tone,
                                                 unsigned duration,
                                                 unsigned logicalChannel,
                                                 unsigned rtpTimestamp)
{
  PTRACE(2, "H323\tSendUserInputIndicationTone("
         << tone << ','
         << duration << ','
         << logicalChannel << ','
         << rtpTimestamp << ')');

  H323ControlPDU pdu;
  pdu.BuildUserInputIndication(tone, duration, logicalChannel, rtpTimestamp);
  return WriteControlPDU(pdu);
}


BOOL H323Connection::SendUserInputIndication(const H245_UserInputIndication & indication)
{
  H323ControlPDU pdu;
  H245_UserInputIndication & ind = pdu.Build(H245_IndicationMessage::e_userInput);
  ind = indication;
  return WriteControlPDU(pdu);
}


void H323Connection::OnUserInputIndication(const H245_UserInputIndication & ind)
{
  switch (ind.GetTag()) {
    case H245_UserInputIndication::e_alphanumeric :
      OnUserInputString((const PASN_GeneralString &)ind);
      break;

    case H245_UserInputIndication::e_signal :
    {
      const H245_UserInputIndication_signal & sig = ind;
      OnUserInputTone(sig.m_signalType[0],
                      sig.HasOptionalField(H245_UserInputIndication_signal::e_duration)
                                ? (unsigned)sig.m_duration : 0);
      break;
    }
    case H245_UserInputIndication::e_signalUpdate :
    {
      const H245_UserInputIndication_signalUpdate & sig = ind;
      OnUserInputTone(' ', sig.m_duration);
      break;
    }
  }
}


RTP_Session * H323Connection::GetSession(unsigned sessionID) const
{
  return rtpSessions.GetSession(sessionID);
}


H323_RTP_Session * H323Connection::GetSessionCallbacks(unsigned sessionID) const
{
  RTP_Session * session = rtpSessions.GetSession(sessionID);
  if (session == NULL)
    return NULL;

  PTRACE(3, "RTP\tFound existing session " << sessionID);
  PObject * data = session->GetUserData();
  PAssert(PIsDescendant(data, H323_RTP_Session), PInvalidCast);
  return (H323_RTP_Session *)data;
}


RTP_Session * H323Connection::UseSession(const OpalTransport & transport,
                                         unsigned sessionID,
                                         RTP_QOS * rtpqos)
{
  RTP_UDP * udp_session = (RTP_UDP *)OpalConnection::UseSession(transport, sessionID, rtpqos);
  if (udp_session == NULL)
    return NULL;

  if (udp_session->GetUserData() == NULL)
    udp_session->SetUserData(new H323_RTP_UDP(*this, *udp_session));
  return udp_session;
}


void H323Connection::ReleaseSession(unsigned sessionID)
{
  rtpSessions.ReleaseSession(sessionID);
}


void H323Connection::OnRTPStatistics(const RTP_Session & session) const
{
  endpoint.OnRTPStatistics(*this, session);
}


static void AddSessionCodecName(PStringStream & name, H323Channel * channel)
{
  if (channel == NULL)
    return;

  OpalMediaStream * stream = ((H323UnidirectionalChannel*)channel)->GetMediaStream();
  if (stream == NULL)
    return;

  OpalMediaFormat mediaFormat = stream->GetMediaFormat();
  if (mediaFormat.IsEmpty())
    return;

  if (name.IsEmpty())
    name << mediaFormat;
  else if (name != mediaFormat)
    name << " / " << mediaFormat;
}


PString H323Connection::GetSessionCodecNames(unsigned sessionID) const
{
  PStringStream name;

  AddSessionCodecName(name, FindChannel(sessionID, FALSE));
  AddSessionCodecName(name, FindChannel(sessionID, TRUE));

  return name;
}


BOOL H323Connection::RequestModeChange(const PString & newModes)
{
  return requestModeProcedure->StartRequest(newModes);
}


BOOL H323Connection::RequestModeChange(const H245_ArrayOf_ModeDescription & newModes)
{
  return requestModeProcedure->StartRequest(newModes);
}


BOOL H323Connection::OnRequestModeChange(const H245_RequestMode & pdu,
                                         H245_RequestModeAck & /*ack*/,
                                         H245_RequestModeReject & /*reject*/,
                                         PINDEX & selectedMode)
{
  for (selectedMode = 0; selectedMode < pdu.m_requestedModes.GetSize(); selectedMode++) {
    BOOL ok = TRUE;
    for (PINDEX i = 0; i < pdu.m_requestedModes[selectedMode].GetSize(); i++) {
      if (localCapabilities.FindCapability(pdu.m_requestedModes[selectedMode][i]) == NULL) {
        ok = FALSE;
        break;
      }
    }
    if (ok)
      return TRUE;
  }

  PTRACE(1, "H245\tMode change rejected as does not have capabilities");
  return FALSE;
}


void H323Connection::OnModeChanged(const H245_ModeDescription & newMode)
{
  CloseAllLogicalChannels(FALSE);

  // Start up the new ones
  for (PINDEX i = 0; i < newMode.GetSize(); i++) {
    H323Capability * capability = localCapabilities.FindCapability(newMode[i]);
    if (PAssertNULL(capability) != NULL) { // Should not occur as OnRequestModeChange checks them
      if (!OpenLogicalChannel(*capability,
                              capability->GetDefaultSessionID(),
                              H323Channel::IsTransmitter)) {
        PTRACE(1, "H245\tCould not open channel after mode change: " << *capability);
      }
    }
  }
}


void H323Connection::OnAcceptModeChange(const H245_RequestModeAck & pdu)
{
  if (t38ModeChangeCapabilities.IsEmpty())
    return;

  PTRACE(2, "H323\tT.38 mode change accepted.");

  // Now we have conviced the other side to send us T.38 data we should do the
  // same assuming the RequestModeChangeT38() function provided a list of \n
  // separaete capability names to start. Only one will be.

  CloseAllLogicalChannels(FALSE);

  PStringArray modes = t38ModeChangeCapabilities.Lines();

  PINDEX first, last;
  if (pdu.m_response.GetTag() == H245_RequestModeAck_response::e_willTransmitMostPreferredMode) {
    first = 0;
    last = 1;
  }
  else {
    first = 1;
    last = modes.GetSize();
  }

  for (PINDEX i = first; i < last; i++) {
    H323Capability * capability = localCapabilities.FindCapability(modes[i]);
    if (capability != NULL && OpenLogicalChannel(*capability,
                                                 capability->GetDefaultSessionID(),
                                                 H323Channel::IsTransmitter)) {
      PTRACE(1, "H245\tOpened " << *capability << " after T.38 mode change");
      break;
    }

    PTRACE(1, "H245\tCould not open channel after T.38 mode change");
  }

  t38ModeChangeCapabilities = PString::Empty();
}


void H323Connection::OnRefusedModeChange(const H245_RequestModeReject * /*pdu*/)
{
  if (!t38ModeChangeCapabilities) {
    PTRACE(2, "H323\tT.38 mode change rejected.");
    t38ModeChangeCapabilities = PString::Empty();
  }
}


BOOL H323Connection::RequestModeChangeT38(const char * capabilityNames)
{
  t38ModeChangeCapabilities = capabilityNames;
  if (RequestModeChange(t38ModeChangeCapabilities))
    return TRUE;

  t38ModeChangeCapabilities = PString::Empty();
  return FALSE;
}


BOOL H323Connection::GetAdmissionRequestAuthentication(const H225_AdmissionRequest & /*arq*/,
                                                       H235Authenticators & /*authenticators*/)
{
  return FALSE;
}


const H323Transport & H323Connection::GetControlChannel() const
{
  return *(controlChannel != NULL ? controlChannel : signallingChannel);
}

OpalTransport & H323Connection::GetTransport() const
{
  return *(controlChannel != NULL ? controlChannel : signallingChannel);
}

void H323Connection::SendLogicalChannelMiscCommand(H323Channel & channel,
                                                   unsigned commandIdentifier)
{
  if (channel.GetDirection() == H323Channel::IsReceiver) {
    H323ControlPDU pdu;
    H245_CommandMessage & command = pdu.Build(H245_CommandMessage::e_miscellaneousCommand);
    H245_MiscellaneousCommand & miscCommand = command;
    miscCommand.m_logicalChannelNumber = (unsigned)channel.GetNumber();
    miscCommand.m_type.SetTag(commandIdentifier);
    WriteControlPDU(pdu);
  }
}


void H323Connection::SetEnforcedDurationLimit(unsigned seconds)
{
  enforcedDurationLimit.SetInterval(0, seconds);
}


void H323Connection::MonitorCallStatus()
{
  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked())
    return;

  if (phase >= ReleasingPhase)
    return;

  if (endpoint.GetRoundTripDelayRate() > 0 && !roundTripDelayTimer.IsRunning()) {
    roundTripDelayTimer = endpoint.GetRoundTripDelayRate();
    StartRoundTripDelay();
  }

/*
  if (endpoint.GetNoMediaTimeout() > 0) {
    BOOL oneRunning = FALSE;
    BOOL allSilent = TRUE;
    for (PINDEX i = 0; i < logicalChannels->GetSize(); i++) {
      H323Channel * channel = logicalChannels->GetChannelAt(i);
      if (channel != NULL && channel->IsDescendant(H323_RTPChannel::Class())) {
        if (channel->IsRunning()) {
          oneRunning = TRUE;
          if (((H323_RTPChannel *)channel)->GetSilenceDuration() < endpoint.GetNoMediaTimeout()) {
            allSilent = FALSE;
            break;
          }
        }
      }
    }
    if (oneRunning && allSilent)
      ClearCall(EndedByTransportFail);
  }
*/

  if (enforcedDurationLimit.GetResetTime() > 0 && enforcedDurationLimit == 0)
    ClearCall(EndedByDurationLimit);
}


/////////////////////////////////////////////////////////////////////////////
