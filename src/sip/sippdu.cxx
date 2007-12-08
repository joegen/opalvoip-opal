/*
 * sippdu.cxx
 *
 * Session Initiation Protocol PDU support.
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (c) 2000 Equivalence Pty. Ltd.
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
 * The Original Code is Open Phone Abstraction Library.
 *
 * The Initial Developer of the Original Code is Equivalence Pty. Ltd.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: sippdu.cxx,v $
 * Revision 2.154  2007/10/10 22:23:47  csoutheren
 * Who would beleive that z9hG4bK is a special prefix mandated by RFC3261
 * I certainly didn't, but I do now. Just goes to show you can't trust anyone :)
 *
 * Revision 2.153  2007/10/10 22:17:41  csoutheren
 * Removed confusing literal string in branch ID
 *
 * Revision 2.152  2007/10/10 19:52:43  dsandras
 * Added missing magic cookie. Fixes Ekiga report #485407.
 *
 * Revision 2.151  2007/10/10 01:23:20  rjongbloed
 * Fixed server address from SRV lookup being overwritten by URI host.
 *
 * Revision 2.150  2007/10/03 23:59:05  rjongbloed
 * Fixed correct operation of DNS SRV lookups to RFC3263 specification,
 *   thanks to Will Hawkins and Kris Marsh for what needs to be done.
 *
 * Revision 2.149  2007/09/24 23:38:39  csoutheren
 * Fixed errors on 64 bit Linux
 *
 * Revision 2.148  2007/09/21 01:34:10  rjongbloed
 * Rewrite of SIP transaction handling to:
 *   a) use PSafeObject and safe collections
 *   b) only one database of transactions, remove connection copy
 *   c) fix timers not always firing due to bogus deadlock avoidance
 *   d) cleaning up only occurs in the existing garbage collection thread
 *   e) use of read/write mutex on endpoint list to avoid possible deadlock
 *
 * Revision 2.147  2007/09/05 13:45:29  csoutheren
 * Applied 1748937 - SIP support for combined Via header
 * Thanks to Simon Zwahlen
 *
 * Revision 2.146  2007/09/04 03:10:28  rjongbloed
 * Make sure late arriving 1xx responses do not "reset" the transaction state when
 *   it is already completed. Also assure a completed transaction only passes the
 *   response that did the completion on to connection/endpoint so multiple
 *   responses do not confuse higher level logic.
 *
 * Revision 2.145  2007/08/24 06:38:53  csoutheren
 * Add way to get empty DisplayName without always using default
 *
 * Revision 2.144  2007/08/22 09:02:19  csoutheren
 * Allow setting of explicit From field in SIP
 *
 * Revision 2.143  2007/08/17 07:55:57  rjongbloed
 * Added more error reporting.
 *
 * Revision 2.142  2007/08/15 09:51:21  dsandras
 * Backport from prePresenceBranch.
 *
 * Revision 2.141  2007/08/14 13:14:29  csoutheren
 * Guard against ridiculous values in SIP headers
 *
 * Revision 2.140  2007/08/13 04:02:49  csoutheren
 * Allow override of SIP display name using StringOptions
 * Normalise setting of local party name
 *
 * Revision 2.139  2007/08/09 08:20:44  csoutheren
 * Fix problem with incorrect contact address being used
 *
 * Revision 2.138  2007/07/22 13:02:19  rjongbloed
 * Cleaned up selection of registered name usage of URL versus host name.
 *
 * Revision 2.137  2007/07/22 11:18:12  rjongbloed
 * Fixed crash if get "unusual" User-Agent field.
 *
 * Revision 2.136  2007/07/05 05:42:53  rjongbloed
 * Removed extraneous transport multiple transmit end connection, not needed
 *   with new socket bundling changes.
 *
 * Revision 2.135  2007/07/05 03:58:22  rjongbloed
 * Cleaned up some odd looking code
 *
 * Revision 2.134  2007/07/02 04:07:58  rjongbloed
 * Added hooks to get at PDU strings being read/written.
 *
 * Revision 2.133  2007/06/29 07:20:26  rjongbloed
 * Small clean up of User-Agent string.
 *
 * Revision 2.132  2007/06/29 06:59:58  rjongbloed
 * Major improvement to the "product info", normalising H.221 and User-Agent mechanisms.
 *
 * Revision 2.131  2007/06/10 08:55:13  rjongbloed
 * Major rework of how SIP utilises sockets, using new "socket bundling" subsystem.
 *
 * Revision 2.130  2007/06/09 15:40:05  dsandras
 * Fixed routing of ACK PDUs. Only an ACK sent for a non 2XX response needs
 * to copy the Route header from the original request.
 *
 * Revision 2.129  2007/06/05 21:37:28  dsandras
 * Use the route set directly from the PDU instead of using the route set
 * from the connection. Make sure the route set is being used when routing
 * all types of PDUs.
 *
 * Revision 2.128  2007/05/28 08:38:35  csoutheren
 * Sanity check Content-Length field
 *
 * Revision 2.127  2007/05/27 02:03:03  csoutheren
 * Applied Ekiga Bugzilla 368524 - Digest Authentication Issues
 * Thankls to Rui Carmo for finding and Knut Omang for fixing
 *
 * Revision 2.126  2007/05/23 20:53:40  dsandras
 * We should release the current session if no ACK is received after
 * an INVITE answer for a period of 64*T1. Don't trigger the ACK timer
 * when sending an ACK, only when not receiving one.
 *
 * Revision 2.125  2007/05/16 01:17:07  csoutheren
 * Added new files to Windows build
 * Removed compiler warnings on Windows
 * Added backwards compatible SIP Register function
 *
 * Revision 2.124  2007/05/15 20:47:20  dsandras
 * Added various handlers to manage subscriptions for presence, message
 * waiting indications, registrations, state publishing,
 * message conversations, ...
 * Adds/fixes support for RFC3856, RFC3903, RFC3863, RFC3265, ...
 * Many improvements over the original SIPInfo code.
 * Code contributed by NOVACOM (http://www.novacom.be) thanks to
 * EuroWeb (http://www.euroweb.hu).
 *
 * Revision 2.123  2007/05/01 05:35:27  rjongbloed
 * Print received SIP PDU as early as possible, in case of crash.
 *
 * Revision 2.122  2007/04/17 21:49:41  dsandras
 * Fixed Via field in previous commit.
 * Make sure the correct port is being used.
 * Improved FindSIPInfoByDomain.
 *
 * Revision 2.121  2007/04/15 10:09:15  dsandras
 * Some systems like CISCO Call Manager do not like having a Contact field in INVITE
 * PDUs which is different to the one being used in the original REGISTER request.
 * Added code to use the same Contact field in both cases if we can determine that
 * we are registered to that specific account and if there is a transport running.
 * Fixed problem where the SIP connection was not released with a BYE PDU when
 * the ACK is received while we are already in EstablishedPhase.
 *
 * Revision 2.120  2007/04/04 02:12:02  rjongbloed
 * Reviewed and adjusted PTRACE log levels
 *   Now follows 1=error,2=warn,3=info,4+=debug
 *
 * Revision 2.119  2007/03/30 14:45:32  hfriederich
 * Reorganization of hte way transactions are handled. Delete transactions
 *   in garbage collector when they're terminated. Update destructor code
 *   to improve safe destruction of SIPEndPoint instances.
 *
 * Revision 2.118  2007/03/29 05:16:50  csoutheren
 * Pass OpalConnection to OpalMediaSream constructor
 * Add ID to OpalMediaStreams so that transcoders can match incoming and outgoing codecs
 *
 * Revision 2.117  2007/03/27 20:16:23  dsandras
 * Temporarily removed use of shared transports as it could have unexpected
 * side effects on the routing of PDUs.
 * Various fixes on the way SIPInfo objects are being handled. Wait
 * for transports to be closed before being deleted. Added missing mutexes.
 * Added garbage collector.
 *
 * Revision 2.116  2007/01/10 09:16:55  csoutheren
 * Allow compilation with video disabled
 *
 * Revision 2.115  2006/12/18 03:18:42  csoutheren
 * Messy but simple fixes
 *   - Add access to SIP REGISTER timeout
 *   - Ensure OpalConnection options are correctly progagated
 *
 * Revision 2.114  2006/12/03 16:54:52  dsandras
 * Do not allow contentLength to be negative in order to prevent
 * crashes when receiving malformed PDUs. Fixes Ekiga report #379801.
 *
 * Revision 2.113  2006/11/02 03:09:06  csoutheren
 * Changed to use correct timeout when sending PDUs
 * Thanks to Peter Kocsis
 *
 * Revision 2.112  2006/10/01 17:16:32  hfriederich
 * Ensures that an ACK is sent out for every final response to INVITE
 *
 * Revision 2.111  2006/09/22 00:58:41  csoutheren
 * Fix usages of PAtomicInteger
 *
 * Revision 2.110  2006/08/28 00:42:25  csoutheren
 * Applied 1545201 - SIPTransaction - control of access to SIPConnection
 * Thanks to Drazen Dimoti
 *
 * Revision 2.109  2006/08/12 04:09:24  csoutheren
 * Applied 1538497 - Add the PING method
 * Thanks to Paul Rolland
 *
 * Revision 2.108  2006/08/03 08:09:52  csoutheren
 * Removed another incorrect double quote
 *
 * Revision 2.107  2006/07/29 08:31:19  hfriederich
 * Removing invalid quotation mark after algorithm=md5. Fixes problems with registration on certain servers
 *
 * Revision 2.106  2006/07/24 09:03:00  csoutheren
 * Removed suprious "response" clause in authentication response
 *
 * Revision 2.105  2006/07/14 13:44:22  csoutheren
 * Fix formatting
 * Fix compile warning on Linux
 *
 * Revision 2.104  2006/07/14 07:37:21  csoutheren
 * Implement qop authentication.
 *
 * Revision 2.103  2006/07/14 06:57:40  csoutheren
 * Fixed problem with opaque authentication
 *
 * Revision 2.102  2006/07/14 04:22:43  csoutheren
 * Applied 1517397 - More Phobos stability fix
 * Thanks to Dinis Rosario
 *
 * Revision 2.101  2006/07/14 01:15:51  csoutheren
 * Add support for "opaque" attribute in SIP authentication
 *
 * Revision 2.100  2006/07/09 10:18:29  csoutheren
 * Applied 1517393 - Opal T.38
 * Thanks to Drazen Dimoti
 *
 * Revision 2.99  2006/07/06 20:37:58  dsandras
 * Applied patch from Brian Lu <brian lu sun com> to fix compilation on opensolaris. thanks!
 *
 * Revision 2.98  2006/07/05 04:29:14  csoutheren
 * Applied 1495008 - Add a callback: OnCreatingINVITE
 * Thanks to mturconi
 *
 * Revision 2.97  2006/06/30 06:59:21  csoutheren
 * Applied 1494417 - Add check for ContentLength tag
 * Thanks to mturconi
 *
 * Revision 2.96  2006/06/30 01:05:18  csoutheren
 * Minor cleanups
 *
 * Revision 2.95  2006/05/30 04:58:06  csoutheren
 * Added suport for SIP INFO message (untested as yet)
 * Fixed some issues with SIP state machine on answering calls
 * Fixed some formatting issues
 *
 * Revision 2.94  2006/05/06 16:04:16  dsandras
 * Fixed GetSendAddress to handle the case where the first route doesn't contain
 * the 'lr' parameter. Fixes Ekiga report #340415.
 *
 * Revision 2.93  2006/05/04 11:17:34  hfriederich
 * improving SIPTransaction::Wait() to wait until transaction is complete instead of finished
 *
 * Revision 2.92  2006/04/11 21:58:25  dsandras
 * Various cleanups and fixes. Fixes Ekiga report #336444.
 *
 * Revision 2.91  2006/03/23 21:25:14  dsandras
 * Fixed parameter of callback called on registration timeout.
 * Simplified SIPOptions code.
 *
 * Revision 2.90  2006/03/20 00:25:36  csoutheren
 * Applied patch #1446482
 * Thanks to Adam Butcher
 *
 * Revision 2.89  2006/03/19 19:38:25  dsandras
 * Use full host when reporting a registration timeout.
 *
 * Revision 2.88  2006/03/19 13:15:12  dsandras
 * Removed cout.
 *
 * Revision 2.87  2006/03/19 12:23:55  dsandras
 * Fixed rport support. Fixes Ekiga report #335002.
 *
 * Revision 2.86  2006/03/18 21:56:07  dsandras
 * Remove REGISTER and SUBSCRIBE from the Allow field. Fixes Ekiga report
 * #334979.
 *
 * Revision 2.85  2006/03/14 10:26:34  dsandras
 * Reverted accidental previous change that was breaking retransmissions.
 *
 * Revision 2.84  2006/03/08 18:34:41  dsandras
 * Added DNS SRV lookup.
 *
 * Revision 2.83  2006/01/16 23:06:20  dsandras
 * Added old-style proxies support (those that do not support working as
 * outbound proxies thanks to an initial routeset).
 *
 * Revision 2.82  2006/01/14 10:43:06  dsandras
 * Applied patch from Brian Lu <Brian.Lu _AT_____ sun.com> to allow compilation
 * with OpenSolaris compiler. Many thanks !!!
 *
 * Revision 2.81  2006/01/12 20:23:44  dsandras
 * Reorganized things to prevent crashes when calling itself.
 *
 * Revision 2.80  2006/01/09 17:48:37  dsandras
 * Remove accidental paste. Let's blame vim.
 *
 * Revision 2.79  2006/01/09 13:01:02  dsandras
 * Prevent deadlock when exiting due to the mutex being locked and the completed
 * timeout notifier not executed yet.
 *
 * Revision 2.78  2006/01/08 14:42:49  dsandras
 * Added guards against closed transport.
 *
 * Revision 2.77  2006/01/02 11:28:07  dsandras
 * Some documentation. Various code cleanups to prevent duplicate code.
 *
 * Revision 2.76  2005/12/14 18:01:00  dsandras
 * Fixed comment.
 *
 * Revision 2.75  2005/12/04 15:01:59  dsandras
 * Fixed IP translation in the VIA field of most request PDUs.
 *
 * Revision 2.74  2005/10/22 18:01:21  dsandras
 * Added tag to FROM field in MESSAGE/REGISTER/SUBSCRIBE requests.
 *
 * Revision 2.73  2005/10/22 17:14:44  dsandras
 * Send an OPTIONS request periodically when STUN is being used to maintain the registrations binding alive.
 *
 * Revision 2.72  2005/10/18 17:23:54  dsandras
 * Fixed VIA in ACK request sent for a non-2xx response.
 *
 * Revision 2.71  2005/10/17 21:27:22  dsandras
 * Fixed VIA in CANCEL request.
 *
 * Revision 2.70  2005/10/09 19:09:34  dsandras
 * Max-Forwards must be part of all requests.
 *
 * Revision 2.69  2005/10/03 21:42:54  dsandras
 * Fixed previous commit (sorry).
 *
 * Revision 2.68  2005/10/03 21:40:38  dsandras
 * Use the port in the VIA, not the default signal port to answer to requests.
 *
 * Revision 2.67  2005/10/02 17:49:08  dsandras
 * Cleaned code to use the new GetContactAddress.
 *
 * Revision 2.66  2005/09/28 20:35:41  dsandras
 * Added support for the branch parameter in outgoing requests.
 *
 * Revision 2.65  2005/09/27 16:13:23  dsandras
 * - Use the targetAddress from the SIPConnection as request URI for a request
 * in a dialog. The SIPConnection class will adjust the targetAddress according
 * to the RFC, ie following the Contact field in a response and following the
 * Route fields.
 * - Added GetSendAddress that will return the OpalTransportAddress to use to
 * send a request in a dialog according to the RFC.
 * - Use SendPDU everywhere for requests in a dialog.
 * - Removed the transmission of ACK from the SIPInvite class so that it can
 * be done in the SIPConnectionc class after processing of the response in order
 * to know the route.
 * - Added the code for ACK requests sent for a 2xx response and for a non-2xx response.
 *
 * Revision 2.64  2005/09/21 19:49:26  dsandras
 * Added a function that returns the transport address where to send responses to incoming requests according to RFC3261 and RFC3581.
 *
 * Revision 2.63  2005/09/20 16:59:32  dsandras
 * Added method that adjusts the VIA field of incoming requests accordingly to the SIP RFC and RFC 3581 if the transport address/port do not correspond to what is specified in the Via. Thanks Ted Szoczei for the feedback.
 *
 * Revision 2.62  2005/09/15 16:59:36  dsandras
 * Add the video SDP part as soon as we can send or receive video.
 *
 * Revision 2.61  2005/09/06 06:42:16  csoutheren
 * Fix for bug #1282276
 * Changed to use "sip:" instead of "sip" when looking for SIP URLs
 *
 * Revision 2.60  2005/08/25 18:51:43  dsandras
 * Fixed bug. Added support for video in the SDP of INVITE's.
 *
 * Revision 2.59  2005/08/10 21:09:34  dsandras
 * Fixed previous commit.
 *
 * Revision 2.58  2005/08/10 19:34:34  dsandras
 * Added helper functions to get and set values of parameters in PDU fields.
 *
 * Revision 2.57  2005/08/04 17:11:20  dsandras
 * Added support for rport in the Via field.
 *
 * Revision 2.56  2005/07/15 18:03:02  dsandras
 * Use the original URI in the ACK request if no contact field is present.
 *
 * Revision 2.55  2005/06/04 12:44:36  dsandras
 * Applied patch from Ted Szoczei to fix leaks and problems on cancelling a call and to improve the Allow PDU field handling.
 *
 * Revision 2.54  2005/05/23 20:14:04  dsandras
 * Added preliminary support for basic instant messenging.
 *
 * Revision 2.53  2005/05/06 07:37:06  csoutheren
 * Various changed while working with SIP carrier
 *   - remove assumption that authentication realm is a domain name.
 *   - stopped rewrite of "To" field when proxy being used
 *   - fix Contact field in REGISTER to match actual port used when Symmetric NATin use
 *   - lots of formatting changes and cleanups
 *
 * Revision 2.52  2005/05/02 20:12:32  dsandras
 * Use the first listener port as signaling port in the Contact field for REGISTER PDU's.
 *
 * Revision 2.51  2005/04/28 20:22:55  dsandras
 * Applied big sanity patch for SIP thanks to Ted Szoczei <tszoczei@microtronix.ca>.
 * Thanks a lot!
 *
 * Revision 2.50  2005/04/28 07:59:37  dsandras
 * Applied patch from Ted Szoczei to fix problem when answering to PDUs containing
 * multiple Via fields in the message header. Thanks!
 *
 * Revision 2.49  2005/04/15 14:01:39  dsandras
 * Added User Agent string in REGISTER and SUBSCRIBE PDUs.
 *
 * Revision 2.48  2005/04/15 10:48:34  dsandras
 * Allow reading on the transport until there is an EOF or it becomes bad. Fixes interoperability problem with QSC.DE which is sending keep-alive messages, leading to a timeout (transport.good() fails, but the stream is still usable).
 *
 * Revision 2.47  2005/04/11 10:38:14  dsandras
 * Added support for INVITE done with the same RTP Session for call HOLD.
 *
 * Revision 2.46  2005/04/11 10:37:14  dsandras
 * Added support for the MESSAGE PDU.
 *
 * Revision 2.45  2005/04/11 10:36:34  dsandras
 * Added support for REFER and its associated NOTIFY for blind transfer.
 *
 * Revision 2.44  2005/03/11 18:12:09  dsandras
 * Added support to specify the realm when registering. That way softphones already know what authentication information to use when required. The realm/domain can also be used in the From field.
 *
 * Revision 2.43  2005/02/19 22:48:48  dsandras
 * Added the possibility to register to several registrars and be able to do authenticated calls to each of them. Added SUBSCRIBE/NOTIFY support for Message Waiting Indications.
 *
 * Revision 2.42  2005/02/19 22:36:25  dsandras
 * Always send PDU's to the proxy when there is one.
 *
 * Revision 2.41  2005/01/16 11:28:06  csoutheren
 * Added GetIdentifier virtual function to OpalConnection, and changed H323
 * and SIP descendants to use this function. This allows an application to
 * obtain a GUID for any connection regardless of the protocol used
 *
 * Revision 2.40  2004/12/27 22:19:27  dsandras
 * Added Allow field to PDUs.
 *
 * Revision 2.39  2004/12/22 18:57:50  dsandras
 * Added support for Call Forwarding via the "302 Moved Temporarily" SIP response.
 *
 * Revision 2.38  2004/12/17 12:06:53  dsandras
 * Added error code to OnRegistrationFailed. Made Register/Unregister wait until the transaction is over. Fixed Unregister so that the SIPRegister is used as a pointer or the object is deleted at the end of the function and make Opal crash when transactions are cleaned. Reverted part of the patch that was sending authentication again when it had already been done on a Register.
 *
 * Revision 2.37  2004/12/12 13:44:38  dsandras
 * - Modified InternalParse so that the remote displayName defaults to the sip url when none is provided.
 * - Changed GetDisplayName accordingly.
 * - Added call to OnRegistrationFailed when the REGISTER fails for any reason.
 *
 * Revision 2.36  2004/11/29 06:53:25  csoutheren
 * Prevent attempt to read infinite size
 * buffer if no ContentLength specified in MIME
 *
 * Revision 2.35  2004/11/08 10:17:51  rjongbloed
 * Tidied some trace logs
 *
 * Revision 2.34  2004/10/25 23:28:28  csoutheren
 * Fixed problems with systems that use commas between authentication parameters
 *
 * Revision 2.33  2004/09/27 12:51:20  rjongbloed
 * Fixed deadlock in SIP transaction timeout
 *
 * Revision 2.32  2004/08/22 12:27:46  rjongbloed
 * More work on SIP registration, time to live refresh and deregistration on exit.
 *
 * Revision 2.31  2004/08/18 13:05:49  rjongbloed
 * Fixed indicating transaction finished before it really is. Possible crash if then delete object.
 *
 * Revision 2.30  2004/08/14 07:56:43  rjongbloed
 * Major revision to utilise the PSafeCollection classes for the connections and calls.
 *
 * Revision 2.29  2004/04/25 09:32:15  rjongbloed
 * Fixed incorrect read of zero length SIP body, thanks Nick Hoath
 *
 * Revision 2.28  2004/03/23 09:43:42  rjongbloed
 * Fixed new C++ stream I/O compatibility, thanks Ted Szoczei
 *
 * Revision 2.27  2004/03/20 09:11:52  rjongbloed
 * Fixed probelm if inital read of stream fr SIP PDU fails. Should not then read again using
 *   >> operator as this then blocks the write due to the ios built in mutex.
 * Added timeout for "inter-packet" characters received. Waits forever for the first byte of
 *   a SIP PDU, then only waits a short time for the rest. This helps with getting a deadlock
 *   if a remote fails to send a full PDU.
 *
 * Revision 2.26  2004/03/16 12:06:11  rjongbloed
 * Changed SIP command URI to always be same as "to" address, not sure if this is correct though.
 *
 * Revision 2.25  2004/03/14 10:14:13  rjongbloed
 * Changes to REGISTER to support authentication
 *
 * Revision 2.24  2004/03/14 08:34:10  csoutheren
 * Added ability to set User-Agent string
 *
 * Revision 2.23  2004/03/13 06:32:18  rjongbloed
 * Fixes for removal of SIP and H.323 subsystems.
 * More registration work.
 *
 * Revision 2.22  2004/03/09 12:09:56  rjongbloed
 * More work on SIP register.
 *
 * Revision 2.21  2004/02/24 11:35:25  rjongbloed
 * Bullet proofed reply parsing for if get a command we don't understand.
 *
 * Revision 2.20  2003/12/16 10:22:45  rjongbloed
 * Applied enhancements, thank you very much Ted Szoczei
 *
 * Revision 2.19  2003/12/15 11:56:17  rjongbloed
 * Applied numerous bug fixes, thank you very much Ted Szoczei
 *
 * Revision 2.18  2003/03/19 00:47:06  robertj
 * GNU 3.2 changes
 *
 * Revision 2.17  2002/09/12 06:58:34  robertj
 * Removed protocol prefix strings as static members as has problems with
 *   use in DLL environment.
 *
 * Revision 2.16  2002/07/08 12:48:54  craigs
 * Do not set Record-Route if it is empty.
 *    Thanks to "Babara" <openh323@objectcrafts.org>
 *
 * Revision 2.15  2002/04/18 02:49:20  robertj
 * Fixed checking the correct state when overwriting terminated transactions.
 *
 * Revision 2.14  2002/04/17 07:24:12  robertj
 * Stopped complteion timer if transaction terminated.
 * Fixed multiple terminations so only the first version is used.
 *
 * Revision 2.13  2002/04/16 09:05:39  robertj
 * Fixed correct Route field setting depending on target URI.
 * Fixed some GNU warnings.
 *
 * Revision 2.12  2002/04/16 07:53:15  robertj
 * Changes to support calls through proxies.
 *
 * Revision 2.11  2002/04/15 08:54:46  robertj
 * Fixed setting correct local UDP port on cancelling INVITE.
 *
 * Revision 2.10  2002/04/12 12:22:45  robertj
 * Allowed for endpoint listener that is not on port 5060.
 *
 * Revision 2.9  2002/04/10 08:12:52  robertj
 * Added call back for when transaction completed, used for invite descendant.
 *
 * Revision 2.8  2002/04/10 03:16:23  robertj
 * Major changes to RTP session management when initiating an INVITE.
 * Improvements in error handling and transaction cancelling.
 *
 * Revision 2.7  2002/04/09 01:02:14  robertj
 * Fixed problems with restarting INVITE on  authentication required response.
 *
 * Revision 2.6  2002/04/05 10:42:04  robertj
 * Major changes to support transactions (UDP timeouts and retries).
 *
 * Revision 2.5  2002/03/18 08:09:31  robertj
 * Changed to use new SetXXX functions in PURL in normalisation.
 *
 * Revision 2.4  2002/03/08 06:28:03  craigs
 * Changed to allow Authorisation to be included in other PDUs
 *
 * Revision 2.3  2002/02/13 02:32:00  robertj
 * Fixed use of correct Decode function and error detection on parsing SDP.
 *
 * Revision 2.2  2002/02/11 07:36:23  robertj
 * Changed SDP to use OpalTransport for hosts instead of IP addresses/ports
 * Added media bypass for streams between compatible protocols.
 *
 * Revision 2.1  2002/02/01 04:53:01  robertj
 * Added (very primitive!) SIP support.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "sippdu.h"
#endif


#include <sip/sippdu.h>

#include <sip/sipep.h>
#include <sip/sipcon.h>
#include <opal/call.h>
#include <opal/manager.h>
#include <opal/connection.h>
#include <opal/transports.h>

#include <ptclib/cypher.h>
#include <ptclib/pdns.h>


#define  SIP_VER_MAJOR  2
#define  SIP_VER_MINOR  0


#define new PNEW


////////////////////////////////////////////////////////////////////////////

static const char * const MethodNames[SIP_PDU::NumMethods] = {
  "INVITE",
  "ACK",
  "OPTIONS",
  "BYE",
  "CANCEL",
  "REGISTER",
  "SUBSCRIBE",
  "NOTIFY",
  "REFER",
  "MESSAGE",
  "INFO",
  "PING",
  "PUBLISH"
};

const char * SIP_PDU::GetStatusCodeDescription (int code)
{
  static struct {
    int code;
    const char * desc;
  } sipErrorDescriptions[] = {
    { SIP_PDU::Information_Trying,                  "Trying" },
    { SIP_PDU::Information_Ringing,                 "Ringing" },
    { SIP_PDU::Information_CallForwarded,           "Call Forwarded" },
    { SIP_PDU::Information_Queued,                  "Queued" },
    { SIP_PDU::Information_Session_Progress,        "Progress" },

    { SIP_PDU::Successful_OK,                       "OK" },
    { SIP_PDU::Successful_Accepted,                 "Accepted" },

    { SIP_PDU::Redirection_MultipleChoices,         "Multiple Choices" },
    { SIP_PDU::Redirection_MovedPermanently,        "Moved Permanently" },
    { SIP_PDU::Redirection_MovedTemporarily,        "Moved Temporarily" },
    { SIP_PDU::Redirection_UseProxy,                "Use Proxy" },
    { SIP_PDU::Redirection_AlternativeService,      "Alternative Service" },

    { SIP_PDU::Failure_BadRequest,                  "BadRequest" },
    { SIP_PDU::Failure_UnAuthorised,                "Unauthorised" },
    { SIP_PDU::Failure_PaymentRequired,             "Payment Required" },
    { SIP_PDU::Failure_Forbidden,                   "Forbidden" },
    { SIP_PDU::Failure_NotFound,                    "Not Found" },
    { SIP_PDU::Failure_MethodNotAllowed,            "Method Not Allowed" },
    { SIP_PDU::Failure_NotAcceptable,               "Not Acceptable" },
    { SIP_PDU::Failure_ProxyAuthenticationRequired, "Proxy Authentication Required" },
    { SIP_PDU::Failure_RequestTimeout,              "Request Timeout" },
    { SIP_PDU::Failure_Conflict,                    "Conflict" },
    { SIP_PDU::Failure_Gone,                        "Gone" },
    { SIP_PDU::Failure_LengthRequired,              "Length Required" },
    { SIP_PDU::Failure_RequestEntityTooLarge,       "Request Entity Too Large" },
    { SIP_PDU::Failure_RequestURITooLong,           "Request URI Too Long" },
    { SIP_PDU::Failure_UnsupportedMediaType,        "Unsupported Media Type" },
    { SIP_PDU::Failure_UnsupportedURIScheme,        "Unsupported URI Scheme" },
    { SIP_PDU::Failure_BadExtension,                "Bad Extension" },
    { SIP_PDU::Failure_ExtensionRequired,           "Extension Required" },
    { SIP_PDU::Failure_IntervalTooBrief,            "Interval Too Brief" },
    { SIP_PDU::Failure_TemporarilyUnavailable,      "Temporarily Unavailable" },
    { SIP_PDU::Failure_TransactionDoesNotExist,     "Call Leg/Transaction Does Not Exist" },
    { SIP_PDU::Failure_LoopDetected,                "Loop Detected" },
    { SIP_PDU::Failure_TooManyHops,                 "Too Many Hops" },
    { SIP_PDU::Failure_AddressIncomplete,           "Address Incomplete" },
    { SIP_PDU::Failure_Ambiguous,                   "Ambiguous" },
    { SIP_PDU::Failure_BusyHere,                    "Busy Here" },
    { SIP_PDU::Failure_RequestTerminated,           "Request Terminated" },
    { SIP_PDU::Failure_NotAcceptableHere,           "Not Acceptable Here" },
    { SIP_PDU::Failure_BadEvent,                    "Bad Event" },
    { SIP_PDU::Failure_RequestPending,              "Request Pending" },
    { SIP_PDU::Failure_Undecipherable,              "Undecipherable" },

    { SIP_PDU::Failure_InternalServerError,         "Internal Server Error" },
    { SIP_PDU::Failure_NotImplemented,              "Not Implemented" },
    { SIP_PDU::Failure_BadGateway,                  "Bad Gateway" },
    { SIP_PDU::Failure_ServiceUnavailable,          "Service Unavailable" },
    { SIP_PDU::Failure_ServerTimeout,               "Server Time-out" },
    { SIP_PDU::Failure_SIPVersionNotSupported,      "SIP Version Not Supported" },
    { SIP_PDU::Failure_MessageTooLarge,             "Message Too Large" },

    { SIP_PDU::GlobalFailure_BusyEverywhere,        "Busy Everywhere" },
    { SIP_PDU::GlobalFailure_Decline,               "Decline" },
    { SIP_PDU::GlobalFailure_DoesNotExistAnywhere,  "Does Not Exist Anywhere" },
    { SIP_PDU::GlobalFailure_NotAcceptable,         "Not Acceptable" },

    { 0 }
  };

  for (PINDEX i = 0; sipErrorDescriptions[i].code != 0; i++) {
    if (sipErrorDescriptions[i].code == code)
      return sipErrorDescriptions[i].desc;
  }
  return "";
}


static const char * const AlgorithmNames[SIPAuthentication::NumAlgorithms] = {
  "MD5"
};


/////////////////////////////////////////////////////////////////////////////

SIPURL::SIPURL()
{
}


SIPURL::SIPURL(const char * str, const char * defaultScheme)
{
  Parse(str, defaultScheme);
}


SIPURL::SIPURL(const PString & str, const char * defaultScheme)
{
  Parse(str, defaultScheme);
}


SIPURL::SIPURL(const PString & name,
               const OpalTransportAddress & address,
               WORD listenerPort)
{
  if (strncmp(name, "sip:", 4) == 0)
    Parse(name);
  else {
    PIPSocket::Address ip;
    WORD port;
    if (address.GetIpAndPort(ip, port)) {
      PStringStream s;
      s << "sip:" << name << '@';
      if (ip.GetVersion() == 6)
        s << '[' << ip << ']';
      else
        s << ip;
      s << ':';
      if (listenerPort != 0)
        s << listenerPort;
      else
        s << port;
      s << ";transport=";
      if (strncmp(address, "tcp", 3) == 0)
        s << "tcp";
      else
        s << "udp";
      Parse(s);
    }
  }
}


BOOL SIPURL::InternalParse(const char * cstr, const char * defaultScheme)
{
  if (defaultScheme == NULL)
    defaultScheme = "sip";

  displayName = PString::Empty();

  PString str = cstr;

  // see if URL is just a URI or it contains a display address as well
  PINDEX start = str.FindLast('<');
  PINDEX end = str.FindLast('>');

  // see if URL is just a URI or it contains a display address as well
  if (start == P_MAX_INDEX || end == P_MAX_INDEX) {
    if (!PURL::InternalParse(cstr, defaultScheme)) {
      return FALSE;
    }
  }
  else {
    // get the URI from between the angle brackets
    if (!PURL::InternalParse(str(start+1, end-1), defaultScheme))
      return FALSE;

    // extract the display address
    end = str.FindLast('"', start);
    start = str.FindLast('"', end-1);
    // There are no double quotes around the display name
    if (start == P_MAX_INDEX && end == P_MAX_INDEX) {
      
      displayName = str.Left(start-1).Trim();
      start = str.FindLast ('<');
      
      // See if there is something before the <
      if (start != P_MAX_INDEX && start > 0)
        displayName = str.Left(start).Trim();
      else { // Use the url as display name
        end = str.FindLast('>');
        if (end != P_MAX_INDEX)
          str = displayName.Mid ((start == P_MAX_INDEX) ? 0:start+1, end-1);

        /* Remove the tag from the display name, if any */
        end = str.Find (';');
        if (end != P_MAX_INDEX)
          str = str.Left (end);

        displayName = str;
        displayName.Replace  ("sip:", "");
      }
    }
    else if (start != P_MAX_INDEX && end != P_MAX_INDEX) {
      // No trim quotes off
      displayName = str(start+1, end-1);
      while ((start = displayName.Find('\\')) != P_MAX_INDEX)
        displayName.Delete(start, 1);
    }
  }

  if (!(scheme *= defaultScheme))
    return PURL::Parse("");

