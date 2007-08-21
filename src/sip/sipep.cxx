/*
 * sipep.cxx
 *
 * Session Initiation Protocol endpoint.
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
 * $Log: sipep.cxx,v $
 * Revision 1.2183  2007/08/21 03:12:03  csoutheren
 * Fix problem with From having wrong protocol
 *
 * Revision 2.181  2007/08/07 19:44:41  dsandras
 * Fixed typo. Ideally, the NAT binding refresh code should move to
 * OpalTransportUDP.
 *
 * Revision 2.180  2007/08/07 17:23:14  hfriederich
 * Re-enable NAT binding refreshes.
 *
 * Revision 2.179  2007/07/22 13:02:14  rjongbloed
 * Cleaned up selection of registered name usage of URL versus host name.
 *
 * Revision 2.178  2007/07/22 03:25:18  rjongbloed
 * Assured that if no expiry time is specified for REGISTER, the endpoint default is used.
 *
 * Revision 2.177  2007/07/05 05:44:16  rjongbloed
 * Simplified the default local party name, prevents it from ever returning 0.0.0.0 as host.
 *
 * Revision 2.176  2007/06/30 16:43:47  dsandras
 * Improved exit sequence.
 *
 * Revision 2.175  2007/06/29 06:59:58  rjongbloed
 * Major improvement to the "product info", normalising H.221 and User-Agent mechanisms.
 *
 * Revision 2.174  2007/06/25 05:16:20  rjongbloed
 * Changed GetDefaultTransport() so can return multiple transport names eg udp$ AND tcp$.
 * Changed listener start up so if no transport is mentioned in the "interface" to listen on
 *   then will listen on all transports supplied by GetDefaultTransport()
 *
 * Revision 2.173  2007/06/10 08:55:13  rjongbloed
 * Major rework of how SIP utilises sockets, using new "socket bundling" subsystem.
 *
 * Revision 2.172  2007/05/23 19:58:04  dsandras
 * Run garbage collector more often.
 *
 * Revision 2.171  2007/05/22 21:04:49  dsandras
 * Fixed issue when PTRACING if the transport could not be created.
 *
 * Revision 2.170  2007/05/21 17:45:11  dsandras
 * Fixed usage of Register due to recent changes in the API leading to a crash
 * on exit (when unregistering).
 *
 * Revision 2.169  2007/05/21 17:34:49  dsandras
 * Added guard against NULL transport.
 *
 * Revision 2.168  2007/05/18 00:35:24  csoutheren
 * Normalise Register functions
 * Add symbol so applications know about presence of presence :)
 *
 * Revision 2.167  2007/05/16 10:25:57  dsandras
 * Fixed previous commit.
 *
 * Revision 2.166  2007/05/16 01:17:07  csoutheren
 * Added new files to Windows build
 * Removed compiler warnings on Windows
 * Added backwards compatible SIP Register function
 *
 * Revision 2.165  2007/05/15 20:46:00  dsandras
 * Added various handlers to manage subscriptions for presence, message
 * waiting indications, registrations, state publishing,
 * message conversations, ...
 * Adds/fixes support for RFC3856, RFC3903, RFC3863, RFC3265, ...
 * Many improvements over the original SIPInfo code.
 * Code contributed by NOVACOM (http://www.novacom.be) thanks to
 * EuroWeb (http://www.euroweb.hu).
 *
 * Revision 2.164  2007/05/15 07:27:34  csoutheren
 * Remove deprecated  interface to STUN server in H323Endpoint
 * Change UseNATForIncomingCall to IsRTPNATEnabled
 * Various cleanups of messy and unused code
 *
 * Revision 2.163  2007/05/10 04:45:35  csoutheren
 * Change CSEQ storage to be an atomic integer
 * Fix hole in transaction mutex handling
 *
 * Revision 2.162  2007/05/01 05:29:32  csoutheren
 * Fix problem with bad delete of SIPConnection when transport not available
 *
 * Revision 2.161  2007/04/21 11:05:56  dsandras
 * Fixed more interoperability problems due to bugs in Cisco Call Manager.
 * Do not clear calls if the video transport can not be established.
 * Only reinitialize the registrar transport if required (STUN is being used
 * and the public IP address has changed).
 *
 * Revision 2.160  2007/04/20 07:08:55  rjongbloed
 * Fixed compiler warning
 *
 * Revision 2.159  2007/04/18 03:23:51  rjongbloed
 * Moved large chunk of code from header to source file.
 *
 * Revision 2.158  2007/04/17 21:49:41  dsandras
 * Fixed Via field in previous commit.
 * Make sure the correct port is being used.
 * Improved FindSIPInfoByDomain.
 *
 * Revision 2.157  2007/04/15 10:09:15  dsandras
 * Some systems like CISCO Call Manager do not like having a Contact field in INVITE
 * PDUs which is different to the one being used in the original REGISTER request.
 * Added code to use the same Contact field in both cases if we can determine that
 * we are registered to that specific account and if there is a transport running.
 * Fixed problem where the SIP connection was not released with a BYE PDU when
 * the ACK is received while we are already in EstablishedPhase.
 *
 * Revision 2.156  2007/04/04 02:12:02  rjongbloed
 * Reviewed and adjusted PTRACE log levels
 *   Now follows 1=error,2=warn,3=info,4+=debug
 *
 * Revision 2.155  2007/04/03 07:40:24  rjongbloed
 * Fixed CreateCall usage so correct function (with userData) is called on
 *   incoming connections.
 *
 * Revision 2.154  2007/03/30 14:45:32  hfriederich
 * Reorganization of hte way transactions are handled. Delete transactions
 *   in garbage collector when they're terminated. Update destructor code
 *   to improve safe destruction of SIPEndPoint instances.
 *
 * Revision 2.153  2007/03/29 23:55:46  rjongbloed
 * Tidied some code when a new connection is created by an endpoint. Now
 *   if someone needs to derive a connection class they can create it without
 *   needing to remember to do any more than the new.
 *
 * Revision 2.152  2007/03/27 21:51:40  dsandras
 * Added more PTRACE statements
 *
 * Revision 2.151  2007/03/27 20:35:30  dsandras
 * More checks for transport validity.
 *
 * Revision 2.150  2007/03/27 20:16:23  dsandras
 * Temporarily removed use of shared transports as it could have unexpected
 * side effects on the routing of PDUs.
 * Various fixes on the way SIPInfo objects are being handled. Wait
 * for transports to be closed before being deleted. Added missing mutexes.
 * Added garbage collector.
 *
 * Revision 2.148  2007/03/17 15:00:24  dsandras
 * Keep the transport open for a longer time when doing text conversations.
 *
 * Revision 2.147  2007/03/13 21:22:49  dsandras
 * Take the remote port into account when unregistering.
 *
 * Revision 2.146  2007/03/10 17:56:58  dsandras
 * Improved locking.
 *
 * Revision 2.145  2007/03/01 05:51:07  rjongbloed
 * Fixed backward compatibility of OnIncomingConnection() virtual
 *   functions on various classes. If an old override returned FALSE
 *   then it will now abort the call as it used to.
 *
 * Revision 2.144  2007/02/27 21:22:42  dsandras
 * Added missing locking. Fixes Ekiga report #411438.
 *
 * Revision 2.143  2007/02/20 07:15:03  csoutheren
 * Second attempt at sane 180 handling
 *
 * Revision 2.142  2007/02/19 04:40:23  csoutheren
 * Don't send multple 100 Trying
 * Guard against inability to create transports
 *
 * Revision 2.141  2007/01/24 04:00:57  csoutheren
 * Arrrghh. Changing OnIncomingConnection turned out to have a lot of side-effects
 * Added some pure viritual functions to prevent old code from breaking silently
 * New OpalEndpoint and OpalConnection descendants will need to re-implement
 * OnIncomingConnection. Sorry :)
 *
 * Revision 2.140  2007/01/18 04:45:17  csoutheren
 * Messy, but simple change to add additional options argument to OpalConnection constructor
 * This allows the provision of non-trivial arguments for connections
 *
 * Revision 2.139  2007/01/15 22:14:19  dsandras
 * Added missing mutexes.
 *
 * Revision 2.138  2007/01/04 22:09:41  dsandras
 * Make sure we do not strip a > when stripping parameters from the From field
 * when receiving a MESSAGE.
 *
 * Revision 2.137  2006/12/18 03:18:42  csoutheren
 * Messy but simple fixes
 *   - Add access to SIP REGISTER timeout
 *   - Ensure OpalConnection options are correctly progagated
 *
 * Revision 2.136  2006/12/10 19:16:19  dsandras
 * Fixed typo.
 *
 * Revision 2.135  2006/12/10 18:31:17  dsandras
 * Synced with Phobos.
 *
 * Revision 2.134  2006/12/08 04:02:51  csoutheren
 * Applied 1575788 - Fix a crash in ~SIPInfo due to an open transport
 * Thanks to Drazen Dimoti
 *
 * Revision 2.133  2006/11/09 18:24:55  hfriederich
 * Ensures that responses to received INVITE get sent out on the same network interface as where the INVITE was received. Remove cout statement
 *
 * Revision 2.132  2006/11/06 13:57:40  dsandras
 * Use readonly locks as suggested by Robert as we are not
 * writing to the collection. Fixes deadlock on SIP when
 * reinvite.
 *
 * Revision 2.131  2006/10/06 09:08:32  dsandras
 * Added autodetection of the realm from the challenge response.
 *
 * Revision 2.130  2006/09/28 07:42:18  csoutheren
 * Merge of useful SRTP implementation
 *
 * Revision 2.129  2006/08/12 04:20:39  csoutheren
 * Removed Windows warning and fixed timeout in SIP PING code
 *
 * Revision 2.128  2006/08/12 04:09:24  csoutheren
 * Applied 1538497 - Add the PING method
 * Thanks to Paul Rolland
 *
 * Revision 2.127  2006/07/29 08:47:34  hfriederich
 * Abort the authentication process after a given number of unsuccessful authentication attempts to prevent infinite authentication attempts. Use not UINT_MAX as GetExpires() default to fix incorrect detection of registration/unregistration
 *
 * Revision 2.126  2006/07/14 04:22:43  csoutheren
 * Applied 1517397 - More Phobos stability fix
 * Thanks to Dinis Rosario
 *
 * Revision 2.125  2006/06/10 15:37:33  dsandras
 * Look for the expires field in the PDU if not present in the contact header.
 *
 * Revision 2.124  2006/05/30 04:58:06  csoutheren
 * Added suport for SIP INFO message (untested as yet)
 * Fixed some issues with SIP state machine on answering calls
 * Fixed some formatting issues
 *
 * Revision 2.123  2006/04/30 17:24:40  dsandras
 * Various clean ups.
 *
 * Revision 2.122  2006/04/11 21:13:57  dsandras
 * Modified GetRegisteredPartyName so that the DefaultLocalPartyName can be
 * used as result if no match is found for the called domain.
 *
 * Revision 2.121  2006/04/06 20:39:41  dsandras
 * Keep the same From header when sending authenticated requests than in the
 * original request. Fixes Ekiga report #336762.
 *
 * Revision 2.120  2006/03/27 20:28:18  dsandras
 * Added mutex to fix concurrency issues between OnReceivedPDU which checks
 * if a connection is in the list, and OnReceivedINVITE, which adds it to the
 * list. Fixes Ekiga report #334847. Thanks Robert for your input on this!
 *
 * Revision 2.119  2006/03/23 21:23:08  dsandras
 * Fixed SIP Options request sent when refreshint NAT bindings.
 *
 * Revision 2.118  2006/03/19 18:57:06  dsandras
 * More work on Ekiga report #334999.
 *
 * Revision 2.117  2006/03/19 18:14:11  dsandras
 * Removed unused variable.
 *
 * Revision 2.116  2006/03/19 13:17:15  dsandras
 * Ignore failures when unregistering and leaving. Fixes Ekiga #334997.
 *
 * Revision 2.115  2006/03/19 12:32:05  dsandras
 * RFC3261 says that "CANCEL messages "SHOULD NOT" be sent for anything but INVITE
 * requests". Fixes Ekiga report #334985.
 *
 * Revision 2.114  2006/03/19 11:45:48  dsandras
 * The remote address of the registrar transport might have changed due
 * to the Via field. This affected unregistering which was reusing
 * the exact same transport to unregister. Fixed Ekiga report #334999.
 *
 * Revision 2.113  2006/03/08 20:10:12  dsandras
 * Fixed logic error when receiving an incomplete MWI NOTIFY PDU.
 *
 * Revision 2.112  2006/03/08 18:55:04  dsandras
 * Added missing return TRUE.
 *
 * Revision 2.111  2006/03/08 18:34:41  dsandras
 * Added DNS SRV lookup.
 *
 * Revision 2.110  2006/03/06 22:52:59  csoutheren
 * Reverted experimental SRV patch due to unintended side effects
 *
 * Revision 2.109  2006/03/06 19:01:47  dsandras
 * Allow registering several accounts with the same realm but different
 * user names to the same provider. Fixed possible crash due to transport
 * deletion before the transaction is over.
 *
 * Revision 2.108  2006/03/06 12:56:03  csoutheren
 * Added experimental support for SIP SRV lookups
 *
 * Revision 2.107  2006/02/28 19:18:24  dsandras
 * Reset the expire time to its initial value accepted by the server when
 * refreshing the registration. Have a default value of 1 for voicemail
 * notifications.
 *
 * Revision 2.106  2006/02/14 19:04:07  dsandras
 * Fixed problem when the voice-message field is not mentionned in the NOTIFY.
 *
 * Revision 2.105  2006/02/08 04:51:19  csoutheren
 * Adde trace message when sending 502
 *
 * Revision 2.104  2006/02/05 22:19:18  dsandras
 * Only refresh registered accounts. In case of timeout of a previously
 * registered account, retry regularly.
 *
 * Revision 2.103  2006/01/31 03:27:19  csoutheren
 * Removed unused variable
 * Fixed typo in comparison
 *
 * Revision 2.102  2006/01/29 21:03:32  dsandras
 * Removed cout.
 *
 * Revision 2.101  2006/01/29 20:55:33  dsandras
 * Allow using a simple username or a fill url when registering.
 *
 * Revision 2.100  2006/01/29 17:09:48  dsandras
 * Added guards against timeout of 0 when registering or subscribing.
 * Make sure activeSIPInfo is empty and destroyed before exiting.
 *
 * Revision 2.99  2006/01/24 22:24:48  dsandras
 * Fixed possible bug with wait called on non existing SIPInfo.
 *
 * Revision 2.98  2006/01/24 19:49:29  dsandras
 * Simplified code, always recreate the transport.
 *
 * Revision 2.97  2006/01/14 10:43:06  dsandras
 * Applied patch from Brian Lu <Brian.Lu _AT_____ sun.com> to allow compilation
 * with OpenSolaris compiler. Many thanks !!!
 *
 * Revision 2.96  2006/01/09 12:15:42  csoutheren
 * Fixed warning under VS.net 2003
 *
 * Revision 2.95  2006/01/08 21:53:41  dsandras
 * Changed IsRegistered so that it takes the registration url as argument,
 * allowing it to work when there are several accounts on the same server.
 *
 * Revision 2.94  2006/01/08 14:43:47  dsandras
 * Improved the NAT binding refresh methods so that it works with all endpoint
 * created transports that require it and so that it can work by sending
 * SIP Options, or empty SIP requests. More methods can be added later.
 *
 * Revision 2.93  2006/01/07 18:30:20  dsandras
 * Further simplification.
 *
 * Revision 2.92  2006/01/07 18:29:44  dsandras
 * Fixed exit problems in case of unregistration failure.
 *
 * Revision 2.91  2006/01/02 11:28:07  dsandras
 * Some documentation. Various code cleanups to prevent duplicate code.
 *
 * Revision 2.90  2005/12/20 20:40:47  dsandras
 * Added missing parameter.
 *
 * Revision 2.89  2005/12/18 21:06:55  dsandras
 * Added function to clean up the registrations object. Moved DeleteObjectsToBeRemoved call outside of the loop.
 *
 * Revision 2.88  2005/12/18 19:18:18  dsandras
 * Regularly clean activeSIPInfo.
 * Fixed problem with Register ignoring the timeout parameter.
 *
 * Revision 2.87  2005/12/17 21:14:12  dsandras
 * Prevent loop when exiting if unregistration fails forever.
 *
 * Revision 2.86  2005/12/14 17:59:50  dsandras
 * Added ForwardConnection executed when the remote asks for a call forwarding.
 * Similar to what is done in the H.323 part with the method of the same name.
 *
 * Revision 2.85  2005/12/11 19:14:20  dsandras
 * Added support for setting a different user name and authentication user name
 * as required by some providers like digisip.
 *
 * Revision 2.84  2005/12/11 12:23:37  dsandras
 * Removed unused variable.
 *
 * Revision 2.83  2005/12/08 21:14:54  dsandras
 * Added function allowing to change the nat binding refresh timeout.
 *
 * Revision 2.82  2005/12/07 12:19:17  dsandras
 * Improved previous commit.
 *
 * Revision 2.81  2005/12/07 11:50:46  dsandras
 * Signal the registration failed in OnAuthenticationRequired with it happens
 * for a REGISTER or a SUBSCRIBE.
 *
 * Revision 2.80  2005/12/05 22:20:57  dsandras
 * Update the transport when the computer is behind NAT, using STUN, the IP
 * address has changed compared to the original transport and a registration
 * refresh must occur.
 *
 * Revision 2.79  2005/12/04 22:08:58  dsandras
 * Added possibility to provide an expire time when registering, if not
 * the default expire time for the endpoint will be used.
 *
 * Revision 2.78  2005/11/30 13:44:20  csoutheren
 * Removed compile warnings on Windows
 *
 * Revision 2.77  2005/11/28 19:07:56  dsandras
 * Moved OnNATTimeout to SIPInfo and use it for active conversations too.
 * Added E.164 support.
 *
 * Revision 2.76  2005/11/07 21:44:49  dsandras
 * Fixed origin of MESSAGE requests. Ignore refreshes for MESSAGE requests.
 *
 * Revision 2.75  2005/11/04 19:12:30  dsandras
 * Fixed OnReceivedResponse for MESSAGE PDU's.
 *
 * Revision 2.74  2005/11/04 13:40:52  dsandras
 * Initialize localPort (thanks Craig for pointing this)
 *
 * Revision 2.73  2005/11/02 22:17:56  dsandras
 * Take the port into account in TransmitSIPInfo. Added missing break statements.
 *
 * Revision 2.72  2005/10/30 23:01:29  dsandras
 * Added possibility to have a body for SIPInfo. Moved MESSAGE sending to SIPInfo for more efficiency during conversations.
 *
 * Revision 2.71  2005/10/30 15:55:55  dsandras
 * Added more calls to OnMessageFailed().
 *
 * Revision 2.70  2005/10/22 17:14:44  dsandras
 * Send an OPTIONS request periodically when STUN is being used to maintain the registrations binding alive.
 *
 * Revision 2.69  2005/10/22 10:29:29  dsandras
 * Increased refresh interval.
 *
 * Revision 2.68  2005/10/20 20:26:22  dsandras
 * Made the transactions handling thread-safe.
 *
 * Revision 2.67  2005/10/11 20:53:35  dsandras
 * Allow the expire field to be outside of the SIPURL formed by the contact field.
 *
 * Revision 2.66  2005/10/09 20:42:59  dsandras
 * Creating the connection thread can take more than 200ms. Send a provisional
 * response before creating it.
 *
 * Revision 2.65  2005/10/05 20:54:54  dsandras
 * Simplified some code.
 *
 * Revision 2.64  2005/10/03 21:46:20  dsandras
 * Fixed previous commit (sorry).
 *
 * Revision 2.63  2005/10/03 21:18:55  dsandras
 * Prevent looping at exit by removing SUBSCRIBE's and failed REGISTER from
 * the activeSIPInfo.
 *
 * Revision 2.62  2005/10/02 21:48:40  dsandras
 * - Use the transport port when STUN is being used when returning the contact address. That allows SIP proxies to regularly ping the UA so that the binding stays alive. As the REGISTER transport stays open, it permits to receive incoming calls when being behind a type of NAT supported by STUN without the need to forward any port (even not the listening port).
 * - Call OnFailed for other registration failure causes than 4xx.
 *
 * Revision 2.61  2005/10/02 17:48:15  dsandras
 * Added function to return the translated contact address of the endpoint.
 * Fixed GetRegisteredPartyName so that it returns a default SIPURL if we
 * have no registrations for the given host.
 *
 * Revision 2.60  2005/10/01 13:19:57  dsandras
 * Implemented back Blind Transfer.
 *
 * Revision 2.59  2005/09/27 16:17:36  dsandras
 * Remove the media streams on transfer.
 *
 * Revision 2.58  2005/09/21 19:50:30  dsandras
 * Cleaned code. Make use of the new SIP_PDU function that returns the correct address where to send responses to incoming requests.
 *
 * Revision 2.57  2005/09/20 17:02:57  dsandras
 * Adjust via when receiving an incoming request.
 *
 * Revision 2.56  2005/08/04 17:13:53  dsandras
 * More implementation of blind transfer.
 *
 * Revision 2.55  2005/06/02 13:18:02  rjongbloed
 * Fixed compiler warnings
 *
 * Revision 2.54  2005/05/25 18:36:07  dsandras
 * Fixed unregistration of all accounts when exiting.
 *
 * Revision 2.53  2005/05/23 20:14:00  dsandras
 * Added preliminary support for basic instant messenging.
 *
 * Revision 2.52  2005/05/13 12:47:51  dsandras
 * Instantly remove unregistration from the collection, and do not process
 * removed SIPInfo objects, thanks to Ted Szoczei.
 *
 * Revision 2.51  2005/05/12 19:44:12  dsandras
 * Fixed leak thanks to Ted Szoczei.
 *
 * Revision 2.50  2005/05/06 07:37:06  csoutheren
 * Various changed while working with SIP carrier
 *   - remove assumption that authentication realm is a domain name.
 *   - stopped rewrite of "To" field when proxy being used
 *   - fix Contact field in REGISTER to match actual port used when Symmetric NATin use
 *   - lots of formatting changes and cleanups
 *
 * Revision 2.49  2005/05/04 17:10:42  dsandras
 * Get rid of the extra parameters in the Via field before using it to change
 * the transport remote address.
 *
 * Revision 2.48  2005/05/03 20:42:33  dsandras
 * Unregister accounts when exiting the program.
 *
 * Revision 2.47  2005/05/02 19:30:36  dsandras
 * Do not use the realm in the comparison for the case when none is specified.
 *
 * Revision 2.46  2005/04/28 20:22:55  dsandras
 * Applied big sanity patch for SIP thanks to Ted Szoczei <tszoczei@microtronix.ca>.
 * Thanks a lot!
 *
 * Revision 2.45  2005/04/28 07:59:37  dsandras
 * Applied patch from Ted Szoczei to fix problem when answering to PDUs containing
 * multiple Via fields in the message header. Thanks!
 *
 * Revision 2.44  2005/04/26 19:50:15  dsandras
 * Fixed remoteAddress and user parameters in MWIReceived.
 * Added function to return the number of registered accounts.
 *
 * Revision 2.43  2005/04/25 21:12:51  dsandras
 * Use the correct remote address.
 *
 * Revision 2.42  2005/04/15 10:48:34  dsandras
 * Allow reading on the transport until there is an EOF or it becomes bad. Fixes interoperability problem with QSC.DE which is sending keep-alive messages, leading to a timeout (transport.good() fails, but the stream is still usable).
 *
 * Revision 2.41  2005/04/11 10:31:32  dsandras
 * Cleanups.
 *
 * Revision 2.40  2005/04/11 10:26:30  dsandras
 * Added SetUpTransfer similar to the one from in the H.323 part.
 *
 * Revision 2.39  2005/03/11 18:12:09  dsandras
 * Added support to specify the realm when registering. That way softphones already know what authentication information to use when required. The realm/domain can also be used in the From field.
 *
 * Revision 2.38  2005/02/21 12:20:05  rjongbloed
 * Added new "options list" to the OpalMediaFormat class.
 *
 * Revision 2.37  2005/02/19 22:48:48  dsandras
 * Added the possibility to register to several registrars and be able to do authenticated calls to each of them. Added SUBSCRIBE/NOTIFY support for Message Waiting Indications.
 *
 * Revision 2.36  2005/01/15 23:34:26  dsandras
 * Applied patch from Ted Szoczei to handle broken incoming connections.
 *
 * Revision 2.35  2005/01/09 03:42:46  rjongbloed
 * Fixed warning about unused parameter
 *
 * Revision 2.34  2004/12/27 22:20:17  dsandras
 * Added preliminary support for OPTIONS requests sent outside of a connection.
 *
 * Revision 2.33  2004/12/25 20:45:12  dsandras
 * Only fail a REGISTER when proxy authentication is required and has already
 * been done if the credentials are identical. Fixes registration refresh problem
 * where some registrars request authentication with a new nonce.
 *
 * Revision 2.32  2004/12/22 18:56:39  dsandras
 * Ignore ACK's received outside the context of a connection (e.g. when sending a non-2XX answer to an INVITE).
 *
 * Revision 2.31  2004/12/17 12:06:52  dsandras
 * Added error code to OnRegistrationFailed. Made Register/Unregister wait until the transaction is over. Fixed Unregister so that the SIPRegister is used as a pointer or the object is deleted at the end of the function and make Opal crash when transactions are cleaned. Reverted part of the patch that was sending authentication again when it had already been done on a Register.
 *
 * Revision 2.30  2004/12/12 13:42:31  dsandras
 * - Send back authentication when required when doing a REGISTER as it might be consecutive to an unregister.
 * - Update the registered variable when unregistering from a SIP registrar.
 * - Added OnRegistrationFailed () function to indicate when a registration failed. Call that function at various strategic places.
 *
 * Revision 2.29  2004/10/02 04:30:11  rjongbloed
 * Added unregister function for SIP registrar
 *
 * Revision 2.28  2004/08/22 12:27:46  rjongbloed
 * More work on SIP registration, time to live refresh and deregistration on exit.
 *
 * Revision 2.27  2004/08/20 12:54:48  rjongbloed
 * Fixed crash caused by double delete of registration transactions
 *
 * Revision 2.26  2004/08/18 13:04:56  rjongbloed
 * Fixed trying to start regitration if have empty string for domain/user
 *
 * Revision 2.25  2004/08/14 07:56:43  rjongbloed
 * Major revision to utilise the PSafeCollection classes for the connections and calls.
 *
 * Revision 2.24  2004/07/11 12:42:13  rjongbloed
 * Added function on endpoints to get the list of all media formats any
 *   connection the endpoint may create can support.
 *
 * Revision 2.23  2004/06/05 14:36:32  rjongbloed
 * Added functions to get registration URL.
 * Added ability to set proxy bu host/user/password strings.
 *
 * Revision 2.22  2004/04/26 06:30:35  rjongbloed
 * Added ability to specify more than one defualt listener for an endpoint,
 *   required by SIP which listens on both UDP and TCP.
 *
 * Revision 2.21  2004/04/26 05:40:39  rjongbloed
 * Added RTP statistics callback to SIP
 *
 * Revision 2.20  2004/04/25 08:46:08  rjongbloed
 * Fixed GNU compatibility
 *
 * Revision 2.19  2004/03/29 10:56:31  rjongbloed
 * Made sure SIP UDP socket is "promiscuous", ie the host/port being sent to may not
 *   be where packets come from.
 *
 * Revision 2.18  2004/03/14 11:32:20  rjongbloed
 * Changes to better support SIP proxies.
 *
 * Revision 2.17  2004/03/14 10:13:04  rjongbloed
 * Moved transport on SIP top be constructed by endpoint as any transport created on
 *   an endpoint can receive data for any connection.
 * Changes to REGISTER to support authentication
 *
 * Revision 2.16  2004/03/14 08:34:10  csoutheren
 * Added ability to set User-Agent string
 *
 * Revision 2.15  2004/03/13 06:51:32  rjongbloed
 * Alllowed for empty "username" in registration
 *
 * Revision 2.14  2004/03/13 06:32:18  rjongbloed
 * Fixes for removal of SIP and H.323 subsystems.
 * More registration work.
 *
 * Revision 2.13  2004/03/09 12:09:56  rjongbloed
 * More work on SIP register.
 *
 * Revision 2.12  2004/02/17 12:41:54  csoutheren
 * Use correct Contact field when behind NAT
 *
 * Revision 2.11  2003/12/20 12:21:18  rjongbloed
 * Applied more enhancements, thank you very much Ted Szoczei
 *
 * Revision 2.10  2003/03/24 04:32:58  robertj
 * SIP register must have a server address
 *
 * Revision 2.9  2003/03/06 03:57:47  robertj
 * IVR support (work in progress) requiring large changes everywhere.
 *
 * Revision 2.8  2002/10/09 04:27:44  robertj
 * Fixed memory leak on error reading PDU, thanks Ted Szoczei
 *
 * Revision 2.7  2002/09/12 06:58:34  robertj
 * Removed protocol prefix strings as static members as has problems with
 *   use in DLL environment.
 *
 * Revision 2.6  2002/07/04 07:41:47  robertj
 * Fixed memory/thread leak of transports.
 *
 * Revision 2.5  2002/04/10 03:15:17  robertj
 * Fixed using incorrect field as default reply address on receiving a respons PDU.
 *
 * Revision 2.4  2002/04/09 08:04:01  robertj
 * Fixed typo in previous patch!
 *
 * Revision 2.3  2002/04/09 07:18:33  robertj
 * Fixed the default return address for PDU requests not attached to connection.
 *
 * Revision 2.2  2002/04/05 10:42:04  robertj
 * Major changes to support transactions (UDP timeouts and retries).
 *
 * Revision 2.1  2002/02/01 04:53:01  robertj
 * Added (very primitive!) SIP support.
 *
 */

#include <ptlib.h>
#include <ptclib/enum.h>

#ifdef __GNUC__
#pragma implementation "sipep.h"
#endif


#include <sip/sipep.h>

#include <sip/sippdu.h>
#include <sip/sipcon.h>

#include <opal/manager.h>
#include <opal/call.h>

#include <sip/handlers.h>


#define new PNEW

 
////////////////////////////////////////////////////////////////////////////

SIPEndPoint::SIPEndPoint(OpalManager & mgr)
  : OpalEndPoint(mgr, "sip", CanTerminateCall),
    retryTimeoutMin(500),     // 0.5 seconds
    retryTimeoutMax(0, 4),    // 4 seconds
    nonInviteTimeout(0, 16),  // 16 seconds
    pduCleanUpTimeout(0, 5),  // 5 seconds
    inviteTimeout(0, 32),     // 32 seconds
    ackTimeout(0, 32),        // 32 seconds
    registrarTimeToLive(0, 0, 0, 1), // 1 hour
    notifierTimeToLive(0, 0, 0, 1), // 1 hour
    natBindingTimeout(0, 0, 1) // 1 minute
{
  defaultSignalPort = 5060;
  mimeForm = FALSE;
  maxRetries = 10;
  lastSentCSeq = 0;

  transactions.DisallowDeleteObjects();
  activeSIPHandlers.AllowDeleteObjects();

  natBindingTimer.SetNotifier(PCREATE_NOTIFIER(NATBindingRefresh));
  natBindingTimer.RunContinuous(natBindingTimeout);
  garbageTimer.SetNotifier(PCREATE_NOTIFIER(GarbageCollector));
  garbageTimer.RunContinuous(PTimeInterval(0, 1));

  natMethod = None;

  PTRACE(4, "SIP\tCreated endpoint.");
}