//  if (!paramVars.Contains("transport"))
//    SetParamVar("transport", "udp");

  Recalculate();
  return !IsEmpty();
}


PString SIPURL::AsQuotedString() const
{
  PStringStream s;

  if (!displayName)
    s << '"' << displayName << "\" ";
  s << '<' << AsString() << '>';

  return s;
}


PString SIPURL::GetDisplayName (BOOL useDefault) const
{
  PString s;
  PINDEX tag;
    
  s = displayName;

  if (displayName.IsEmpty () && useDefault) {

    s = AsString ();
    s.Replace ("sip:", "");

    /* There could be a tag if we are using the URL,
     * remove it
     */
    tag = s.Find (';');
    if (tag != P_MAX_INDEX)
      s = s.Left (tag);
  }

  return s;
}


OpalTransportAddress SIPURL::GetHostAddress() const
{
  PString addr = paramVars("transport", "udp") + '$';

  if (paramVars.Contains("maddr"))
    addr += paramVars["maddr"];
  else
    addr += hostname;

  if (port > 0)
    addr.sprintf(":%u", port);

  return addr;
}


void SIPURL::AdjustForRequestURI()
{
  paramVars.RemoveAt("tag");
  paramVars.RemoveAt("transport");
  queryVars.RemoveAll();
  Recalculate();
}


#if P_DNS
BOOL SIPURL::AdjustToDNS(PINDEX entry)
{
  // RFC3263 states we do not do lookup if explicit port mentioned
  if (GetPortSupplied())
    return TRUE;

  // Or it is a valid IP address, not a domain name
  PIPSocket::Address ip = GetHostName();
  if (ip.IsValid())
    return TRUE;

  // Do the SRV lookup, if fails, then we actually return TRUE so outer loops
  // can use the original host name value.
  PIPSocketAddressAndPortVector addrs;
  if (!PDNS::LookupSRV(GetHostName(), "_sip._" + paramVars("transport", "udp"), GetPort(), addrs))
    return TRUE;

  // Got the SRV list, return FALSE if outer loop has got to the end of it
  if (entry >= (PINDEX)addrs.size())
    return FALSE;

  // Adjust our host and port to what the DNS SRV record says
  SetHostName(addrs[entry].address.AsString());
  SetPort(addrs[entry].port);
  return TRUE;
}
#else
BOOL SIPURL::AdjustToDNS(PINDEX)
{
  return TRUE;
}
#endif


/////////////////////////////////////////////////////////////////////////////

SIPMIMEInfo::SIPMIMEInfo(BOOL _compactForm)
  : compactForm(_compactForm)
{
}