SIPEndPoint::~SIPEndPoint()
{
  while (activeSIPHandlers.GetSize()>0) {
    for (PSafePtr<SIPHandler> handler(activeSIPHandlers, PSafeReadOnly); handler != NULL; ++handler) {
      PString aor = handler->GetRemotePartyAddress ();
      if (handler->GetMethod() == SIP_PDU::Method_REGISTER 
          && handler->GetState() == SIPHandler::Subscribed) {
        Unregister(aor);
      }
      else { 
        handler->SetState(SIPHandler::Unsubscribed);
        handler->SetExpire(-1);
      }
    }

    activeSIPHandlers.DeleteObjectsToBeRemoved();
    PThread::Current()->Sleep(10); // Let GarbageCollect() do the cleanup
  }

  for (PINDEX i = 0; i < transactions.GetSize(); i++) {
    PWaitAndSignal m(transactionsMutex);
    SIPTransaction *transaction = transactions.GetAt(i);
    if (transaction)
      WaitForTransactionCompletion(transaction);
  }

  // Clean up
  transactions.RemoveAll();
  listeners.RemoveAll();

  // Stop timers before compiler destroys member objects
  natBindingTimer.Stop();
  garbageTimer.Stop();
  
  PWaitAndSignal m(transactionsMutex);
  PTRACE(4, "SIP\tDeleted endpoint.");
}


BOOL SIPEndPoint::NewIncomingConnection(OpalTransport * transport)
{
  PTRACE_IF(2, transport->IsReliable(), "SIP\tListening thread started.");

  transport->SetBufferSize(SIP_PDU::MaxSize);

  do {
    HandlePDU(*transport);
  } while (transport->IsOpen() && transport->IsReliable() && !transport->bad() && !transport->eof());

  PTRACE_IF(2, transport->IsReliable(), "SIP\tListening thread finished.");

  return TRUE;
}


void SIPEndPoint::TransportThreadMain(PThread &, INT param)
{
  PTRACE(4, "SIP\tRead thread started.");
  OpalTransport * transport = (OpalTransport *)param;

  do {
    HandlePDU(*transport);
  } while (transport->IsOpen() && !transport->bad() && !transport->eof());
  
  PTRACE(4, "SIP\tRead thread finished.");
}


void SIPEndPoint::NATBindingRefresh(PTimer &, INT)
{
  PTRACE(5, "SIP\tNAT Binding refresh started.");

  if (natMethod == None)
    return;

  for (PSafePtr<SIPHandler> handler(activeSIPHandlers, PSafeReadOnly); handler != NULL; ++handler) {

    OpalTransport & transport = handler->GetTransport();
    BOOL stunTransport = FALSE;

    if (!transport)
      return;

    stunTransport = (!transport.IsReliable() && GetManager().GetSTUN(transport.GetRemoteAddress().GetHostName()));

    if (!stunTransport)
      return;

    switch (natMethod) {

      case Options: 
        {
          PStringList emptyRouteSet;
          SIPOptions options(*this, transport, SIPURL(transport.GetRemoteAddress()).GetHostName());
          options.Write(transport, options.GetSendAddress(emptyRouteSet));
        }
        break;
      case EmptyRequest:
        {
          transport.Write("\r\n", 2);
        }
        break;
      default:
        break;
    }
  }

  PTRACE(5, "SIP\tNAT Binding refresh finished.");
}