PINDEX SIPMIMEInfo::GetContentLength() const
{
  PString len = GetFullOrCompact("Content-Length", 'l');
  if (len.IsEmpty())
    return 0; //P_MAX_INDEX;
  return len.AsInteger();
}

BOOL SIPMIMEInfo::IsContentLengthPresent() const
{
  return !GetFullOrCompact("Content-Length", 'l').IsEmpty();
}


PString SIPMIMEInfo::GetContentType() const
{
  return GetFullOrCompact("Content-Type", 'c');
}


void SIPMIMEInfo::SetContentType(const PString & v)
{
  SetAt(compactForm ? "c" : "Content-Type",  v);
}


PString SIPMIMEInfo::GetContentEncoding() const
{
  return GetFullOrCompact("Content-Encoding", 'e');
}


void SIPMIMEInfo::SetContentEncoding(const PString & v)
{
  SetAt(compactForm ? "e" : "Content-Encoding",  v);
}


PString SIPMIMEInfo::GetFrom() const
{
  return GetFullOrCompact("From", 'f');
}


void SIPMIMEInfo::SetFrom(const PString & v)
{
  SetAt(compactForm ? "f" : "From",  v);
}


PString SIPMIMEInfo::GetCallID() const
{
  return GetFullOrCompact("Call-ID", 'i');
}


void SIPMIMEInfo::SetCallID(const PString & v)
{
  SetAt(compactForm ? "i" : "Call-ID",  v);
}


PString SIPMIMEInfo::GetContact() const
{
  return GetFullOrCompact("Contact", 'm');
}


void SIPMIMEInfo::SetContact(const PString & v)
{
  SetAt(compactForm ? "m" : "Contact",  v);
}


void SIPMIMEInfo::SetContact(const SIPURL & url)
{
  SetContact(url.AsQuotedString());
}


PString SIPMIMEInfo::GetSubject() const
{
  return GetFullOrCompact("Subject", 's');
}


void SIPMIMEInfo::SetSubject(const PString & v)
{
  SetAt(compactForm ? "s" : "Subject",  v);
}


PString SIPMIMEInfo::GetTo() const
{
  return GetFullOrCompact("To", 't');
}


void SIPMIMEInfo::SetTo(const PString & v)
{
  SetAt(compactForm ? "t" : "To",  v);
}


PString SIPMIMEInfo::GetVia() const
{
  return GetFullOrCompact("Via", 'v');
}


void SIPMIMEInfo::SetVia(const PString & v)
{
  SetAt(compactForm ? "v" : "Via",  v);
}


PStringList SIPMIMEInfo::GetViaList() const
{
  PStringList viaList;
  PString s = GetVia();
  if (s.FindOneOf("\r\n") != P_MAX_INDEX)
    viaList = s.Lines();
  else
    viaList = s.Tokenise(",", FALSE);

  return viaList;
}


void SIPMIMEInfo::SetViaList(const PStringList & v)
{
  PString fieldValue;
  if (v.GetSize() > 0)
  {
    fieldValue = v[0];
    for (PINDEX i = 1; i < v.GetSize(); i++)
      fieldValue += '\n' + v[i];
  }
  SetAt(compactForm ? "v" : "Via", fieldValue);
}


PString SIPMIMEInfo::GetReferTo() const
{
  return GetFullOrCompact("Refer-To", 'r');
}


void SIPMIMEInfo::SetReferTo(const PString & r)
{
  SetAt(compactForm ? "r" : "Refer-To",  r);
}

PString SIPMIMEInfo::GetReferredBy() const
{
  return GetFullOrCompact("Referred-By", 'b');
}


void SIPMIMEInfo::SetReferredBy(const PString & r)
{
  SetAt(compactForm ? "b" : "Referred-By",  r);
}

void SIPMIMEInfo::SetContentLength(PINDEX v)
{
  SetAt(compactForm ? "l" : "Content-Length", PString(PString::Unsigned, v));
}


PString SIPMIMEInfo::GetCSeq() const
{
  return (*this)("CSeq");       // no compact form
}


void SIPMIMEInfo::SetCSeq(const PString & v)
{
  SetAt("CSeq",  v);            // no compact form
}


PStringList SIPMIMEInfo::GetRoute() const
{
  return GetRouteList("Route");
}


void SIPMIMEInfo::SetRoute(const PStringList & v)
{
  SetRouteList("Route",  v);
}


PStringList SIPMIMEInfo::GetRecordRoute() const
{
  return GetRouteList("Record-Route");
}


void SIPMIMEInfo::SetRecordRoute(const PStringList & v)
{
  if (!v.IsEmpty())
    SetRouteList("Record-Route",  v);
}


PStringList SIPMIMEInfo::GetRouteList(const char * name) const
{
  PStringList routeSet;

  PString s = (*this)(name);
  PINDEX left;
  PINDEX right = 0;
  while ((left = s.Find('<', right)) != P_MAX_INDEX &&
         (right = s.Find('>', left)) != P_MAX_INDEX &&
         (right - left) > 5)
    routeSet.AppendString(s(left+1, right-1));

  return routeSet;
}


void SIPMIMEInfo::SetRouteList(const char * name, const PStringList & v)
{
  PStringStream s;

  for (PINDEX i = 0; i < v.GetSize(); i++) {
    if (i > 0)
      s << ',';
    s << '<' << v[i] << '>';
  }

  SetAt(name,  s);
}


PString SIPMIMEInfo::GetAccept() const
{
  return (*this)(PCaselessString("Accept"));    // no compact form
}


void SIPMIMEInfo::SetAccept(const PString & v)
{
  SetAt("Accept", v);  // no compact form
}


PString SIPMIMEInfo::GetAcceptEncoding() const
{
  return (*this)(PCaselessString("Accept-Encoding"));   // no compact form
}


void SIPMIMEInfo::SetAcceptEncoding(const PString & v)
{
  SetAt("Accept-Encoding", v); // no compact form
}


PString SIPMIMEInfo::GetAcceptLanguage() const
{
  return (*this)(PCaselessString("Accept-Language"));   // no compact form
}


void SIPMIMEInfo::SetAcceptLanguage(const PString & v)
{
  SetAt("Accept-Language", v); // no compact form
}


PString SIPMIMEInfo::GetAllow() const
{
  return (*this)(PCaselessString("Allow"));     // no compact form
}


void SIPMIMEInfo::SetAllow(const PString & v)
{
  SetAt("Allow", v);   // no compact form
}



PString SIPMIMEInfo::GetDate() const
{
  return (*this)(PCaselessString("Date"));      // no compact form
}


void SIPMIMEInfo::SetDate(const PString & v)
{
  SetAt("Date", v);    // no compact form
}


void SIPMIMEInfo::SetDate(const PTime & t)
{
  SetDate(t.AsString(PTime::RFC1123, PTime::GMT)) ;
}


void SIPMIMEInfo::SetDate(void) // set to current date
{
  SetDate(PTime()) ;
}

        
unsigned SIPMIMEInfo::GetExpires(unsigned dflt) const
{
  PString v = (*this)(PCaselessString("Expires"));      // no compact form
  if (v.IsEmpty())
    return dflt;

  return v.AsUnsigned();
}


void SIPMIMEInfo::SetExpires(unsigned v)
{
  SetAt("Expires", PString(PString::Unsigned, v));      // no compact form
}


PINDEX SIPMIMEInfo::GetMaxForwards() const
{
  PString len = (*this)(PCaselessString("Max-Forwards"));       // no compact form
  if (len.IsEmpty())
    return P_MAX_INDEX;
  return len.AsInteger();
}


void SIPMIMEInfo::SetMaxForwards(PINDEX v)
{
  SetAt("Max-Forwards", PString(PString::Unsigned, v)); // no compact form
}


PINDEX SIPMIMEInfo::GetMinExpires() const
{
  PString len = (*this)(PCaselessString("Min-Expires"));        // no compact form
  if (len.IsEmpty())
    return P_MAX_INDEX;
  return len.AsInteger();
}


void SIPMIMEInfo::SetMinExpires(PINDEX v)
{
  SetAt("Min-Expires",  PString(PString::Unsigned, v)); // no compact form
}


PString SIPMIMEInfo::GetProxyAuthenticate() const
{
  return (*this)(PCaselessString("Proxy-Authenticate"));        // no compact form
}


void SIPMIMEInfo::SetProxyAuthenticate(const PString & v)
{
  SetAt("Proxy-Authenticate",  v);      // no compact form
}


PString SIPMIMEInfo::GetSupported() const
{
  return GetFullOrCompact("Supported", 'k');
}

void SIPMIMEInfo::SetSupported(const PString & v)
{
  SetAt(compactForm ? "k" : "Supported",  v);
}


PString SIPMIMEInfo::GetUnsupported() const
{
  return (*this)(PCaselessString("Unsupported"));       // no compact form
}


void SIPMIMEInfo::SetUnsupported(const PString & v)
{
  SetAt("Unsupported",  v);     // no compact form
}


PString SIPMIMEInfo::GetEvent() const
{
  return GetFullOrCompact("Event", 'o');
}


void SIPMIMEInfo::SetEvent(const PString & v)
{
  SetAt(compactForm ? "o" : "Event",  v);
}


PString SIPMIMEInfo::GetSubscriptionState() const
{
  return (*this)(PCaselessString("Subscription-State"));       // no compact form
}


void SIPMIMEInfo::SetSubscriptionState(const PString & v)
{
  SetAt("Subscription-State",  v);     // no compact form
}


PString SIPMIMEInfo::GetUserAgent() const
{
  return (*this)(PCaselessString("User-Agent"));        // no compact form
}


void SIPMIMEInfo::SetUserAgent(const PString & v)
{
  SetAt("User-Agent",  v);     // no compact form
}


PString SIPMIMEInfo::GetOrganization() const
{
  return (*this)(PCaselessString("Organization"));        // no compact form
}


void SIPMIMEInfo::SetOrganization(const PString & v)
{
  SetAt("Organization",  v);     // no compact form
}


static const char UserAgentTokenChars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-.!%*_+`'~";

void SIPMIMEInfo::GetProductInfo(OpalProductInfo & info)
{
  PCaselessString str = GetUserAgent();
  if (str.IsEmpty()) {
    str = (*this)("Server");
    if (str.IsEmpty())
      return; // Have nothing, change nothing
  }

  // This is not strictly correct according to he BNF, but we cheat
  // and assume that the prod/ver tokens are first and if there is an
  // explicit comment field, it is always last. If any other prod/ver
  // tokens are present, then they will end up as the comments too.
  // All other variations just end up as one big comment

  PINDEX endFirstToken = str.FindSpan(UserAgentTokenChars);
  if (endFirstToken == 0) {
    info.name = str;
    info.vendor = info.version = PString::Empty();
    info.manufacturerCode = info.t35Extension = info.t35CountryCode = 0;
    return;
  }

  PINDEX endSecondToken = endFirstToken;
  if (endFirstToken != P_MAX_INDEX && str[endFirstToken] == '/')
    endSecondToken = str.FindSpan(UserAgentTokenChars, endFirstToken+1);

  info.name = str.Left(endFirstToken);
  info.version = str(endFirstToken+1, endSecondToken);
  info.vendor = GetOrganization();
}


void SIPMIMEInfo::SetProductInfo(const PString & ua, const OpalProductInfo & info)
{
  PString userAgent = ua;
  if (userAgent.IsEmpty()) {
    PINDEX pos;
    PCaselessString temp = info.name;
    temp.Replace(' ', '-', TRUE);
    while ((pos = temp.FindSpan(UserAgentTokenChars)) != P_MAX_INDEX)
      temp.Delete(pos, 1);
    if (!temp.IsEmpty()) {
      userAgent = temp;

      temp = info.version;
      temp.Replace(' ', '-', TRUE);
      while ((pos = temp.FindSpan(UserAgentTokenChars)) != P_MAX_INDEX)
        temp.Delete(pos, 1);
      if (!temp.IsEmpty())
        userAgent += '/' + temp;
    }
  }

  if (!userAgent.IsEmpty())
    SetUserAgent(userAgent);      // no compact form

  if (!info.vendor.IsEmpty())
    SetOrganization(info.vendor);      // no compact form
}


PString SIPMIMEInfo::GetWWWAuthenticate() const
{
  return (*this)(PCaselessString("WWW-Authenticate"));  // no compact form
}


void SIPMIMEInfo::SetWWWAuthenticate(const PString & v)
{
  SetAt("WWW-Authenticate",  v);        // no compact form
}

PString SIPMIMEInfo::GetSIPIfMatch() const
{
  return (*this)(PCaselessString("SIP-If-Match"));  // no compact form
}

void SIPMIMEInfo::SetSIPIfMatch(const PString & v)
{
  SetAt("SIP-If-Match",  v);        // no compact form
}

PString SIPMIMEInfo::GetSIPETag() const
{
  return (*this)(PCaselessString("SIP-ETag"));  // no compact form
}

void SIPMIMEInfo::SetSIPETag(const PString & v)
{
  SetAt("SIP-ETag",  v);        // no compact form
}

void SIPMIMEInfo::SetFieldParameter(const PString & param,
                                          PString & field,
                                    const PString & value)
{
  PStringStream s;
  
  PCaselessString val = field;
  
  if (HasFieldParameter(param, field)) {

    val = GetFieldParameter(param, field);
    
    if (!val.IsEmpty()) // The parameter already has a value, replace it.
      field.Replace(val, value);
    else { // The parameter has no value
     
      s << param << "=" << value;
      field.Replace(param, s);
    }
  }
  else { // There is no such parameter

    s << field << ";" << param << "=" << value;
    field = s;
  }
}


PString SIPMIMEInfo::GetFieldParameter(const PString & param,
                                       const PString & field)
{
  PINDEX j = 0;
  
  PCaselessString val = field;
  if ((j = val.FindLast (param)) != P_MAX_INDEX) {

    val = val.Mid(j+param.GetLength());
    if ((j = val.Find (';')) != P_MAX_INDEX)
      val = val.Left(j);

    if ((j = val.Find (' ')) != P_MAX_INDEX)
      val = val.Left(j);
    
    if ((j = val.Find (',')) != P_MAX_INDEX)
      val = val.Left(j);
    
    if ((j = val.Find ('=')) != P_MAX_INDEX) 
      val = val.Mid(j+1);
    else
      val = "";
  }
  else
    val = "";

  return val;
}


BOOL SIPMIMEInfo::HasFieldParameter(const PString & param, const PString & field)
{
  PCaselessString val = field;
  
  return (val.Find(param) != P_MAX_INDEX);
}


PString SIPMIMEInfo::GetFullOrCompact(const char * fullForm, char compactForm) const
{
  if (Contains(PCaselessString(fullForm)))
    return (*this)[fullForm];
  return (*this)(PCaselessString(compactForm));
}


////////////////////////////////////////////////////////////////////////////////////

SIPAuthentication::SIPAuthentication(const PString & user, const PString & pwd)
  : username(user), password(pwd)
{
  algorithm = NumAlgorithms;
  isProxy = FALSE;
}


static PString GetAuthParam(const PString & auth, const char * name)
{
  PString value;

  PINDEX pos = auth.Find(name);
  if (pos != P_MAX_INDEX)  {
    pos += strlen(name);
    while (isspace(auth[pos]) || (auth[pos] == ','))
      pos++;
    if (auth[pos] == '=') {
      pos++;
      while (isspace(auth[pos]))
        pos++;
      if (auth[pos] == '"') {
        pos++;
        value = auth(pos, auth.Find('"', pos)-1);
      }
      else {
        PINDEX base = pos;
        while (auth[pos] != '\0' && !isspace(auth[pos]) && (auth[pos] != ','))
          pos++;
        value = auth(base, pos-1);
      }
    }
  }

  return value;
}


BOOL SIPAuthentication::Parse(const PCaselessString & auth, BOOL proxy)
{
  authRealm.MakeEmpty();
  nonce.MakeEmpty();
  opaque.MakeEmpty();
  algorithm = NumAlgorithms;

  qopAuth = qopAuthInt = FALSE;
  cnonce.MakeEmpty();
  nonceCount.SetValue(1);

  if (auth.Find("digest") != 0) {
    PTRACE(1, "SIP\tUnknown authentication type");
    return FALSE;
  }

  PCaselessString str = GetAuthParam(auth, "algorithm");
  if (str.IsEmpty())
    algorithm = Algorithm_MD5;  // default
  else if (str == "md5")
    algorithm = Algorithm_MD5;
  else {
    PTRACE(1, "SIP\tUnknown authentication algorithm");
    return FALSE;
  }

  authRealm = GetAuthParam(auth, "realm");
  if (authRealm.IsEmpty()) {
    PTRACE(1, "SIP\tNo realm in authentication");
    return FALSE;
  }

  nonce = GetAuthParam(auth, "nonce");
  if (nonce.IsEmpty()) {
    PTRACE(1, "SIP\tNo nonce in authentication");
    return FALSE;
  }

  opaque = GetAuthParam(auth, "opaque");
  if (!opaque.IsEmpty()) {
    PTRACE(2, "SIP\tAuthentication contains opaque data");
  }

  PString qopStr = GetAuthParam(auth, "qop");
  if (!qopStr.IsEmpty()) {
    PTRACE(3, "SIP\tAuthentication contains qop-options " << qopStr);
    PStringList options = qopStr.Tokenise(',', TRUE);
    qopAuth    = options.GetStringsIndex("auth") != P_MAX_INDEX;
    qopAuthInt = options.GetStringsIndex("auth-int") != P_MAX_INDEX;
    cnonce = OpalGloballyUniqueID().AsString();
  }

  isProxy = proxy;
  return TRUE;
}


BOOL SIPAuthentication::IsValid() const
{
  return /*!authRealm && */ !username && !nonce && algorithm < NumAlgorithms;
}


static PString AsHex(PMessageDigest5::Code & digest)
{
  PStringStream out;
  out << hex << setfill('0');
  for (PINDEX i = 0; i < 16; i++)
    out << setw(2) << (unsigned)((BYTE *)&digest)[i];
  return out;
}


BOOL SIPAuthentication::Authorise(SIP_PDU & pdu) const
{
  if (!IsValid()) {
    PTRACE(2, "SIP\tNo authentication information present");
    return FALSE;
  }

  PTRACE(3, "SIP\tAdding authentication information");

  PMessageDigest5 digestor;
  PMessageDigest5::Code a1, a2, response;

  PString uriText = pdu.GetURI().AsString();
  PINDEX pos = uriText.Find(";");
  if (pos != P_MAX_INDEX)
    uriText = uriText.Left(pos);

  digestor.Start();
  digestor.Process(username);
  digestor.Process(":");
  digestor.Process(authRealm);
  digestor.Process(":");
  digestor.Process(password);
  digestor.Complete(a1);

  digestor.Start();
  digestor.Process(MethodNames[pdu.GetMethod()]);
  digestor.Process(":");
  digestor.Process(uriText);
  if (qopAuthInt) {
    digestor.Process(":");
    digestor.Process(pdu.GetEntityBody());
  }
  digestor.Complete(a2);

  PStringStream auth;
  auth << "Digest "
          "username=\"" << username << "\", "
          "realm=\"" << authRealm << "\", "
          "nonce=\"" << nonce << "\", "
          "uri=\"" << uriText << "\", "
          "algorithm=" << AlgorithmNames[algorithm];

  digestor.Start();
  digestor.Process(AsHex(a1));
  digestor.Process(":");
  digestor.Process(nonce);
  digestor.Process(":");

  if (qopAuthInt || qopAuth) {
    PString nc(psprintf("%08i", (unsigned int)nonceCount));
    ++nonceCount;
    PString qop;
    if (qopAuthInt)
      qop = "auth-int";
    else
      qop = "auth";
    digestor.Process(nc);
    digestor.Process(":");
    digestor.Process(cnonce);
    digestor.Process(":");
    digestor.Process(qop);
    digestor.Process(":");
    digestor.Process(AsHex(a2));
    digestor.Complete(response);
    auth << ", "
         << "response=\"" << AsHex(response) << "\", "
         << "cnonce=\"" << cnonce << "\", "
         << "nc=\"" << nc << "\", "
         << "qop=\"" << qop << "\"";
  }
  else {
    digestor.Process(AsHex(a2));
    digestor.Complete(response);
    auth << ", response=\"" << AsHex(response) << "\"";
  }

  if (!opaque.IsEmpty())
    auth << ", opaque=\"" << opaque << "\"";

  pdu.GetMIME().SetAt(isProxy ? "Proxy-Authorization" : "Authorization", auth);
  return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////

SIP_PDU::SIP_PDU()
{
  Construct(NumMethods);
}


SIP_PDU::SIP_PDU(Methods method,
                 const SIPURL & dest,
                 const PString & to,
                 const PString & from,
                 const PString & callID,
                 unsigned cseq,
                 const OpalTransportAddress & via)
{
  Construct(method, dest, to, from, callID, cseq, via);
}


SIP_PDU::SIP_PDU(Methods method,
                 SIPConnection & connection,
                 const OpalTransport & transport)
{
  Construct(method, connection, transport);
}


SIP_PDU::SIP_PDU(const SIP_PDU & request, 
                 StatusCodes code, 
                 const char * contact,
                 const char * extra)
{
  method       = NumMethods;
  statusCode   = code;
  versionMajor = request.GetVersionMajor();
  versionMinor = request.GetVersionMinor();
  sdp = NULL;

  // add mandatory fields to response (RFC 2543, 11.2)
  const SIPMIMEInfo & requestMIME = request.GetMIME();
  mime.SetTo(requestMIME.GetTo());
  mime.SetFrom(requestMIME.GetFrom());
  mime.SetCallID(requestMIME.GetCallID());
  mime.SetCSeq(requestMIME.GetCSeq());
  mime.SetVia(requestMIME.GetVia());
  mime.SetRecordRoute(requestMIME.GetRecordRoute());
  SetAllow();

  /* Use extra parameter as redirection URL in case of 302 */
  if (code == SIP_PDU::Redirection_MovedTemporarily) {
    SIPURL contact(extra);
    mime.SetContact(contact.AsQuotedString());
    extra = NULL;
  }
  else if (contact != NULL) {
    mime.SetContact(PString(contact));
  }
    
  // format response
  if (extra != NULL)
    info = extra;
  else
    info = GetStatusCodeDescription(code);
}


SIP_PDU::SIP_PDU(const SIP_PDU & pdu)
  : method(pdu.method),
    statusCode(pdu.statusCode),
    uri(pdu.uri),
    versionMajor(pdu.versionMajor),
    versionMinor(pdu.versionMinor),
    info(pdu.info),
    mime(pdu.mime),
    entityBody(pdu.entityBody)
{
  if (pdu.sdp != NULL)
    sdp = new SDPSessionDescription(*pdu.sdp);
  else
    sdp = NULL;
}


SIP_PDU & SIP_PDU::operator=(const SIP_PDU & pdu)
{
  method = pdu.method;
  statusCode = pdu.statusCode;
  uri = pdu.uri;
  versionMajor = pdu.versionMajor;
  versionMinor = pdu.versionMinor;
  info = pdu.info;
  mime = pdu.mime;
  entityBody = pdu.entityBody;

  delete sdp;
  if (pdu.sdp != NULL)
    sdp = new SDPSessionDescription(*pdu.sdp);
  else
    sdp = NULL;

  return *this;
}


SIP_PDU::~SIP_PDU()
{
  delete sdp;
}


void SIP_PDU::Construct(Methods meth)
{
  method = meth;
  statusCode = IllegalStatusCode;

  versionMajor = SIP_VER_MAJOR;
  versionMinor = SIP_VER_MINOR;

  sdp = NULL;
}


void SIP_PDU::Construct(Methods meth,
                        const SIPURL & dest,
                        const PString & to,
                        const PString & from,
                        const PString & callID,
                        unsigned cseq,
                        const OpalTransportAddress & via)
{
  PString allMethods;
  
  Construct(meth);

  uri = dest;
  uri.AdjustForRequestURI();

  mime.SetTo(to);
  mime.SetFrom(from);
  mime.SetCallID(callID);
  mime.SetCSeq(PString(cseq) & MethodNames[method]);
  mime.SetMaxForwards(70);  

  // construct Via:
  PINDEX dollar = via.Find('$');

  OpalGloballyUniqueID branch;
  PStringStream str;
  str << "SIP/" << versionMajor << '.' << versionMinor << '/'
      << via.Left(dollar).ToUpper() << ' ';
  PIPSocket::Address ip;
  WORD port;
  if (via.GetIpAndPort(ip, port))
    str << ip << ':' << port;
  else
    str << via.Mid(dollar+1);
  str << ";branch=z9hG4bK" << branch << ";rport";

  mime.SetVia(str);

  SetAllow();
}


void SIP_PDU::Construct(Methods meth,
                        SIPConnection & connection,
                        const OpalTransport & transport)
{
  PStringList routeSet = connection.GetRouteSet();
  SIPEndPoint & endpoint = connection.GetEndPoint();
  PString localPartyName = connection.GetLocalPartyName();
  SIPURL contact = endpoint.GetContactURL(transport, localPartyName, SIPURL(connection.GetRemotePartyAddress()).GetHostName());
  SIPURL via = endpoint.GetLocalURL(transport, localPartyName);
  mime.SetContact(contact);

  SIPURL targetAddress = connection.GetTargetAddress();
  targetAddress.AdjustForRequestURI(),

  Construct(meth,
            targetAddress,
            connection.GetRemotePartyAddress(),
            connection.GetExplicitFrom(),
            connection.GetToken(),
            connection.GetNextCSeq(),
            via.GetHostAddress());

  SetRoute(routeSet); // Possibly adjust the URI and the route
}


BOOL SIP_PDU::SetRoute(const PStringList & set)
{
  PStringList routeSet = set;

  if (routeSet.IsEmpty())
    return FALSE;

  SIPURL firstRoute = routeSet[0];
  if (!firstRoute.GetParamVars().Contains("lr")) {
    // this procedure is specified in RFC3261:12.2.1.1 for backwards compatibility with RFC2543
    routeSet.MakeUnique();
    routeSet.RemoveAt(0);
    routeSet.AppendString(uri.AsString());
    uri = firstRoute;
    uri.AdjustForRequestURI();
  }

  mime.SetRoute(routeSet);
  return TRUE;
}


void SIP_PDU::SetAllow(void)
{
  PStringStream str;
  PStringList methods;
  
  for (PINDEX i = 0 ; i < SIP_PDU::NumMethods ; i++) {
    PString method(MethodNames[i]);
    if (method.Find("SUBSCRIBE") == P_MAX_INDEX && method.Find("REGISTER") == P_MAX_INDEX)
      methods += method;
  }
  
  str << setfill(',') << methods << setfill(' ');

  mime.SetAllow(str);
}


void SIP_PDU::AdjustVia(OpalTransport & transport)
{
  // Update the VIA field following RFC3261, 18.2.1 and RFC3581
  PStringList viaList = mime.GetViaList();
  PString via = viaList[0];
  PString port, ip;
  PINDEX j = 0;
  
  if ((j = via.FindLast (' ')) != P_MAX_INDEX)
    via = via.Mid(j+1);
  if ((j = via.Find (';')) != P_MAX_INDEX)
    via = via.Left(j);
  if ((j = via.Find (':')) != P_MAX_INDEX) {

    ip = via.Left(j);
    port = via.Mid(j+1);
  }
  else
    ip = via;

  PIPSocket::Address a (ip);
  PIPSocket::Address remoteIp;
  WORD remotePort;
  if (transport.GetLastReceivedAddress().GetIpAndPort(remoteIp, remotePort)) {

    if (mime.HasFieldParameter("rport", viaList[0]) && mime.GetFieldParameter("rport", viaList[0]).IsEmpty()) {
      // fill in empty rport and received for RFC 3581
      mime.SetFieldParameter("rport", viaList[0], remotePort);
      mime.SetFieldParameter("received", viaList[0], remoteIp);
    }
    else if (remoteIp != a ) // set received when remote transport address different from Via address
      mime.SetFieldParameter("received", viaList[0], remoteIp);
  }
  else if (!a.IsValid()) {
    // Via address given has domain name
    mime.SetFieldParameter("received", viaList[0], via);
  }

  mime.SetViaList(viaList);
}


OpalTransportAddress SIP_PDU::GetViaAddress(OpalEndPoint &ep)
{
  PStringList viaList = mime.GetViaList();
  PString viaAddress = viaList[0];
  PString proto = viaList[0];
  PString viaPort = ep.GetDefaultSignalPort();

  PINDEX j = 0;
  // get the address specified in the Via
  if ((j = viaAddress.FindLast (' ')) != P_MAX_INDEX)
    viaAddress = viaAddress.Mid(j+1);
  if ((j = viaAddress.Find (';')) != P_MAX_INDEX)
    viaAddress = viaAddress.Left(j);
  if ((j = viaAddress.Find (':')) != P_MAX_INDEX) {
    viaPort = viaAddress.Mid(j+1);
    viaAddress = viaAddress.Left(j);
  }

  // get the protocol type from Via header
  if ((j = proto.FindLast (' ')) != P_MAX_INDEX)
    proto = proto.Left(j);
  if ((j = proto.FindLast('/')) != P_MAX_INDEX)
    proto = proto.Mid(j+1);

  // maddr is present, no support for multicast yet
  if (mime.HasFieldParameter("maddr", viaList[0])) 
    viaAddress = mime.GetFieldParameter("maddr", viaList[0]);
  // received and rport are present
  else if (mime.HasFieldParameter("received", viaList[0]) && mime.HasFieldParameter("rport", viaList[0])) {
    viaAddress = mime.GetFieldParameter("received", viaList[0]);
    viaPort = mime.GetFieldParameter("rport", viaList[0]);
  }
  // received is present
  else if (mime.HasFieldParameter("received", viaList[0]))
    viaAddress = mime.GetFieldParameter("received", viaList[0]);

  OpalTransportAddress address(viaAddress+":"+viaPort, ep.GetDefaultSignalPort(), (proto *= "TCP") ? "$tcp" : "udp$");

  return address;
}


OpalTransportAddress SIP_PDU::GetSendAddress(const PStringList & routeSet)
{
  if (!routeSet.IsEmpty()) {

    SIPURL firstRoute = routeSet[0];
    if (firstRoute.GetParamVars().Contains("lr")) {
      return OpalTransportAddress(firstRoute.GetHostAddress());
    }
  }

  return OpalTransportAddress();
}


void SIP_PDU::PrintOn(ostream & strm) const
{
  strm << mime.GetCSeq() << ' ';
  if (method != NumMethods)
    strm << uri;
  else if (statusCode != IllegalStatusCode)
    strm << '<' << (unsigned)statusCode << '>';
  else
    strm << "<<Uninitialised>>";
}


BOOL SIP_PDU::Read(OpalTransport & transport)
{
  // Do this to force a Read() by the PChannelBuffer outside of the
  // ios::lock() mutex which would prevent simultaneous reads and writes.
  transport.SetReadTimeout(PMaxTimeInterval);
#if defined(__MWERKS__) || (__GNUC__ >= 3) || (_MSC_VER >= 1300) || defined(SOLARIS)
  if (transport.rdbuf()->pubseekoff(0, ios_base::cur) == streampos(-1))
#else
  if (transport.rdbuf()->seekoff(0, ios::cur, ios::in) == EOF)
#endif                  
    transport.clear(ios::badbit);

  if (!transport.IsOpen()) {
    PTRACE(1, "SIP\tAttempt to read PDU from closed tansport " << transport);
    return FALSE;
  }

  // get the message from transport into cmd and parse MIME
  transport.clear();
  PString cmd(512);
  if (!transport.bad() && !transport.eof()) {
    transport.SetReadTimeout(3000);
    transport >> cmd >> mime;
  }

  if (transport.bad()) {
    PTRACE_IF(1, transport.GetErrorCode(PChannel::LastReadError) != PChannel::NoError,
              "SIP\tPDU Read failed: " << transport.GetErrorText(PChannel::LastReadError));
    return FALSE;
  }

  if (cmd.IsEmpty()) {
    PTRACE(2, "SIP\tNo Request-Line or Status-Line received on " << transport);
    return FALSE;
  }

  if (cmd.Left(4) *= "SIP/") {
    // parse Response version, code & reason (ie: "SIP/2.0 200 OK")
    PINDEX space = cmd.Find(' ');
    if (space == P_MAX_INDEX) {
      PTRACE(2, "SIP\tBad Status-Line received on " << transport);
      return FALSE;
    }

    versionMajor = cmd.Mid(4).AsUnsigned();
    versionMinor = cmd(cmd.Find('.')+1, space).AsUnsigned();
    statusCode = (StatusCodes)cmd.Mid(++space).AsUnsigned();
    info    = cmd.Mid(cmd.Find(' ', space));
    uri     = PString();
  }
  else {
    // parse the method, URI and version
    PStringArray cmds = cmd.Tokenise( ' ', FALSE);
    if (cmds.GetSize() < 3) {
      PTRACE(2, "SIP\tBad Request-Line received on " << transport);
      return FALSE;
    }

    int i = 0;
    while (!(cmds[0] *= MethodNames[i])) {
      i++;
      if (i >= NumMethods) {
        PTRACE(2, "SIP\tUnknown method name " << cmds[0] << " received on " << transport);
        return FALSE;
      }
    }
    method = (Methods)i;

    uri = cmds[1];
    versionMajor = cmds[2].Mid(4).AsUnsigned();
    versionMinor = cmds[2].Mid(cmds[2].Find('.')+1).AsUnsigned();
    info = PString();
  }

  if (versionMajor < 2) {
    PTRACE(2, "SIP\tInvalid version received on " << transport);
    return FALSE;
  }

  // get the SDP content body
  // if a content length is specified, read that length
  // if no content length is specified (which is not the same as zero length)
  // then read until plausible end of header marker
  PINDEX contentLength = mime.GetContentLength();

  // assume entity bodies can't be longer than a UDP packet
  if (contentLength > 1500) {
    PTRACE(2, "SIP\tImplausibly long Content-Length " << contentLength << " received on " << transport);
    return FALSE;
  }
  else if (contentLength < 0) {
    PTRACE(2, "SIP\tImpossible negative Content-Length on " << transport);
    return FALSE;
  }

  if (contentLength > 0)
    transport.read(entityBody.GetPointer(contentLength+1), contentLength);
  else if (!mime.IsContentLengthPresent()) {
    PBYTEArray pp;

#if defined(__MWERKS__) || (__GNUC__ >= 3) || (_MSC_VER >= 1300) || defined(SOLARIS)
    transport.rdbuf()->pubseekoff(0, ios_base::cur);
#else
    transport.rdbuf()->seekoff(0, ios::cur, ios::in);
#endif 

    //store in pp ALL the PDU (from beginning)
    transport.read((char*)pp.GetPointer(transport.GetLastReadCount()),transport.GetLastReadCount());
    PINDEX pos = 3;
    while(++pos < pp.GetSize() && !(pp[pos]=='\n' && pp[pos-1]=='\r' && pp[pos-2]=='\n' && pp[pos-3]=='\r'))
      ; //end of header is marked by "\r\n\r\n"

    if (pos<pp.GetSize())
      pos++;
    contentLength = PMAX(0,pp.GetSize() - pos);
    if(contentLength > 0)
      memcpy(entityBody.GetPointer(contentLength+1),pp.GetPointer()+pos,  contentLength);
    else
      contentLength = 0;
  }

  ////////////////
  entityBody[contentLength] = '\0';

#if PTRACING
  if (PTrace::CanTrace(4))
    PTRACE(4, "SIP\tPDU Received on " << transport << "\n"
           << cmd << '\n' << mime << entityBody);
  else
    PTRACE(3, "SIP\tPDU Received " << cmd << " on " << transport);
#endif

  BOOL removeSDP = TRUE;

  // 'application/' is case sensitive, 'sdp' is not
  PString ContentType = mime.GetContentType();
  if ((ContentType.Left(12) == "application/") && (ContentType.Mid(12) *= "sdp")) {
    sdp = new SDPSessionDescription();
    removeSDP = !sdp->Decode(entityBody);
  }

  if (removeSDP) {
    delete sdp;
    sdp = NULL;
  }

  return TRUE;
}


BOOL SIP_PDU::Write(OpalTransport & transport, const OpalTransportAddress & remoteAddress)
{
  if (!transport.IsOpen()) {
    PTRACE(1, "SIP\tAttempt to write PDU to closed tansport " << transport);
    return FALSE;
  }

  if (!remoteAddress.IsEmpty() && !transport.GetRemoteAddress().IsEquivalent(remoteAddress)) {
    // skip transport identifier
    SIPURL hosturl = remoteAddress.Mid(remoteAddress.Find('$')+1);

    // Do a DNS SRV lookup
    hosturl.AdjustToDNS();

    OpalTransportAddress actualRemoteAddress = hosturl.GetHostAddress();
    PTRACE(3, "SIP\tAdjusting transport remote address to " << actualRemoteAddress);
    transport.SetRemoteAddress(actualRemoteAddress);
  }


  PString str = Build();

#if PTRACING
  if (PTrace::CanTrace(4))
    PTRACE(4, "SIP\tSending PDU on " << transport << '\n' << str);
  else if (method != NumMethods)
    PTRACE(3, "SIP\tSending PDU " << MethodNames[method] << ' ' << uri << " on " << transport);
  else
    PTRACE(3, "SIP\tSending PDU " << (unsigned)statusCode << ' ' << info << " on " << transport);
#endif

  if (transport.WriteString(str))
    return TRUE;

  PTRACE(1, "SIP\tPDU Write failed: " << transport.GetErrorText(PChannel::LastWriteError));
  return FALSE;
}


PString SIP_PDU::Build()
{
  PStringStream str;

  if (sdp != NULL) {
    entityBody = sdp->Encode();
    mime.SetContentType("application/sdp");
  }

  mime.SetContentLength(entityBody.GetLength());

  if (method != NumMethods)
    str << MethodNames[method] << ' ' << uri << ' ';

  str << "SIP/" << versionMajor << '.' << versionMinor;

  if (method == NumMethods)
    str << ' ' << (unsigned)statusCode << ' ' << info;

  str << "\r\n"
      << setfill('\r') << mime << setfill(' ')
      << entityBody;
  return str;
}


PString SIP_PDU::GetTransactionID() const
{
  // sometimes peers put <> around address, use GetHostAddress on GetFrom to handle all cases
  SIPURL fromURL(mime.GetFrom());
  return mime.GetCallID() + fromURL.GetHostAddress().ToLower() + PString(mime.GetCSeq());
}


////////////////////////////////////////////////////////////////////////////////////

SIPTransaction::SIPTransaction(SIPEndPoint & ep,
                               OpalTransport & trans,
                               const PTimeInterval & minRetryTime,
                               const PTimeInterval & maxRetryTime)
  : endpoint(ep),
    transport(trans)
{
  connection = (SIPConnection *)NULL;
  Construct(minRetryTime, maxRetryTime);
  PTRACE(4, "SIP\tTransaction " << mime.GetCSeq() << " created.");
}


SIPTransaction::SIPTransaction(SIPConnection & conn,
                               OpalTransport & trans,
                               Methods meth)
  : SIP_PDU(meth, conn, trans),
    endpoint(conn.GetEndPoint()),
    transport(trans)
{
  connection = &conn;
  Construct();
  PTRACE(4, "SIP\tTransaction " << mime.GetCSeq() << " created.");
}


void SIPTransaction::Construct(const PTimeInterval & minRetryTime, const PTimeInterval & maxRetryTime)
{
  retryTimer.SetNotifier(PCREATE_NOTIFIER(OnRetry));
  completionTimer.SetNotifier(PCREATE_NOTIFIER(OnTimeout));

  retry = 1;
  state = NotStarted;

  retryTimeoutMin = ((minRetryTime != PMaxTimeInterval) && (minRetryTime != 0)) ? minRetryTime : endpoint.GetRetryTimeoutMin(); 
  retryTimeoutMax = ((maxRetryTime != PMaxTimeInterval) && (maxRetryTime != 0)) ? maxRetryTime : endpoint.GetRetryTimeoutMax();
}


SIPTransaction::~SIPTransaction()
{
  PTRACE_IF(1, state < Terminated_Success, "SIP\tDestroying transaction " << mime.GetCSeq() << " which is not yet terminated.");
  PTRACE(4, "SIP\tTransaction " << mime.GetCSeq() << " destroyed.");
}


BOOL SIPTransaction::Start()
{
  endpoint.AddTransaction(this);

  if (state != NotStarted) {
    PAssertAlways(PLogicError);
    return FALSE;
  }

  if (connection != NULL)
    connection->GetAuthenticator().Authorise(*this); 

  PSafeLockReadWrite lock(*this);

  state = Trying;
  retry = 0;
  retryTimer = retryTimeoutMin;
  localAddress = transport.GetLocalAddress();

  if (method == Method_INVITE)
    completionTimer = endpoint.GetInviteTimeout();
  else
    completionTimer = endpoint.GetNonInviteTimeout();

  PStringList routeSet = this->GetMIME().GetRoute(); // Get the route set from the PDU
  if (connection != NULL) {
    // Use the connection transport to send the request
    if (connection->SendPDU(*this, GetSendAddress(routeSet)))
      return TRUE;
  }
  else {
    if (Write(transport, GetSendAddress(routeSet)))
      return TRUE;
  }

  SetTerminated(Terminated_TransportError);
  return FALSE;
}


void SIPTransaction::WaitForCompletion()
{
  if (state >= Completed)
    return;

  if (state == NotStarted)
    Start();

  completed.Wait();
}


BOOL SIPTransaction::Cancel()
{
  PSafeLockReadWrite lock(*this);

  if (state == NotStarted || state >= Cancelling) {
    PTRACE(3, "SIP\tTransaction " << mime.GetCSeq() << " cannot be cancelled.");
    return FALSE;
  }

  completionTimer = endpoint.GetPduCleanUpTimeout();
  return ResendCANCEL();
}


void SIPTransaction::Abort()
{
  if (LockReadWrite()) {
    SetTerminated(Terminated_Aborted);
    UnlockReadWrite();
  }
}


BOOL SIPTransaction::ResendCANCEL()
{
  SIP_PDU cancel(Method_CANCEL,
                 uri,
                 mime.GetTo(),
                 mime.GetFrom(),
                 mime.GetCallID(),
                 mime.GetCSeqIndex(),
                 localAddress);
  // Use the topmost via header from the INVITE we cancel as per 9.1. 
  PStringList viaList = mime.GetViaList();
  cancel.GetMIME().SetVia(viaList[0]);

  if (!transport.SetLocalAddress(localAddress) || !cancel.Write(transport)) {
    SetTerminated(Terminated_TransportError);
    return FALSE;
  }

  if (state < Cancelling) {
    state = Cancelling;
    retry = 0;
    retryTimer = retryTimeoutMin;
  }

  return TRUE;
}


BOOL SIPTransaction::OnReceivedResponse(SIP_PDU & response)
{
  // Stop the timers outside of the mutex to avoid deadlock
  retryTimer.Stop();
  completionTimer.Stop();

  PString cseq = response.GetMIME().GetCSeq();

  // If is the response to a CANCEl we sent, then just ignore it
  if (cseq.Find(MethodNames[Method_CANCEL]) != P_MAX_INDEX) {
    // Lock only if we have not already locked it in SIPInvite::OnReceivedResponse
    if (LockReadWrite()) {
      SetTerminated(Terminated_Cancelled);
      UnlockReadWrite();
    }
    return FALSE;
  }

  // Something wrong here, response is not for the request we made!
  if (cseq.Find(MethodNames[method]) == P_MAX_INDEX) {
    PTRACE(2, "SIP\tTransaction " << cseq << " response not for " << *this);
    return FALSE;
  }

  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return FALSE;

  BOOL notCompletedFlag = state < Completed;

  /* Really need to check if response is actually meant for us. Have a
     temporary cheat in assuming that we are only sending a given CSeq to one
     and one only host, so anything coming back with that CSeq is OK. This has
     "issues" according to the spec but
     */
  if (notCompletedFlag && response.GetStatusCode()/100 == 1) {
    PTRACE(3, "SIP\tTransaction " << cseq << " proceeding.");

    if (state == Trying)
      state = Proceeding;

    retry = 0;
    retryTimer = retryTimeoutMax;
    completionTimer = endpoint.GetNonInviteTimeout();
  }
  else {
    if (notCompletedFlag) {
      PTRACE(3, "SIP\tTransaction " << cseq << " completed.");
      state = Completed;
    }

    completed.Signal();
    completionTimer = endpoint.GetPduCleanUpTimeout();
  }

  if (notCompletedFlag) {
    if (connection != NULL)
      connection->OnReceivedResponse(*this, response);
    else
      endpoint.OnReceivedResponse(*this, response);

    if (state == Completed)
      return OnCompleted(response);
  }

  return TRUE;
}


BOOL SIPTransaction::OnCompleted(SIP_PDU & /*response*/)
{
  return TRUE;
}


void SIPTransaction::OnRetry(PTimer &, INT)
{
  PSafeLockReadWrite lock(*this);

  if (!lock.IsLocked() || state != Trying)
    return;

  retry++;

  PTRACE(3, "SIP\tTransaction " << mime.GetCSeq() << " timeout, making retry " << retry);

  if (retry >= endpoint.GetMaxRetries()) {
    SetTerminated(Terminated_RetriesExceeded);
    return;
  }

  if (state == Cancelling) {
    if (!ResendCANCEL())
      return;
  }
  else if (!transport.SetLocalAddress(localAddress) || !Write(transport)) {
    SetTerminated(Terminated_TransportError);
    return;
  }

  PTimeInterval timeout = retryTimeoutMin*(1<<retry);
  if (timeout > retryTimeoutMax)
    retryTimer = retryTimeoutMax;
  else
    retryTimer = timeout;
}


void SIPTransaction::OnTimeout(PTimer &, INT)
{
  PSafeLockReadWrite lock(*this);

  if (lock.IsLocked() && state <= Completed)
    SetTerminated(state != Completed ? Terminated_Timeout : Terminated_Success);
}


void SIPTransaction::SetTerminated(States newState)
{
#if PTRACING
  static const char * const StateNames[NumStates] = {
    "NotStarted",
    "Trying",
    "Aborting",
    "Proceeding",
    "Cancelling",
    "Completed",
    "Terminated_Success",
    "Terminated_Timeout",
    "Terminated_RetriesExceeded",
    "Terminated_TransportError",
    "Terminated_Cancelled",
    "Terminated_Aborted"
  };
#endif
  
  if (state >= Terminated_Success) {
    PTRACE_IF(3, newState != Terminated_Success, "SIP\tTried to set state " << StateNames[newState] 
              << " for transaction " << mime.GetCSeq()
              << " but already terminated ( " << StateNames[state] << ')');
    return;
  }
  
  States oldState = state;
  
  state = newState;
  PTRACE(3, "SIP\tSet state " << StateNames[newState] << " for transaction " << mime.GetCSeq());

  // Transaction failed, tell the endpoint
  if (state != Terminated_Success) 
    endpoint.OnTransactionTimeout(*this);

  if (oldState != Completed)
    completed.Signal();

  if (state != Terminated_Success && connection != NULL)
    connection->OnTransactionFailed(*this);
}


////////////////////////////////////////////////////////////////////////////////////

SIPInvite::SIPInvite(SIPConnection & connection, OpalTransport & transport)
  : SIPTransaction(connection, transport, Method_INVITE)
{
  mime.SetDate() ;                             // now
  mime.SetProductInfo(connection.GetEndPoint().GetUserAgent(), connection.GetProductInfo());

  connection.BuildSDP(sdp, rtpSessions, OpalMediaFormat::DefaultAudioSessionID);
#if OPAL_VIDEO
  if (connection.GetEndPoint().GetManager().CanAutoStartTransmitVideo()
      || connection.GetEndPoint().GetManager().CanAutoStartReceiveVideo())
    connection.BuildSDP(sdp, rtpSessions, OpalMediaFormat::DefaultVideoSessionID);
#endif
#if OPAL_T38FAX
  connection.BuildSDP(sdp, rtpSessions, OpalMediaFormat::DefaultDataSessionID);
#endif
  connection.OnCreatingINVITE(*this);
}


SIPInvite::SIPInvite(SIPConnection & connection, OpalTransport & transport, RTP_SessionManager & sm)
  : SIPTransaction(connection, transport, Method_INVITE)
{
  mime.SetDate() ;                             // now
  mime.SetProductInfo(connection.GetEndPoint().GetUserAgent(), connection.GetProductInfo());

  rtpSessions = sm;
  connection.BuildSDP(sdp, rtpSessions, OpalMediaFormat::DefaultAudioSessionID);
#if OPAL_VIDEO
  if (connection.GetEndPoint().GetManager().CanAutoStartTransmitVideo()
      || connection.GetEndPoint().GetManager().CanAutoStartReceiveVideo())
    connection.BuildSDP(sdp, rtpSessions, OpalMediaFormat::DefaultVideoSessionID);
#endif
#if OPAL_T38FAX
  connection.BuildSDP(sdp, rtpSessions, OpalMediaFormat::DefaultDataSessionID);
#endif
  connection.OnCreatingINVITE(*this);
}

SIPInvite::SIPInvite(SIPConnection & connection, OpalTransport & transport, unsigned rtpSessionId)
  : SIPTransaction(connection, transport, Method_INVITE)
{
  mime.SetDate() ;                             // now
  mime.SetProductInfo(connection.GetEndPoint().GetUserAgent(), connection.GetProductInfo());

  connection.BuildSDP(sdp, rtpSessions, rtpSessionId);
}

BOOL SIPInvite::OnReceivedResponse(SIP_PDU & response)
{
  States originalState = state;

  if (!SIPTransaction::OnReceivedResponse(response))
    return FALSE;

  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return FALSE;

  if (response.GetStatusCode()/100 == 1)
    completionTimer = PTimeInterval(0, mime.GetExpires(180));
  else if (originalState >= Completed) {
    // If the state was already 'Completed', ensure that still an ACK is sent
    connection->SendACK(*this, response);
  }

  /* Handle response to outgoing call cancellation */
  if (response.GetStatusCode() == Failure_RequestTerminated)
    SetTerminated(Terminated_Success);

  return TRUE;
}


SIPRegister::SIPRegister(SIPEndPoint & ep,
                         OpalTransport & trans,
                         const PStringList & routeSet,
                         const SIPURL & address,
                         const PString & id,
                         unsigned expires,
                         const PTimeInterval & minRetryTime, 
                         const PTimeInterval & maxRetryTime)
  : SIPTransaction(ep, trans, minRetryTime, maxRetryTime)
{
  PString addrStr = address.AsQuotedString();
  OpalTransportAddress viaAddress = ep.GetLocalURL(transport).GetHostAddress();
  SIP_PDU::Construct(Method_REGISTER,
                     "sip:"+address.GetHostName(),
                     addrStr,
                     addrStr+";tag="+OpalGloballyUniqueID().AsString(),
                     id,
                     endpoint.GetNextCSeq(),
                     viaAddress);

  mime.SetProductInfo(ep.GetUserAgent(), ep.GetProductInfo());
  SIPURL contact = endpoint.GetLocalURL(trans, address.GetUserName());
  mime.SetContact(contact);
  mime.SetExpires(expires);

  SetRoute(routeSet);
}


SIPSubscribe::SIPSubscribe(SIPEndPoint & ep,
                           OpalTransport & trans,
                           SIPSubscribe::SubscribeType & type,
                           const PStringList & routeSet,
                           const SIPURL & targetAddress,
                           const PString & remotePartyAddress,
                           const PString & localPartyAddress,
                           const PString & id,
                           const unsigned & cseq,
                           unsigned expires)
  : SIPTransaction(ep, trans)
{
  PString acceptField;
  PString eventField;
  
  switch (type) {
  case MessageSummary:
    eventField = "message-summary";
    acceptField = "application/simple-message-summary";
    break;
    
  default:
  case Presence:
    eventField = "presence";
    acceptField = "application/pidf+xml";
    break;
  }

  SIPURL address = targetAddress;
  address.AdjustForRequestURI();

  OpalTransportAddress viaAddress = ep.GetLocalURL(transport).GetHostAddress();
  SIP_PDU::Construct(Method_SUBSCRIBE,
                     address,
                     remotePartyAddress,
                     localPartyAddress,
                     id,
                     cseq,
                     viaAddress);

  SIPURL contact = 
    endpoint.GetLocalURL(trans, SIPURL(localPartyAddress).GetUserName());
  mime.SetProductInfo(ep.GetUserAgent(), ep.GetProductInfo());
  mime.SetContact(contact);
  mime.SetAccept(acceptField);
  mime.SetEvent(eventField);
  mime.SetExpires(expires);

  SetRoute(routeSet);
}

SIPPublish::SIPPublish(SIPEndPoint & ep,
                       OpalTransport & trans,
                       const PStringList & /*routeSet*/,
                       const SIPURL & targetAddress,
                       const PString & sipIfMatch,
                       const PString & body,
                       unsigned expires)
  : SIPTransaction(ep, trans)
{
  PString addrStr = targetAddress.AsQuotedString();
  PString id = OpalGloballyUniqueID().AsString();
  id += "@" + PIPSocket::GetHostName();
  OpalTransportAddress viaAddress = ep.GetLocalURL(transport).GetHostAddress();

  SIP_PDU::Construct(Method_PUBLISH,
                     targetAddress,
                     addrStr,
                     addrStr+";tag="+OpalGloballyUniqueID().AsString(),
                     id,
                     endpoint.GetNextCSeq(),
                     viaAddress);

  mime.SetProductInfo(ep.GetUserAgent(), ep.GetProductInfo());
  SIPURL contact = endpoint.GetLocalURL(trans, targetAddress.GetUserName());
  mime.SetContact(contact);
  mime.SetExpires(expires);

  if (!sipIfMatch.IsEmpty())
    mime.SetSIPIfMatch(sipIfMatch);
  
  mime.SetEvent("presence");
  mime.SetContentType("application/pidf+xml");

  if (!body.IsEmpty())
    entityBody = body;
}


/////////////////////////////////////////////////////////////////////////

SIPRefer::SIPRefer(SIPConnection & connection, OpalTransport & transport, const PString & refer, const PString & referred_by)
  : SIPTransaction(connection, transport, Method_REFER)
{
  Construct(connection, transport, refer, referred_by);
}

SIPRefer::SIPRefer(SIPConnection & connection, OpalTransport & transport, const PString & refer)
  : SIPTransaction(connection, transport, Method_REFER)
{
  Construct(connection, transport, refer, PString::Empty());
}


void SIPRefer::Construct(SIPConnection & connection, OpalTransport & /*transport*/, const PString & refer, const PString & referred_by)
{
  mime.SetProductInfo(connection.GetEndPoint().GetUserAgent(), connection.GetProductInfo());
  mime.SetReferTo(refer);
  if(!referred_by.IsEmpty())
    mime.SetReferredBy(referred_by);
}


/////////////////////////////////////////////////////////////////////////

SIPReferNotify::SIPReferNotify(SIPConnection & connection, OpalTransport & transport, StatusCodes code)
  : SIPTransaction(connection, transport, Method_NOTIFY)
{
  PStringStream str;
  
  mime.SetProductInfo(connection.GetEndPoint().GetUserAgent(), connection.GetProductInfo());
  mime.SetSubscriptionState("terminated;reason=noresource"); // Do not keep an internal state
  mime.SetEvent("refer");
  mime.SetContentType("message/sipfrag;version=2.0");


  str << "SIP/" << versionMajor << '.' << versionMinor << " " << code << " " << GetStatusCodeDescription(code);
  entityBody = str;
}


/////////////////////////////////////////////////////////////////////////

SIPMessage::SIPMessage(SIPEndPoint & ep,
                       OpalTransport & trans,
                       const SIPURL & address,
                       const PStringList & routeSet,
                       const PString & body)
  : SIPTransaction(ep, trans)
{
  PString id = OpalGloballyUniqueID().AsString() + "@" + PIPSocket::GetHostName();
  OpalTransportAddress viaAddress = ep.GetLocalURL(transport).GetHostAddress();
    
  // Build the correct From field
  PString displayName = ep.GetDefaultDisplayName();
  PString partyName = endpoint.GetRegisteredPartyName(address).AsString(); 

  SIPURL myAddress("\"" + displayName + "\" <" + partyName + ">"); 
  
  SIP_PDU::Construct(Method_MESSAGE,
                     "sip:"+address.GetUserName()+"@"+address.GetHostName(),
                     address.AsQuotedString(),
                     myAddress.AsQuotedString()+";tag="+OpalGloballyUniqueID().AsString(),
                     id,
                     endpoint.GetNextCSeq(),
                     viaAddress);
  mime.SetContentType("text/plain;charset=UTF-8");
  mime.SetRoute(routeSet);

  entityBody = body;
}

/////////////////////////////////////////////////////////////////////////

SIPPing::SIPPing(SIPEndPoint & ep,
                 OpalTransport & trans,
                 const SIPURL & address,
                 const PString & body)
  : SIPTransaction(ep, trans)
{
  PString id = OpalGloballyUniqueID().AsString() + "@" + PIPSocket::GetHostName();
  OpalTransportAddress viaAddress = ep.GetLocalURL(transport).GetHostAddress();
    
  // Build the correct From field
  PString displayName = ep.GetDefaultDisplayName();
  PString partyName = endpoint.GetRegisteredPartyName(address).AsString(); 

  SIPURL myAddress("\"" + displayName + "\" <" + partyName + ">"); 
  
  SIP_PDU::Construct(Method_PING,
                     "sip:"+address.GetUserName()+"@"+address.GetHostName(),
                     address.AsQuotedString(),
                     // myAddress.AsQuotedString()+";tag="+OpalGloballyUniqueID().AsString(),
                     "sip:"+address.GetUserName()+"@"+address.GetHostName(),
                     id,
                     endpoint.GetNextCSeq(),
                     viaAddress);
  mime.SetContentType("text/plain;charset=UTF-8");

  entityBody = body;
}


/////////////////////////////////////////////////////////////////////////

SIPAck::SIPAck(SIPEndPoint & ep,
               SIPTransaction & invite,
               SIP_PDU & response)
  : SIP_PDU (SIP_PDU::Method_ACK,
             invite.GetURI(),
             response.GetMIME().GetTo(),
             invite.GetMIME().GetFrom(),
             invite.GetMIME().GetCallID(),
             invite.GetMIME().GetCSeqIndex(),
             ep.GetLocalURL(invite.GetTransport()).GetHostAddress()),
  transaction(invite)
{
  Construct();
  // Use the topmost via header from the INVITE we ACK as per 17.1.1.3
  // as well as the initial Route
  PStringList viaList = invite.GetMIME().GetViaList();
  mime.SetVia(viaList[0]);

  if (transaction.GetMIME().GetRoute().GetSize() > 0)
    mime.SetRoute(transaction.GetMIME().GetRoute());
}


SIPAck::SIPAck(SIPTransaction & invite)
  : SIP_PDU (SIP_PDU::Method_ACK,
             *invite.GetConnection(),
             invite.GetTransport()),
  transaction(invite)
{
  mime.SetCSeq(PString(invite.GetMIME().GetCSeqIndex()) & MethodNames[Method_ACK]);
  Construct();
}


void SIPAck::Construct()
{
  // Add authentication if had any on INVITE
  if (transaction.GetMIME().Contains("Proxy-Authorization") || transaction.GetMIME().Contains("Authorization"))
    transaction.GetConnection()->GetAuthenticator().Authorise(*this);
}


/////////////////////////////////////////////////////////////////////////

SIPOptions::SIPOptions(SIPEndPoint & ep,
                       OpalTransport & trans,
                       const SIPURL & address)
  : SIPTransaction(ep, trans)
{
  PString requestURI;
  PString hosturl;
  PString id = OpalGloballyUniqueID().AsString() + "@" + PIPSocket::GetHostName();
  OpalTransportAddress viaAddress = ep.GetLocalURL(transport).GetHostAddress();
    
  // Build the correct From field
  PString displayName = ep.GetDefaultDisplayName();
  PString partyName = endpoint.GetRegisteredPartyName(SIPURL(address).GetHostName()).AsString();

  SIPURL myAddress("\"" + displayName + "\" <" + partyName + ">"); 

  requestURI = "sip:" + address.AsQuotedString();
  
  SIP_PDU::Construct(Method_OPTIONS,
                     requestURI,
                     address.AsQuotedString(),
                     myAddress.AsQuotedString()+";tag="+OpalGloballyUniqueID().AsString(),
                     id,
                     endpoint.GetNextCSeq(),
                     viaAddress);
  mime.SetAccept("application/sdp");
}
// End of file ////////////////////////////////////////////////////////////////