void SIPEndPoint::GarbageCollector(PTimer &, INT)
{
  for (PINDEX i = activeSIPHandlers.GetSize(); i > 0; i--) {

    PSafePtr<SIPHandler> handler = activeSIPHandlers.GetAt (i-1, PSafeReadWrite);
    if (handler->CanBeDeleted()) { 
      activeSIPHandlers.RemoveAt(i-1);
    }
  }
  activeSIPHandlers.DeleteObjectsToBeRemoved();

  // Delete terminated transactions
  {
    PWaitAndSignal m(completedTransactionsMutex);
      
    for (PINDEX i = completedTransactions.GetSize(); i > 0; i--) {
      SIPTransaction & transaction = completedTransactions[i-1];
          
      if (transaction.IsTerminated()) {
        completedTransactions.RemoveAt(i-1);
      }
    }
  }

}



OpalTransport * SIPEndPoint::CreateTransport(const OpalTransportAddress & remoteAddress,
                                             const OpalTransportAddress & localAddress)
{
  OpalTransport * transport = NULL;
	
  for (PINDEX i = 0; i < listeners.GetSize(); i++) {
    OpalTransportAddress binding = listeners[i].GetLocalAddress();
    if (binding.Left(binding.Find('$')) *= remoteAddress.Left(remoteAddress.Find('$'))) {
      transport = listeners[i].CreateTransport(localAddress);
      break;
    }
  }

  if (transport == NULL) {
    // No compatible listeners, can't use their binding
    transport = remoteAddress.CreateTransport(*this, OpalTransportAddress::NoBinding);
    if(transport == NULL) {
      PTRACE(1, "SIP\tCould not create transport for " << remoteAddress);
      return NULL;
    }
  }

  transport->SetRemoteAddress(remoteAddress);
  PTRACE(4, "SIP\tCreated transport " << *transport);

  transport->SetBufferSize(SIP_PDU::MaxSize);
  if (!transport->ConnectTo(remoteAddress)) {
    PTRACE(1, "SIP\tCould not connect to " << remoteAddress << " - " << transport->GetErrorText());
    transport->CloseWait();
    delete transport;
    return NULL;
  }

  transport->SetPromiscuous(OpalTransport::AcceptFromAny);

  if (transport->IsReliable())
    transport->AttachThread(PThread::Create(PCREATE_NOTIFIER(TransportThreadMain),
                                            (INT)transport,
                                            PThread::NoAutoDeleteThread,
                                            PThread::NormalPriority,
                                            "SIP Transport:%x"));
  return transport;
}


void SIPEndPoint::HandlePDU(OpalTransport & transport)
{
  // create a SIP_PDU structure, then get it to read and process PDU
  SIP_PDU * pdu = new SIP_PDU;

  PTRACE(4, "SIP\tWaiting for PDU on " << transport);
  if (pdu->Read(transport)) {
    if (OnReceivedPDU(transport, pdu)) 
      return;
  }
  else if (transport.good()) {
    PTRACE(2, "SIP\tMalformed request received on " << transport);
    SendResponse(SIP_PDU::Failure_BadRequest, transport, *pdu);
  }

  delete pdu;
}


BOOL SIPEndPoint::MakeConnection(OpalCall & call,
                                 const PString & _remoteParty,
                                 void * userData,
                                 unsigned int options,
                                 OpalConnection::StringOptions * stringOptions)
{
  PString remoteParty;
  
  if (_remoteParty.Find("sip:") != 0)
    return FALSE;
  
  ParsePartyName(_remoteParty, remoteParty);

  PStringStream callID;
  OpalGloballyUniqueID id;
  callID << id << '@' << PIPSocket::GetHostName();
  SIPConnection * connection = CreateConnection(call, callID, userData, remoteParty, NULL, NULL, options, stringOptions);
  if (!AddConnection(connection))
    return FALSE;

  // If we are the A-party then need to initiate a call now in this thread. If
  // we are the B-Party then SetUpConnection() gets called in the context of
  // the A-party thread.
  if (call.GetConnection(0) == (OpalConnection*)connection)
    connection->SetUpConnection();

  return TRUE;
}


OpalMediaFormatList SIPEndPoint::GetMediaFormats() const
{
  return OpalMediaFormat::GetAllRegisteredMediaFormats();
}


BOOL SIPEndPoint::IsAcceptedAddress(const SIPURL & /*toAddr*/)
{
  return TRUE;
}


SIPConnection * SIPEndPoint::CreateConnection(OpalCall & call,
                                              const PString & token,
                                              void * /*userData*/,
                                              const SIPURL & destination,
                                              OpalTransport * transport,
                                              SIP_PDU * /*invite*/,
                                              unsigned int options,
                                              OpalConnection::StringOptions * stringOptions)
{
  return new SIPConnection(call, *this, token, destination, transport, options, stringOptions);
}


BOOL SIPEndPoint::SetupTransfer(const PString & token,  
				const PString & /*callIdentity*/, 
				const PString & _remoteParty,  
				void * userData)
{
  PString remoteParty;
  // Make a new connection
  PSafePtr<OpalConnection> otherConnection = 
    GetConnectionWithLock(token, PSafeReference);
  if (otherConnection == NULL) {
    return FALSE;
  }
  
  OpalCall & call = otherConnection->GetCall();
  
  call.RemoveMediaStreams();
  
  ParsePartyName(_remoteParty, remoteParty);

  PStringStream callID;
  OpalGloballyUniqueID id;
  callID << id << '@' << PIPSocket::GetHostName();
  SIPConnection * connection = CreateConnection(call, callID, userData, remoteParty, NULL, NULL);
  if (!AddConnection(connection))
    return FALSE;

  call.OnReleased(*otherConnection);
  
  connection->SetUpConnection();
  otherConnection->Release(OpalConnection::EndedByCallForwarded);

  return TRUE;
}


BOOL SIPEndPoint::ForwardConnection(SIPConnection & connection,  
				    const PString & forwardParty)
{
  OpalCall & call = connection.GetCall();
  
  PStringStream callID;
  OpalGloballyUniqueID id;
  callID << id << '@' << PIPSocket::GetHostName();

  SIPConnection * conn = CreateConnection(call, callID, NULL, forwardParty, NULL, NULL);
  if (!AddConnection(conn))
    return FALSE;

  call.OnReleased(connection);
  
  conn->SetUpConnection();
  connection.Release(OpalConnection::EndedByCallForwarded);

  return TRUE;
}

BOOL SIPEndPoint::OnReceivedPDU(OpalTransport & transport, SIP_PDU * pdu)
{
  // Adjust the Via list 
  if (pdu && pdu->GetMethod() != SIP_PDU::NumMethods)
    pdu->AdjustVia(transport);

  // Find a corresponding connection
  PSafePtr<SIPConnection> connection = GetSIPConnectionWithLock(pdu->GetMIME().GetCallID(), PSafeReadOnly);
  if (connection != NULL) {
    connection->QueuePDU(pdu);
    return TRUE;
  }
  
  switch (pdu->GetMethod()) {
    case SIP_PDU::NumMethods :
      {
        PWaitAndSignal m(transactionsMutex);
        SIPTransaction * transaction = transactions.GetAt(pdu->GetTransactionID());
        if (transaction != NULL)
          transaction->OnReceivedResponse(*pdu);
      }
      break;

    case SIP_PDU::Method_INVITE :
      return OnReceivedINVITE(transport, pdu);

    case SIP_PDU::Method_REGISTER :
    case SIP_PDU::Method_SUBSCRIBE :
      {
        SendResponse(SIP_PDU::Failure_MethodNotAllowed, transport, *pdu);
        break;
      }

    case SIP_PDU::Method_NOTIFY :
       return OnReceivedNOTIFY(transport, *pdu);
       break;

    case SIP_PDU::Method_MESSAGE :
      {
        OnReceivedMESSAGE(transport, *pdu);
        SendResponse(SIP_PDU::Successful_OK, transport, *pdu);
        break;
      }
   
    case SIP_PDU::Method_OPTIONS :
     {
       SendResponse(SIP_PDU::Successful_OK, transport, *pdu);
       break;
     }
    case SIP_PDU::Method_ACK :
      {
	// If we receive an ACK outside of the context of a connection,
	// ignore it.
      }
      break;

    default :
      {
        SendResponse(SIP_PDU::Failure_TransactionDoesNotExist, transport, *pdu);
      }
  }

  return FALSE;
}


void SIPEndPoint::OnReceivedResponse(SIPTransaction & transaction, SIP_PDU & response)
{
  PSafePtr<SIPHandler> handler = NULL;
 
  if (transaction.GetMethod() == SIP_PDU::Method_REGISTER
      || transaction.GetMethod() == SIP_PDU::Method_SUBSCRIBE
      || transaction.GetMethod() == SIP_PDU::Method_PUBLISH
      || transaction.GetMethod() == SIP_PDU::Method_MESSAGE) {

    PString callID = transaction.GetMIME().GetCallID ();

    // Have a response to the REGISTER/SUBSCRIBE/MESSAGE, 
    handler = activeSIPHandlers.FindSIPHandlerByCallID (callID, PSafeReadOnly);
    if (handler == NULL) 
      return;
  }

  switch (response.GetStatusCode()) {
    case SIP_PDU::Failure_UnAuthorised :
    case SIP_PDU::Failure_ProxyAuthenticationRequired :
      OnReceivedAuthenticationRequired(transaction, response);
      break;
    case SIP_PDU::Failure_RequestTimeout :
      if (handler != NULL) {
        handler->OnTransactionTimeout(transaction);
      }
      break;

    default :
      switch (response.GetStatusCode()/100) {
        case 1 :
          // Do nothing on 1xx
          break;
        case 2 :
          OnReceivedOK(transaction, response);
          break;
        default :
	  if (handler != NULL) {
	    // Failure for a SUBSCRIBE/REGISTER/PUBLISH/MESSAGE 
	    handler->OnFailed (response.GetStatusCode());
	  }
      }
  }
}


BOOL SIPEndPoint::OnReceivedINVITE(OpalTransport & transport, SIP_PDU * request)
{
  SIPMIMEInfo & mime = request->GetMIME();

  // parse the incoming To field, and check if we accept incoming calls for this address
  SIPURL toAddr(mime.GetTo());
  if (!IsAcceptedAddress(toAddr)) {
    PTRACE(2, "SIP\tIncoming INVITE from " << request->GetURI() << " for unknown address " << toAddr);
    SendResponse(SIP_PDU::Failure_NotFound, transport, *request);
    return FALSE;
  }
  
  // send provisional response here because creating the connection can take a long time
  // on some systems
  SendResponse(SIP_PDU::Information_Trying, transport, *request);

  // ask the endpoint for a connection
  SIPConnection *connection = CreateConnection(*manager.CreateCall(NULL),
                                               mime.GetCallID(),
                                               NULL,
                                               request->GetURI(),
                                               CreateTransport(transport.GetRemoteAddress(), transport.GetLocalAddress()),
                                               request);
  if (!AddConnection(connection)) {
    PTRACE(1, "SIP\tFailed to create SIPConnection for INVITE from " << request->GetURI() << " for " << toAddr);
    SendResponse(SIP_PDU::Failure_NotFound, transport, *request);
    return FALSE;
  }

  // Get the connection to handle the rest of the INVITE
  connection->QueuePDU(request);
  return TRUE;
}


void SIPEndPoint::OnReceivedAuthenticationRequired(SIPTransaction & transaction, SIP_PDU & response)
{
  PSafePtr<SIPHandler> realm_handler = NULL;
  PSafePtr<SIPHandler> callid_handler = NULL;
  SIPTransaction * request = NULL;
  SIPAuthentication auth;
  
  BOOL isProxy = 
    response.GetStatusCode() == SIP_PDU::Failure_ProxyAuthenticationRequired;
  PString lastNonce;
  PString lastUsername;
  
#if PTRACING
  const char * proxyTrace = isProxy ? "Proxy " : "";
#endif
  PTRACE(3, "SIP\tReceived " << proxyTrace << "Authentication Required response");
  
  // Only support REGISTER and SUBSCRIBE for now
  if (transaction.GetMethod() != SIP_PDU::Method_REGISTER
      && transaction.GetMethod() != SIP_PDU::Method_SUBSCRIBE
      && transaction.GetMethod() != SIP_PDU::Method_MESSAGE) {
    PTRACE(1, "SIP\tCannot do " << proxyTrace << "Authentication Required for non REGISTER, SUBSCRIBE or MESSAGE");
    return;
  }
  
  // Try to find authentication information for the given call ID
  callid_handler = activeSIPHandlers.FindSIPHandlerByCallID(response.GetMIME().GetCallID(), PSafeReadOnly);

  if (!callid_handler)
    return;
  
  // Received authentication required response, try to find authentication
  // for the given realm
  if (!auth.Parse(response.GetMIME()(isProxy 
				     ? "Proxy-Authenticate"
				     : "WWW-Authenticate"),
		  isProxy)) {
    callid_handler->OnFailed(SIP_PDU::Failure_UnAuthorised);
    return;
  }

  // Try to find authentication information for the requested realm
  // That realm is specified when registering
  realm_handler = activeSIPHandlers.FindSIPHandlerByAuthRealm(auth.GetAuthRealm(), auth.GetUsername().IsEmpty()?SIPURL(response.GetMIME().GetFrom()).GetUserName():auth.GetUsername(), PSafeReadOnly);

  // No authentication information found for the realm, 
  // use what we have for the given CallID
  // and update the realm
  if (realm_handler == NULL) {
    realm_handler = callid_handler;
    if (!auth.GetAuthRealm().IsEmpty())
      realm_handler->SetAuthRealm(auth.GetAuthRealm());
    PTRACE(3, "SIP\tUpdated realm to " << auth.GetAuthRealm());
  }
  
  // No realm handler, weird
  if (realm_handler == NULL) {
    PTRACE(1, "SIP\tNo Authentication handler found for that realm, authentication impossible");
    return;
  }
  
  if (realm_handler->GetAuthentication().IsValid()) {
    lastUsername = realm_handler->GetAuthentication().GetUsername();
    lastNonce = realm_handler->GetAuthentication().GetNonce();
  }

  if (!realm_handler->GetAuthentication().Parse(response.GetMIME()(isProxy 
								? "Proxy-Authenticate"
								: "WWW-Authenticate"),
					     isProxy)) {
    callid_handler->OnFailed(SIP_PDU::Failure_UnAuthorised);
    return;
  }
  
  // Already sent handler for that callID. Check if params are different
  // from the ones found for the given realm
  if (callid_handler->GetAuthentication().IsValid()
      && lastUsername == callid_handler->GetAuthentication().GetUsername ()
      && lastNonce == callid_handler->GetAuthentication().GetNonce ()
      && callid_handler->GetState() == SIPHandler::Subscribing) {
    PTRACE(2, "SIP\tAlready done REGISTER/SUBSCRIBE for " << proxyTrace << "Authentication Required");
    callid_handler->OnFailed(SIP_PDU::Failure_UnAuthorised);
    return;
  }
  
  // Abort after some unsuccesful authentication attempts. This is required since
  // some implementations return "401 Unauthorized" with a different nonce at every
  // time.
  unsigned authenticationAttempts = callid_handler->GetAuthenticationAttempts();
  if(authenticationAttempts < 10) {
    authenticationAttempts++;
    callid_handler->SetAuthenticationAttempts(authenticationAttempts);
  } else {
    PTRACE(1, "SIP\tAborting after " << authenticationAttempts << " attempts to REGISTER/SUBSCRIBE");
    callid_handler->OnFailed(SIP_PDU::Failure_UnAuthorised);
    return;
  }

  // Restart the transaction with new authentication handler
  request = callid_handler->CreateTransaction(transaction.GetTransport());
  if (!realm_handler->GetAuthentication().Authorise(*request)) {
    // don't send again if no authentication handler available
    delete request;
    callid_handler->OnFailed(SIP_PDU::Failure_UnAuthorised);
    return;
  }
  // Section 8.1.3.5 of RFC3261 tells that the authenticated
  // request SHOULD have the same value of the Call-ID, To and From.
  request->GetMIME().SetFrom(transaction.GetMIME().GetFrom());
  if (!request->Start()) {
    delete request;
    PTRACE(1, "SIP\tCould not restart REGISTER/SUBSCRIBE for Authentication Required");
  }
}


void SIPEndPoint::OnReceivedOK(SIPTransaction & /*transaction*/, SIP_PDU & response)
{
  PSafePtr<SIPHandler> handler = NULL;

  handler = activeSIPHandlers.FindSIPHandlerByCallID(response.GetMIME().GetCallID(), PSafeReadOnly);
 
  if (handler == NULL) 
    return;
  
  // reset the number of unsuccesful authentication attempts
  handler->SetAuthenticationAttempts(0);
  handler->OnReceivedOK(response);
}
    

void SIPEndPoint::OnTransactionTimeout(SIPTransaction & transaction)
{
  PSafePtr<SIPHandler> handler = NULL;

  handler = activeSIPHandlers.FindSIPHandlerByCallID(transaction.GetMIME().GetCallID(), PSafeReadOnly);
  if (handler == NULL) 
    return;
  
  handler->OnTransactionTimeout(transaction);
}


BOOL SIPEndPoint::OnReceivedNOTIFY (OpalTransport & transport, SIP_PDU & pdu)
{
  PSafePtr<SIPHandler> handler = NULL;
  PCaselessString state;
  SIPSubscribe::SubscribeType event;
  
  PTRACE(3, "SIP\tReceived NOTIFY");
  
  if (pdu.GetMIME().GetEvent().Find("message-summary") != P_MAX_INDEX)
    event = SIPSubscribe::MessageSummary;
  else if (pdu.GetMIME().GetEvent().Find("presence") != P_MAX_INDEX)
    event = SIPSubscribe::Presence;
  else {

    // Only support MWI and presence Subscribe for now, 
    // other events are unrequested
    SendResponse(SIP_PDU::Failure_BadEvent, transport, pdu);

    return FALSE;
  }

    
  // A NOTIFY will have the same CallID than the SUBSCRIBE request
  // it corresponds to
  handler = activeSIPHandlers.FindSIPHandlerByCallID(pdu.GetMIME().GetCallID(), PSafeReadOnly);
  if (handler == NULL) {

    if (event == SIPSubscribe::Presence) {
      PTRACE(3, "SIP\tCould not find a SUBSCRIBE corresponding to the NOTIFY");
      SendResponse(SIP_PDU::Failure_TransactionDoesNotExist, transport, pdu);
      return FALSE;
    }
    if (event == SIPSubscribe::MessageSummary) {
      PTRACE(3, "SIP\tCould not find a SUBSCRIBE corresponding to the NOTIFY : Work around Asterisk bug");
      SIPURL url_from (pdu.GetMIME().GetFrom());
      SIPURL url_to (pdu.GetMIME().GetTo());
      PString to = url_to.GetUserName() + "@" + url_from.GetHostName();
      handler = activeSIPHandlers.FindSIPHandlerByUrl(to, SIP_PDU::Method_SUBSCRIBE, event, PSafeReadOnly);
      if (handler == NULL) {
        SendResponse(SIP_PDU::Failure_TransactionDoesNotExist, transport, pdu);
        return FALSE;
      }
    }
  }
  if (!handler->OnReceivedNOTIFY(pdu))
    return FALSE;

  return TRUE;
}


void SIPEndPoint::OnReceivedMESSAGE(OpalTransport & /*transport*/, 
				    SIP_PDU & pdu)
{
  PString from = pdu.GetMIME().GetFrom();
  PINDEX j = from.Find (';');
  if (j != P_MAX_INDEX)
    from = from.Left(j); // Remove all parameters
  j = from.Find ('<');
  if (j != P_MAX_INDEX && from.Find ('>') == P_MAX_INDEX)
    from += '>';

  OnMessageReceived(from, pdu.GetEntityBody());
}


void SIPEndPoint::OnRegistrationFailed(const PString & /*aor*/, 
				       SIP_PDU::StatusCodes /*reason*/, 
				       BOOL /*wasRegistering*/)
{
}
    

void SIPEndPoint::OnRegistered(const PString & /*aor*/, 
			       BOOL /*wasRegistering*/)
{
}


BOOL SIPEndPoint::IsRegistered(const PString & url) 
{
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByUrl(url, SIP_PDU::Method_REGISTER, PSafeReadOnly);

  if (handler == NULL)
    return FALSE;
  
  return (handler->GetState() == SIPHandler::Subscribed);
}


BOOL SIPEndPoint::IsSubscribed(SIPSubscribe::SubscribeType type,
                               const PString & to) 
{
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByUrl(to, SIP_PDU::Method_SUBSCRIBE, type, PSafeReadOnly);

  if (handler == NULL)
    return FALSE;

  return (handler->GetState() == SIPHandler::Subscribed);
}

BOOL SIPEndPoint::Register(
      const PString & host,
      const PString & user,
      const PString & authName,
      const PString & password,
      const PString & authRealm,
      unsigned expire,
      const PTimeInterval & minRetryTime, 
      const PTimeInterval & maxRetryTime
)
{
  PString aor;
  if (user.Find('@') != P_MAX_INDEX)
    aor = user;
  else
    aor = user + '@' + host;

  if (expire == 0)
    expire = GetRegistrarTimeToLive().GetSeconds();
  return Register(expire, aor, authName, password, authRealm, minRetryTime, maxRetryTime);
}

BOOL SIPEndPoint::Register(unsigned expire,
                           const PString & aor,
                           const PString & authName,
                           const PString & password,
                           const PString & realm,
                           const PTimeInterval & minRetryTime, 
                           const PTimeInterval & maxRetryTime)
{
  PSafePtr<SIPHandler> handler = NULL;
  
  // Create the SIPHandler structure
  handler = activeSIPHandlers.FindSIPHandlerByUrl(aor, SIP_PDU::Method_REGISTER, PSafeReadOnly);
  
  // If there is already a request with this URL and method, 
  // then update it with the new information
  if (handler != NULL) {
    if (!password.IsEmpty())
      handler->SetPassword(password); // Adjust the password if required 
    if (!realm.IsEmpty())
      handler->SetAuthRealm(realm);   // Adjust the realm if required 
    if (!authName.IsEmpty())
      handler->SetAuthUser(authName); // Adjust the authUser if required 
    handler->SetExpire(expire);      // Adjust the expire field
  } 
  // Otherwise create a new request with this method type
  else {
    handler = CreateRegisterHandler(aor, authName, password, realm, expire, minRetryTime, maxRetryTime);
    activeSIPHandlers.Append(handler);
  }

  if (!handler->SendRequest()) {
    activeSIPHandlers.Remove(handler);
    return FALSE;
  }

  return TRUE;
}

SIPRegisterHandler * SIPEndPoint::CreateRegisterHandler(const PString & aor,
                                                        const PString & authName, 
                                                        const PString & password, 
                                                        const PString & realm,
                                                        int expire,
                                                        const PTimeInterval & minRetryTime, 
                                                        const PTimeInterval & maxRetryTime)
{
  return new SIPRegisterHandler(*this, aor, authName, password, realm, expire, minRetryTime, maxRetryTime);
}

BOOL SIPEndPoint::Unregister(const PString & aor)
{
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByUrl (aor, SIP_PDU::Method_REGISTER, PSafeReadOnly);
  if (handler == NULL) {
    PTRACE(1, "SIP\tCould not find active REGISTER for " << aor);
    return FALSE;
  }

  if (!handler->GetState() == SIPHandler::Subscribed) {
    handler->SetExpire(-1);
    return FALSE;
  }
      
  return Register(0, aor);
}


BOOL SIPEndPoint::Subscribe(SIPSubscribe::SubscribeType & type,
                            unsigned expire,
                            const PString & to)
{
  // Create the SIPHandler structure
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByUrl(to, SIP_PDU::Method_SUBSCRIBE, type, PSafeReadOnly);
  
  // If there is already a request with this URL and method, 
  // then update it with the new information
  if (handler != NULL) {
    handler->SetExpire(expire);      // Adjust the expire field
  } 
  // Otherwise create a new request with this method type
  else {
    handler = new SIPSubscribeHandler(*this, 
                                      type,
                                      to,
                                      expire);
    activeSIPHandlers.Append(handler);
  }

  if (!handler->SendRequest()) {
    handler->SetExpire(-1);
    return FALSE;
  }

  return TRUE;
}


BOOL SIPEndPoint::Unsubscribe(SIPSubscribe::SubscribeType & type,
                              const PString & to)
{
  // Create the SIPHandler structure
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByUrl(to, SIP_PDU::Method_SUBSCRIBE, type, PSafeReadOnly);

  if (handler == NULL) {
    PTRACE(1, "SIP\tCould not find active SUBSCRIBE for " << to);
    return FALSE;
  }

  if (!handler->GetState() == SIPHandler::Subscribed) {
    handler->SetExpire(-1);
    return FALSE;
  }
      
  return Subscribe(type, 0, to);
}



BOOL SIPEndPoint::Message (const PString & to, 
                           const PString & body)
{
  // Create the SIPHandler structure
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByUrl(to, SIP_PDU::Method_MESSAGE, PSafeReadOnly);
  
  // Otherwise create a new request with this method type
  if (handler != NULL)
    handler->SetBody(body);
  else {
    handler = new SIPMessageHandler(*this, 
                                    to,
                                    body);
    activeSIPHandlers.Append(handler);
  }

  if (!handler->SendRequest()) {
    handler->SetExpire(-1);
    return FALSE;
  }

  return TRUE;
}


BOOL SIPEndPoint::Publish(const PString & to,
                          const PString & body,
                          unsigned expire)
{
  // Create the SIPHandler structure
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByUrl(to, SIP_PDU::Method_PUBLISH, PSafeReadOnly);

  // Otherwise create a new request with this method type
  if (handler != NULL)
    handler->SetBody(body);
  else {
    handler = new SIPPublishHandler(*this,
                                    to,
                                    body,
                                    expire);
    activeSIPHandlers.Append(handler);

    if (!handler->SendRequest()) {
      handler->SetExpire(-1);
      return FALSE;
    }
  }

  return TRUE;
}


BOOL SIPEndPoint::Ping(const PString & to)
{
  // Create the SIPHandler structure
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByUrl(to, SIP_PDU::Method_PING, PSafeReadOnly);

  // Otherwise create a new request with this method type
  if (handler == NULL) {
    handler = new SIPPingHandler(*this,
                                 to);
  }

  if (!handler->SendRequest()) {
    handler->SetExpire(-1);
    return FALSE;
  }

  return TRUE;
}


void SIPEndPoint::OnMessageReceived (const SIPURL & /*from*/,
				     const PString & /*body*/)
{
}


void SIPEndPoint::OnMWIReceived (const PString & /*to*/,
				 SIPSubscribe::MWIType /*type*/,
				 const PString & /*msgs*/)
{
}


void SIPEndPoint::OnPresenceInfoReceived (const PString & /*user*/,
                                          const PString & /*basic*/,
                                          const PString & /*note*/)
{
}


void SIPEndPoint::OnMessageFailed(const SIPURL & /* messageUrl */,
				  SIP_PDU::StatusCodes /* reason */)
{
}


void SIPEndPoint::ParsePartyName(const PString & remoteParty, PString & party)
{
  party = remoteParty;
  
#if P_DNS
  // if there is no '@', and then attempt to use ENUM
  if (remoteParty.Find('@') == P_MAX_INDEX) {

    // make sure the number has only digits
    PString e164 = remoteParty;
    if (e164.Left(4) *= "sip:")
      e164 = e164.Mid(4);
    PINDEX i;
    for (i = 0; i < e164.GetLength(); ++i)
      if (!isdigit(e164[i]) && (i != 0 || e164[0] != '+'))
	      break;
    if (i >= e164.GetLength()) {
      PString str;
      if (PDNS::ENUMLookup(e164, "E2U+SIP", str)) {
	      PTRACE(4, "SIP\tENUM converted remote party " << remoteParty << " to " << str);
	      party = str;
      }
    }
  }
#endif
}


void SIPEndPoint::SetProxy(const PString & hostname,
                           const PString & username,
                           const PString & password)
{
  PStringStream str;
  if (!hostname) {
    str << "sip:";
    if (!username) {
      str << username;
      if (!password)
        str << ':' << password;
      str << '@';
    }
    str << hostname;
  }
  proxy = str;
}


void SIPEndPoint::SetProxy(const SIPURL & url) 
{ 
  proxy = url; 
}


PString SIPEndPoint::GetUserAgent() const 
{
  return userAgentString;
}


BOOL SIPEndPoint::WaitForTransactionCompletion(SIPTransaction * transaction)
{
  transaction->WaitForCompletion();
  BOOL success = !transaction->IsFailed();
  AddCompletedTransaction(transaction);
  return success;
}


BOOL SIPEndPoint::GetAuthentication(const PString & realm, SIPAuthentication &auth) 
{
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByAuthRealm(realm, PString::Empty(), PSafeReadOnly);
  if (handler == NULL)
    return FALSE;

  auth = handler->GetAuthentication();

  return TRUE;
}


SIPURL SIPEndPoint::GetRegisteredPartyName(const SIPURL & url)
{
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByDomain(url.GetHostName(), SIP_PDU::Method_REGISTER, PSafeReadOnly);
  
  if (handler == NULL) 
    return GetDefaultRegisteredPartyName();

  return handler->GetTargetAddress();
}


SIPURL SIPEndPoint::GetDefaultRegisteredPartyName()
{
  OpalTransportAddress addr(PIPSocket::GetHostName(), GetDefaultSignalPort(), "udp");
  SIPURL rpn(GetDefaultLocalPartyName(), addr, GetDefaultSignalPort());
  rpn.SetDisplayName(GetDefaultDisplayName());
  return rpn;
}


SIPURL SIPEndPoint::GetContactURL(const OpalTransport &transport, const PString & userName, const PString & host)
{
  PSafePtr<SIPHandler> handler = activeSIPHandlers.FindSIPHandlerByDomain(host, SIP_PDU::Method_REGISTER, PSafeReadOnly);
  return GetLocalURL(handler != NULL ? handler->GetTransport() : transport, userName);
}


SIPURL SIPEndPoint::GetLocalURL(const OpalTransport &transport, const PString & userName)
{
  PIPSocket::Address ip(PIPSocket::GetDefaultIpAny());
  OpalTransportAddress contactAddress = transport.GetLocalAddress();
  WORD contactPort = GetDefaultSignalPort();
  if (transport.IsOpen())
    transport.GetLocalAddress().GetIpAndPort(ip, contactPort);
  else {
    for (PINDEX i = 0; i < listeners.GetSize(); i++) {
      OpalTransportAddress binding = listeners[i].GetLocalAddress();
      if (transport.IsCompatibleTransport(binding)) {
        binding.GetIpAndPort(ip, contactPort);
        break;
      }
    }
  }

  PIPSocket::Address localIP;
  WORD localPort;
  
  if (contactAddress.GetIpAndPort(localIP, localPort)) {
    PIPSocket::Address remoteIP;
    if (transport.GetRemoteAddress().GetIpAddress(remoteIP)) {
      GetManager().TranslateIPAddress(localIP, remoteIP); 	 
      contactPort = localPort;
      contactAddress = OpalTransportAddress(localIP, contactPort, "udp");
    }
  }

  SIPURL contact(userName, contactAddress, contactPort);

  return contact;
}


BOOL SIPEndPoint::SendResponse(SIP_PDU::StatusCodes code, OpalTransport & transport, SIP_PDU & pdu)
{
  SIP_PDU response(pdu, code);
  PString username = SIPURL(response.GetMIME().GetTo()).GetUserName();
  response.GetMIME().SetContact(GetLocalURL(transport, username));
  return response.Write(transport, pdu.GetViaAddress(*this));
}


void SIPEndPoint::OnRTPStatistics(const SIPConnection & /*connection*/,
                                  const RTP_Session & /*session*/) const
{
}


// End of file ////////////////////////////////////////////////////////////////
