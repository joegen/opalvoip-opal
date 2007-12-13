/*
 * sipcon.cxx
 *
 * Session Initiation Protocol connection.
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
 * $Log: sipcon.cxx,v $
 * Revision 2.265  2007/10/10 01:24:44  rjongbloed
 * Removed redundent code and added extra log for if SRV was used.
 *
 * Revision 2.264  2007/10/03 23:59:05  rjongbloed
 * Fixed correct operation of DNS SRV lookups to RFC3263 specification,
 *   thanks to Will Hawkins and Kris Marsh for what needs to be done.
 *
 * Revision 2.263  2007/09/24 23:38:39  csoutheren
 * Fixed errors on 64 bit Linux
 *
 * Revision 2.262  2007/09/21 01:34:10  rjongbloed
 * Rewrite of SIP transaction handling to:
 *   a) use PSafeObject and safe collections
 *   b) only one database of transactions, remove connection copy
 *   c) fix timers not always firing due to bogus deadlock avoidance
 *   d) cleaning up only occurs in the existing garbage collection thread
 *   e) use of read/write mutex on endpoint list to avoid possible deadlock
 *
 * Revision 2.261  2007/09/20 23:47:31  rjongbloed
 * Fixed MaCarthy Boolean || operator from preventing offering video if has audio.
 *
 * Revision 2.260  2007/09/18 06:25:11  csoutheren
 * Ensure non-matching SDP is handled correctly
 *
 * Revision 2.259  2007/09/10 00:11:14  rjongbloed
 * AddedOpalMediaFormat::IsTransportable() function as better test than simply
 *   checking the payload type, condition is more complex.
 *
 * Revision 2.258  2007/09/05 14:03:37  csoutheren
 * Applied 1783448 - Removing media streams when releasing session
 * Thanks to Borko Jandras
 *
 * Revision 2.257  2007/08/31 11:36:13  csoutheren
 * Fix usage of GetOtherPartyConnection without PSafePtr
 *
 * Revision 2.256  2007/08/24 07:49:22  csoutheren
 * Get access to called URL
 *
 * Revision 2.255  2007/08/24 06:38:53  csoutheren
 * Add way to get empty DisplayName without always using default
 *
 * Revision 2.254  2007/08/22 09:02:19  csoutheren
 * Allow setting of explicit From field in SIP
 *
 * Revision 2.253  2007/08/13 04:02:49  csoutheren
 * Allow override of SIP display name using StringOptions
 * Normalise setting of local party name
 *
 * Revision 2.252  2007/07/27 05:56:10  csoutheren
 * Fix problem with SIP NAT detection
 *
 * Revision 2.251  2007/07/26 01:05:44  rjongbloed
 * Code tidy up.
 *
 * Revision 2.250  2007/07/25 03:42:55  csoutheren
 * Fix extraction of remote signal address from Contact field
 *
 * Revision 2.249  2007/07/25 01:16:03  csoutheren
 * Fix problem with reinvite and loop detection
 *
 * Revision 2.248  2007/07/24 13:46:45  rjongbloed
 * Fixed correct selection of interface after forked INVITE reply arrives on bundled socket.
 *
 * Revision 2.247  2007/07/23 06:34:19  csoutheren
 * Stop re-creation of new RTP sessions after SIP authentication fails
 * Do not create video RTP sessions if no video media formats
 *
 * Revision 2.246  2007/07/23 04:58:57  csoutheren
 * Always check return value of OnOpenSourceMediaStreams
 *
 * Revision 2.245  2007/07/22 13:02:14  rjongbloed
 * Cleaned up selection of registered name usage of URL versus host name.
 *
 * Revision 2.244  2007/07/22 12:25:24  rjongbloed
 * Removed redundent mutex
 *
 * Revision 2.243  2007/07/10 06:46:32  csoutheren
 * Remove all vestiges of sentTrying variable and fix transmission of 180 Trying
 * when using AnswerCallDeferred
 *
 * Revision 2.242  2007/07/08 13:11:25  rjongbloed
 * Fixed accepting re-INVITE from the side of call that did not send the original INVITE.
 *
 * Revision 2.241  2007/07/06 07:01:37  rjongbloed
 * Fixed borken re-INVITE handling (Hold and Retrieve)
 *
 * Revision 2.240  2007/07/05 06:25:14  rjongbloed
 * Fixed GNU compiler warnings.
 *
 * Revision 2.239  2007/07/05 05:40:21  rjongbloed
 * Changes to correctly distinguish between INVITE types: normal, forked and re-INVITE
 *   these are all handled slightly differently for handling request and responses.
 * Tidied the translation of SIP status codes and OPAL call end types and Q.931 cause codes.
 * Fixed (accidental?) suppression of 180 responses on alerting.
 *
 * Revision 2.238  2007/06/30 00:14:17  csoutheren
 * Remove warning from unused variable
 *
 * Revision 2.237  2007/06/29 23:34:16  csoutheren
 * Add support for SIP 183 messages
 *
 * Revision 2.236  2007/06/29 06:59:58  rjongbloed
 * Major improvement to the "product info", normalising H.221 and User-Agent mechanisms.
 *
 * Revision 2.235  2007/06/28 12:08:26  rjongbloed
 * Simplified mutex strategy to avoid some wierd deadlocks. All locking of access
 *   to an OpalConnection must be via the PSafeObject locks.
 *
 * Revision 2.234  2007/06/27 18:19:49  csoutheren
 * Fix compile when video disabled
 *
 * Revision 2.233  2007/06/22 05:41:47  rjongbloed
 * Major codec API update:
 *   Automatically map OpalMediaOptions to SIP/SDP FMTP parameters.
 *   Automatically map OpalMediaOptions to H.245 Generic Capability parameters.
 *   Largely removed need to distinguish between SIP and H.323 codecs.
 *   New mechanism for setting OpalMediaOptions from within a plug in.
 *
 * Revision 2.232  2007/06/14 01:51:07  csoutheren
 * Remove warnings on Linux
 *
 * Revision 2.231  2007/06/10 08:55:12  rjongbloed
 * Major rework of how SIP utilises sockets, using new "socket bundling" subsystem.
 *
 * Revision 2.230  2007/06/09 15:40:05  dsandras
 * Fixed routing of ACK PDUs. Only an ACK sent for a non 2XX response needs
 * to copy the Route header from the original request.
 *
 * Revision 2.229  2007/06/05 21:37:28  dsandras
 * Use the route set directly from the PDU instead of using the route set
 * from the connection. Make sure the route set is being used when routing
 * all types of PDUs.
 *
 * Revision 2.228  2007/06/04 09:12:52  rjongbloed
 * Removed too tightly specified connection for I-Frame request, was only for
 *   PCSS connections which precludes gateways etc.
 * Also removed some redundent locking.
 *
 * Revision 2.227  2007/06/01 04:24:08  csoutheren
 * Added handling for SIP video update request as per
 * draft-levin-mmusic-xml-media-control-10.txt
 *
 * Revision 2.226  2007/05/23 20:53:40  dsandras
 * We should release the current session if no ACK is received after
 * an INVITE answer for a period of 64*T1. Don't trigger the ACK timer
 * when sending an ACK, only when not receiving one.
 *
 * Revision 2.225  2007/05/23 19:58:04  dsandras
 * Run garbage collector more often.
 *
 * Revision 2.224  2007/05/15 20:49:21  dsandras
 * Added various handlers to manage subscriptions for presence, message
 * waiting indications, registrations, state publishing,
 * message conversations, ...
 * Adds/fixes support for RFC3856, RFC3903, RFC3863, RFC3265, ...
 * Many improvements over the original SIPInfo code.
 * Code contributed by NOVACOM (http://www.novacom.be) thanks to
 * EuroWeb (http://www.euroweb.hu).
 *
 * Revision 2.223  2007/05/15 07:27:34  csoutheren
 * Remove deprecated  interface to STUN server in H323Endpoint
 * Change UseNATForIncomingCall to IsRTPNATEnabled
 * Various cleanups of messy and unused code
 *
 * Revision 2.222  2007/05/15 05:25:34  csoutheren
 * Add UseNATForIncomingCall override so applications can optionally implement their own NAT activation strategy
 *
 * Revision 2.221  2007/05/10 04:45:35  csoutheren
 * Change CSEQ storage to be an atomic integer
 * Fix hole in transaction mutex handling
 *
 * Revision 2.220  2007/05/09 01:41:46  csoutheren
 * Ensure CANCEL is sent if incoming call hungup before ACK is received
 *
 * Revision 2.219  2007/04/26 07:01:47  csoutheren
 * Add extra code to deal with getting media formats from connections early enough to do proper
 * gatewaying between calls. The SIP and H.323 code need to have the handing of the remote
 * and local formats standardized, but this will do for now
 *
 * Revision 2.218  2007/04/23 01:18:29  csoutheren
 * Removed warnings on Windows
 *
 * Revision 2.217  2007/04/21 11:05:56  dsandras
 * Fixed more interoperability problems due to bugs in Cisco Call Manager.
 * Do not clear calls if the video transport can not be established.
 * Only reinitialize the registrar transport if required (STUN is being used
 * and the public IP address has changed).
 *
 * Revision 2.216  2007/04/20 06:48:06  csoutheren
 * Fix typo
 *
 * Revision 2.215  2007/04/19 06:34:12  csoutheren
 * Applied 1703206 - OpalVideoFastUpdatePicture over SIP
 * Thanks to Josh Mahonin
 *
 * Revision 2.214  2007/04/15 10:09:15  dsandras
 * Some systems like CISCO Call Manager do not like having a Contact field in INVITE
 * PDUs which is different to the one being used in the original REGISTER request.
 * Added code to use the same Contact field in both cases if we can determine that
 * we are registered to that specific account and if there is a transport running.
 * Fixed problem where the SIP connection was not released with a BYE PDU when
 * the ACK is received while we are already in EstablishedPhase.
 *
 * Revision 2.213  2007/04/04 02:12:02  rjongbloed
 * Reviewed and adjusted PTRACE log levels
 *   Now follows 1=error,2=warn,3=info,4+=debug
 *
 * Revision 2.212  2007/04/03 05:27:30  rjongbloed
 * Cleaned up somewhat confusing usage of the OnAnswerCall() virtual
 *   function. The name is innaccurate and exists as a legacy from the
 *   OpenH323 days. it now only indicates how alerting is done
 *   (with/without media) and does not actually answer the call.
 *
 * Revision 2.211  2007/03/30 14:45:32  hfriederich
 * Reorganization of hte way transactions are handled. Delete transactions
 *   in garbage collector when they're terminated. Update destructor code
 *   to improve safe destruction of SIPEndPoint instances.
 *
 * Revision 2.210  2007/03/30 02:09:53  rjongbloed
 * Fixed various GCC warnings
 *
 * Revision 2.209  2007/03/29 05:16:50  csoutheren
 * Pass OpalConnection to OpalMediaSream constructor
 * Add ID to OpalMediaStreams so that transcoders can match incoming and outgoing codecs
 *
 * Revision 2.208  2007/03/27 20:16:23  dsandras
 * Temporarily removed use of shared transports as it could have unexpected
 * side effects on the routing of PDUs.
 * Various fixes on the way SIPInfo objects are being handled. Wait
 * for transports to be closed before being deleted. Added missing mutexes.
 * Added garbage collector.
 *
 * Revision 2.206  2007/03/19 03:56:36  csoutheren
 * Fix problem with outgoing SDP on retry
 *
 * Revision 2.205  2007/03/19 01:10:45  rjongbloed
 * Fixed compiler warning
 *
 * Revision 2.204  2007/03/15 21:23:13  dsandras
 * Make sure lastTransportAddress is correctly initialized even when
 * uncompliant SIP PDUs are received.
 *
 * Revision 2.203  2007/03/13 02:17:49  csoutheren
 * Remove warnings/errors when compiling with various turned off
 *
 * Revision 2.202  2007/03/13 00:33:11  csoutheren
 * Simple but messy changes to allow compile time removal of protocol
 * options such as H.450 and H.460
 * Fix MakeConnection overrides
 *
 * Revision 2.201  2007/03/08 03:55:09  csoutheren
 * Applied 1674008 - Connection-specific jitter buffer values
 * Thanks to Borko Jandras
 *
 * Revision 2.200  2007/03/01 05:51:07  rjongbloed
 * Fixed backward compatibility of OnIncomingConnection() virtual
 *   functions on various classes. If an old override returned FALSE
 *   then it will now abort the call as it used to.
 *
 * Revision 2.199  2007/02/20 07:15:03  csoutheren
 * Second attempt at sane 180 handling
 *
 * Revision 2.198  2007/02/19 04:43:42  csoutheren
 * Added OnIncomingMediaChannels so incoming calls can optionally be handled in two stages
 *
 * Revision 2.197  2007/01/24 04:00:57  csoutheren
 * Arrrghh. Changing OnIncomingConnection turned out to have a lot of side-effects
 * Added some pure viritual functions to prevent old code from breaking silently
 * New OpalEndpoint and OpalConnection descendants will need to re-implement
 * OnIncomingConnection. Sorry :)
 *
 * Revision 2.196  2007/01/22 02:09:01  csoutheren
 * Fix mistake that crashes incoming calls
 *
 * Revision 2.195  2007/01/18 12:49:22  csoutheren
 * Add ability to disable T.38 in compile
 *
 * Revision 2.194  2007/01/18 04:45:17  csoutheren
 * Messy, but simple change to add additional options argument to OpalConnection constructor
 * This allows the provision of non-trivial arguments for connections
 *
 * Revision 2.193  2007/01/15 22:13:59  dsandras
 * Make sure we do not ignore SIP reINVITEs.
 *
 * Revision 2.192  2007/01/10 09:16:55  csoutheren
 * Allow compilation with video disabled
 *
 * Revision 2.191  2007/01/02 17:28:40  dsandras
 * Do not create a new RTPSessionManager when authenticating an INVITE, because
 * it will use new UDP ports which is not needed and causes bad behaviors with
 * some broken SIP Proxies. (Ekiga report #359971)
 *
 * Revision 2.190  2006/12/18 03:18:42  csoutheren
 * Messy but simple fixes
 *   - Add access to SIP REGISTER timeout
 *   - Ensure OpalConnection options are correctly progagated
 *
 * Revision 2.189  2006/12/08 04:24:19  csoutheren
 * Applied 1604554 - SIP remote hold improvements
 * Thanks to Simon Zwahlen
 *
 * Revision 2.188  2006/11/28 20:24:24  dsandras
 * Fixed crash when calling himself due to transport deletion.
 *
 * Revision 2.187  2006/11/11 12:23:18  hfriederich
 * Code reorganisation to improve RFC2833 handling for both SIP and H.323. Thanks Simon Zwahlen for the idea
 *
 * Revision 2.186  2006/11/10 23:19:51  hfriederich
 * Don't send RFC2833 if already sent INFO
 *
 * Revision 2.185  2006/11/09 18:24:55  hfriederich
 * Ensures that responses to received INVITE get sent out on the same network interface as where the INVITE was received. Remove cout statement
 *
 * Revision 2.184  2006/11/09 18:02:07  hfriederich
 * Prevent sending OK if already releasing, thus having sent a final response. Modify call end reason if media format list empty
 *
 * Revision 2.183  2006/10/28 16:40:28  dsandras
 * Fixed SIP reinvite without breaking H.323 calls.
 *
 * Revision 2.182  2006/10/11 01:22:51  csoutheren
 * Applied 1565585 - SIP changed codec order (compatibility to Mediatrix)
 * Thanks to Simon Zwahlen
 *
 * Revision 2.181  2006/10/11 00:40:07  csoutheren
 * Fix problem introduced in v1.162 that prevent external RTP from working properly
 *
 * Revision 2.180  2006/10/06 05:33:12  hfriederich
 * Fix RFC2833 for SIP connections
 *
 * Revision 2.179  2006/10/01 17:16:32  hfriederich
 * Ensures that an ACK is sent out for every final response to INVITE
 *
 * Revision 2.178  2006/08/28 00:53:52  csoutheren
 * Applied 1546613 - Transport check in SIPConnection::SetConnected
 * Thanks to Drazen Dimoti
 *
 * Revision 2.177  2006/08/28 00:35:57  csoutheren
 * Applied 1545205 - Usage of SetPhase in SIPConnection
 * Thanks to Drazen Dimoti
 *
 * Revision 2.176  2006/08/28 00:32:16  csoutheren
 * Applied 1545191 - SIPConnection invitationsMutex fix
 * Thanks to Drazen Dimoti
 *
 * Revision 2.175  2006/08/28 00:06:10  csoutheren
 * Applied 1545107 - MediaStream - safe access to patch for adding a filter
 * Thanks to Drazen Dimoti
 *
 * Revision 2.174  2006/08/20 13:35:56  csoutheren
 * Backported changes to protect against rare error conditions
 *
 * Revision 2.173  2006/08/12 04:09:24  csoutheren
 * Applied 1538497 - Add the PING method
 * Thanks to Paul Rolland
 *
 * Revision 2.172  2006/08/12 03:23:33  csoutheren
 * Fixed sending SIP INFO application/dtmf
 *
 * Revision 2.171  2006/08/10 04:31:59  csoutheren
 * Applied 1534444 - Lock to streamsMutex in SIPConnection::InitRFC2833Handler
 * Thanks to Drazen Dimoti
 *
 * Revision 2.170  2006/08/10 04:21:40  csoutheren
 * Applied 1532444 - Small fix for SIPConnection::OnReceivedPDU
 * Thanks to Borko Jandras
 *
 * Revision 2.169  2006/08/07 19:46:24  hfriederich
 * Ensuring that no INVITE is sent if connection is already in releasing phase
 *
 * Revision 2.168  2006/07/30 11:36:09  hfriederich
 * Fixes problems with empty SDPMediaDescriptions
 *
 * Revision 2.167  2006/07/24 14:03:40  csoutheren
 * Merged in audio and video plugins from CVS branch PluginBranch
 *
 * Revision 2.166  2006/07/21 00:42:08  csoutheren
 * Applied 1525040 - More locking when changing connection's phase
 * Thanks to Borko Jandras
 *
 * Revision 2.165  2006/07/14 13:45:01  csoutheren
 * Applied 1522528 - fixes a crash on SIP call release
 * Thanks to Drazen Dimoti
 *
 * Revision 2.164  2006/07/14 04:22:43  csoutheren
 * Applied 1517397 - More Phobos stability fix
 * Thanks to Dinis Rosario
 *
 * Revision 2.163  2006/07/09 12:47:23  dsandras
 * Added clear cause for EndedByNoAnswer.
 *
 * Revision 2.162  2006/07/09 10:18:28  csoutheren
 * Applied 1517393 - Opal T.38
 * Thanks to Drazen Dimoti
 *
 * Revision 2.161  2006/07/09 10:03:29  csoutheren
 * Applied 1518681 - Refactoring boilerplate code
 * Thanks to Borko Jandras
 *
 * Revision 2.160  2006/07/05 04:29:14  csoutheren
 * Applied 1495008 - Add a callback: OnCreatingINVITE
 * Thanks to mturconi
 *
 * Revision 2.159  2006/06/30 14:55:53  dsandras
 * Reverted broken patch. Reinvites can have different branch ID's.
 *
 * Revision 2.158  2006/06/30 01:42:39  csoutheren
 * Applied 1509216 - SIPConnection::OnReceivedOK fix
 * Thanks to Boris Pavacic
 *
 * Revision 2.157  2006/06/28 11:29:08  csoutheren
 * Patch 1456858 - Add mutex to transaction dictionary and other stability patches
 * Thanks to drosario
 *
 * Revision 2.156  2006/06/27 13:50:25  csoutheren
 * Patch 1375137 - Voicetronix patches and lid enhancements
 * Thanks to Frederich Heem
 *
 * Revision 2.155  2006/06/22 00:26:04  csoutheren
 * Remove warning on gcc
 *
 * Revision 2.154  2006/06/20 05:23:58  csoutheren
 * Ignore INVITEs that arrive via different route for same call
 * Ensure CANCELs are sent for all INVITEs in all cases
 *
 * Revision 2.153  2006/06/15 17:41:34  dsandras
 * Definitely fixed recently introduced bug when forwarding calls.
 *
 * Revision 2.152  2006/06/15 00:29:58  csoutheren
 * Fixed warning on Linux
 *
 * Revision 2.151  2006/06/15 00:18:58  csoutheren
 * Fixed problem with call forwarding from last commit, thanks to Damien Sandras
 *
 * Revision 2.150  2006/06/09 04:22:24  csoutheren
 * Implemented mapping between SIP release codes and Q.931 codes as specified
 *  by RFC 3398
 *
 * Revision 2.149  2006/05/30 04:58:06  csoutheren
 * Added suport for SIP INFO message (untested as yet)
 * Fixed some issues with SIP state machine on answering calls
 * Fixed some formatting issues
 *
 * Revision 2.148  2006/05/24 00:56:14  csoutheren
 * Fixed problem with deferred answer on SIP calls
 *
 * Revision 2.147  2006/04/24 21:19:31  dsandras
 * Fixed previous commit.
 *
 * Revision 2.146  2006/04/23 20:12:52  dsandras
 * The RFC tells that the SDP answer SHOULD have the same payload type than the
 * SDP offer. Added rtpmap support to allow this. Fixes problems with Asterisk,
 * and Ekiga report #337456.
 *
 * Revision 2.145  2006/04/22 19:49:33  dsandras
 * Immediately initialize the transport with the proxy address (if any) when
 * doing the connection setup. Fixed Ekiga report #334455.
 *
 * Revision 2.144  2006/04/22 10:48:14  dsandras
 * Added support for SDP offers in the OK response, and SDP answer in the ACK
 * request.
 *
 * Revision 2.143  2006/04/10 05:18:41  csoutheren
 * Fix problem where reverse channel is opened using wrong media format list
 *
 * Revision 2.142  2006/04/06 20:39:41  dsandras
 * Keep the same From header when sending authenticated requests than in the
 * original request. Fixes Ekiga report #336762.
 *
 * Revision 2.141  2006/03/23 00:24:49  csoutheren
 * Detect if ClearCall is used within OnIncomingConnection
 *
 * Revision 2.140  2006/03/19 17:18:46  dsandras
 * Fixed SRV handling.
 *
 * Revision 2.139  2006/03/19 16:59:00  dsandras
 * Removed cout again.
 *
 * Revision 2.138  2006/03/19 16:58:19  dsandras
 * Do not release a call we are originating when receiving CANCEL.
 *
 * Revision 2.137  2006/03/19 16:05:00  dsandras
 * Improved CANCEL handling. Ignore missing transport type.
 *
 * Revision 2.136  2006/03/18 21:45:28  dsandras
 * Do an SRV lookup when creating the connection. Some domains to which
 * INVITEs are sent do not have an A record, which makes the transport creation
 * and the connection fail. Fixes Ekiga report #334994.
 *
 * Revision 2.135  2006/03/08 18:34:41  dsandras
 * Added DNS SRV lookup.
 *
 * Revision 2.134  2006/03/06 22:52:59  csoutheren
 * Reverted experimental SRV patch due to unintended side effects
 *
 * Revision 2.133  2006/03/06 12:56:02  csoutheren
 * Added experimental support for SIP SRV lookups
 *
 * Revision 2.132.2.7  2006/04/25 01:06:22  csoutheren
 * Allow SIP-only codecs
 *
 * Revision 2.132.2.6  2006/04/10 06:24:30  csoutheren
 * Backport from CVS head up to Plugin_Merge3
 *
 * Revision 2.132.2.5  2006/04/10 05:24:42  csoutheren
 * Backport from CVS head
 *
 * Revision 2.132.2.4  2006/04/06 05:34:16  csoutheren
 * Backports from CVS head up to Plugin_Merge2
 *
 * Revision 2.132.2.3  2006/04/06 05:33:09  csoutheren
 * Backports from CVS head up to Plugin_Merge2
 *
 * Revision 2.132.2.2  2006/04/06 01:21:21  csoutheren
 * More implementation of video codec plugins
 *
 * Revision 2.132.2.1  2006/03/20 02:25:27  csoutheren
 * Backports from CVS head
 *
 * Revision 2.143  2006/04/10 05:18:41  csoutheren
 * Fix problem where reverse channel is opened using wrong media format list
 *
 * Revision 2.142  2006/04/06 20:39:41  dsandras
 * Keep the same From header when sending authenticated requests than in the
 * original request. Fixes Ekiga report #336762.
 *
 * Revision 2.141  2006/03/23 00:24:49  csoutheren
 * Detect if ClearCall is used within OnIncomingConnection
 *
 * Revision 2.140  2006/03/19 17:18:46  dsandras
 * Fixed SRV handling.
 *
 * Revision 2.139  2006/03/19 16:59:00  dsandras
 * Removed cout again.
 *
 * Revision 2.138  2006/03/19 16:58:19  dsandras
 * Do not release a call we are originating when receiving CANCEL.
 *
 * Revision 2.137  2006/03/19 16:05:00  dsandras
 * Improved CANCEL handling. Ignore missing transport type.
 *
 * Revision 2.136  2006/03/18 21:45:28  dsandras
 * Do an SRV lookup when creating the connection. Some domains to which
 * INVITEs are sent do not have an A record, which makes the transport creation
 * and the connection fail. Fixes Ekiga report #334994.
 *
 * Revision 2.135  2006/03/08 18:34:41  dsandras
 * Added DNS SRV lookup.
 *
 * Revision 2.134  2006/03/06 22:52:59  csoutheren
 * Reverted experimental SRV patch due to unintended side effects
 *
 * Revision 2.133  2006/03/06 12:56:02  csoutheren
 * Added experimental support for SIP SRV lookups
 *
 * Revision 2.132  2006/02/11 21:05:27  dsandras
 * Release the call when something goes wrong.
 *
 * Revision 2.131  2006/02/11 15:47:41  dsandras
 * When receiving an invite, try using the remote prefered codec. The targetAddress
 * should be initialized to the contact field value of the incoming invite
 * when receiving a call.
 *
 * Revision 2.130  2006/02/10 23:44:04  csoutheren
 * Applied fix for SetConnection and RFC2833 startup
 *
 * Revision 2.129  2006/02/06 22:40:11  dsandras
 * Added additional check for rtpSession thanks to Guillaume Fraysse.
 *
 * Revision 2.128  2006/02/04 16:22:38  dsandras
 * Fixed recently introduced bug reported by Guillaume Fraysse. Thanks!
 *
 * Revision 2.127  2006/02/04 16:06:24  dsandras
 * Fixed problems with media formats being used when calling and when the remote
 * has a different prefered order than ours.
 *
 * Revision 2.126  2006/02/02 07:02:58  csoutheren
 * Added RTP payload map to transcoders and connections to allow remote SIP endpoints
 * to change the payload type used for outgoing RTP.
 *
 * Revision 2.125  2006/01/29 20:55:32  dsandras
 * Allow using a simple username or a fill url when registering.
 *
 * Revision 2.124  2006/01/23 22:54:57  csoutheren
 * Ensure codec remove mask is applied on outgoing SIP calls
 *
 * Revision 2.123  2006/01/23 22:11:06  dsandras
 * Only rebuild the routeSet if we have an outbound proxy and the routeSet is
 * empty.
 *
 * Revision 2.122  2006/01/22 22:18:36  dsandras
 * Added a failure case.
 *
 * Revision 2.121  2006/01/21 13:55:02  dsandras
 * Fixed default route set when an outbound proxy is being used thanks to Vincent
 * Untz <vuntz gnome org>. Thanks!
 *
 * Revision 2.120  2006/01/16 23:05:09  dsandras
 * Minor fixes. Reset the route set to the proxy (if any), when authenticating
 * invite.
 *
 * Revision 2.119  2006/01/12 20:23:44  dsandras
 * Reorganized things to prevent crashes when calling itself.
 *
 * Revision 2.118  2006/01/09 12:19:07  csoutheren
 * Added member variables to capture incoming destination addresses
 *
 * Revision 2.117  2006/01/09 11:49:46  dsandras
 * Call SetRemoteSocketInfo to update the remote address when doing calls.
 *
 * Revision 2.116  2006/01/02 14:47:28  dsandras
 * More code cleanups.
 *
 * Revision 2.115  2006/01/02 11:28:07  dsandras
 * Some documentation. Various code cleanups to prevent duplicate code.
 *
 * Revision 2.114  2005/12/26 20:53:37  dsandras
 * Small fix.
 *
 * Revision 2.113  2005/12/14 17:59:50  dsandras
 * Added ForwardConnection executed when the remote asks for a call forwarding.
 * Similar to what is done in the H.323 part with the method of the same name.
 *
 * Revision 2.112  2005/12/11 12:23:01  dsandras
 * Prevent loop of authenticating when it fails by storing the last parameters
 * before they are updated.
 *
 * Revision 2.111  2005/12/06 21:32:25  dsandras
 * Applied patch from Frederic Heem <frederic.heem _Atttt_ telsey.it> to fix
 * assert in PSyncPoint when OnReleased is called twice from different threads.
 * Thanks! (Patch #1374240)
 *
 * Revision 2.110  2005/12/04 15:02:00  dsandras
 * Fixed IP translation in the VIA field of most request PDUs.
 *
 * Revision 2.109  2005/11/25 21:02:52  dsandras
 * Release the call if the SDP can not be built.
 *
 * Revision 2.108  2005/11/20 20:55:55  dsandras
 * End the connection when the session can not be created.
 *
 * Revision 2.107  2005/10/27 20:53:36  dsandras
 * Send a BYE when receiving a NOTIFY for a REFER we just sent.
 *
 * Revision 2.106  2005/10/22 19:57:33  dsandras
 * Added SIP failure cause.
 *
 * Revision 2.105  2005/10/22 14:43:48  dsandras
 * Little cleanup.
 *
 * Revision 2.104  2005/10/22 12:16:05  dsandras
 * Moved mutex preventing media streams to be opened before they are completely closed to the SIPConnection class.
 *
 * Revision 2.103  2005/10/20 20:25:28  dsandras
 * Update the RTP Session address when the new streams are started.
 *
 * Revision 2.102  2005/10/18 20:50:13  dsandras
 * Fixed tone playing in session progress.
 *
 * Revision 2.101  2005/10/18 17:50:35  dsandras
 * Fixed REFER so that it doesn't send a BYE but sends the NOTIFY. Fixed leak on exit if a REFER transaction was in progress while the connection is destroyed.
 *
 * Revision 2.100  2005/10/13 19:33:50  dsandras
 * Added GetDirection to get the default direction for a media stream. Modified OnSendMediaDescription to call BuildSDP if no reverse streams can be opened.
 *
 * Revision 2.99  2005/10/13 18:14:45  dsandras
 * Another try to get it right.
 *
 * Revision 2.98  2005/10/12 21:39:04  dsandras
 * Small protection for the SDP.
 *
 * Revision 2.97  2005/10/12 21:32:08  dsandras
 * Cleanup.
 *
 * Revision 2.96  2005/10/12 21:10:39  dsandras
 * Removed CanAutoStartTransmitVideo and CanAutoStartReceiveVideo from the SIPConnection class.
 *
 * Revision 2.95  2005/10/12 17:55:18  dsandras
 * Fixed previous commit.
 *
 * Revision 2.94  2005/10/12 08:53:42  dsandras
 * Committed cleanup patch.
 *
 * Revision 2.93  2005/10/11 21:51:44  dsandras
 * Reverted a previous patch.
 *
 * Revision 2.92  2005/10/11 21:47:04  dsandras
 * Fixed problem when sending the 200 OK response to an INVITE for which some media stream is 'sendonly'.
 *
 * Revision 2.91  2005/10/08 19:27:26  dsandras
 * Added support for OnForwarded.
 *
 * Revision 2.90  2005/10/05 21:27:25  dsandras
 * Having a source/sink stream is not opened because of sendonly/recvonly is not
 * a stream opening failure. Fixed problems with streams direction.
 *
 * Revision 2.89  2005/10/04 16:32:25  dsandras
 * Added back support for CanAutoStartReceiveVideo.
 *
 * Revision 2.88  2005/10/04 12:57:18  rjongbloed
 * Removed CanOpenSourceMediaStream/CanOpenSinkMediaStream functions and
 *   now use overides on OpenSourceMediaStream/OpenSinkMediaStream
 *
 * Revision 2.87  2005/10/02 17:49:08  dsandras
 * Cleaned code to use the new GetContactAddress.
 *
 * Revision 2.86  2005/09/27 16:17:12  dsandras
 * - Added SendPDU method and use it everywhere for requests sent in the dialog
 * and to transmit responses to incoming requests.
 * - Fixed again re-INVITE support and 200 OK generation.
 * - Update the route set when receiving responses and requests according to the
 * RFC.
 * - Update the targetAddress to the Contact field when receiving a response.
 * - Transmit the ack for 2xx and non-2xx responses.
 * - Do not update the remote transport address when receiving requests and
 * responses. Do it only in SendPDU as the remote transport address for a response
 * and for a request in a dialog are different.
 * - Populate the route set with an initial route when an outbound proxy should
 * be used as recommended by the RFC.
 *
 * Revision 2.85  2005/09/21 21:03:04  dsandras
 * Fixed reINVITE support.
 *
 * Revision 2.84  2005/09/21 19:50:30  dsandras
 * Cleaned code. Make use of the new SIP_PDU function that returns the correct address where to send responses to incoming requests.
 *
 * Revision 2.83  2005/09/20 17:13:57  dsandras
 * Fixed warning.
 *
 * Revision 2.82  2005/09/20 17:04:35  dsandras
 * Removed redundant call to SetBufferSize.
 *
 * Revision 2.81  2005/09/20 07:57:29  csoutheren
 * Fixed bug in previous commit
 *
 * Revision 2.80  2005/09/20 07:24:05  csoutheren
 * Removed assumption of UDP transport for SIP
 *
 * Revision 2.79  2005/09/17 20:54:16  dsandras
 * Check for existing transport before using it.
 *
 * Revision 2.78  2005/09/15 17:07:47  dsandras
 * Fixed video support. Make use of BuildSDP when possible. Do not open the sink/source media streams if the remote or the local user have disabled audio/video transmission/reception by checking the direction media and session attributes.
 *
 * Revision 2.77  2005/08/25 18:49:52  dsandras
 * Added SIP Video support. Changed API of BuildSDP to allow it to be called
 * for both audio and video.
 *
 * Revision 2.76  2005/08/09 09:16:24  rjongbloed
 * Fixed compiler warning
 *
 * Revision 2.75  2005/08/04 17:15:52  dsandras
 * Fixed local port for sending requests on incoming calls.
 * Allow for codec changes on re-INVITE.
 * More blind transfer implementation.
 *
 * Revision 2.74  2005/07/15 17:22:43  dsandras
 * Use correct To tag when sending a new INVITE when authentication is required and an outbound proxy is being used.
 *
 * Revision 2.73  2005/07/14 08:51:19  csoutheren
 * Removed CreateExternalRTPAddress - it's not needed because you can override GetMediaAddress
 * to do the same thing
 * Fixed problems with logic associated with media bypass
 *
 * Revision 2.72  2005/07/11 01:52:26  csoutheren
 * Extended AnsweringCall to work for SIP as well as H.323
 * Fixed problems with external RTP connection in H.323
 * Added call to OnClosedMediaStream
 *
 * Revision 2.71  2005/06/04 12:44:36  dsandras
 * Applied patch from Ted Szoczei to fix leaks and problems on cancelling a call and to improve the Allow PDU field handling.
 *
 * Revision 2.70  2005/05/25 18:34:25  dsandras
 * Added missing AdjustMediaFormats so that only enabled common codecs are used on outgoing calls.
 *
 * Revision 2.69  2005/05/23 20:55:15  dsandras
 * Use STUN on incoming calls if required so that all the PDU fields are setup appropriately.
 *
 * Revision 2.68  2005/05/18 17:26:39  dsandras
 * Added back proxy support for INVITE.
 *
 * Revision 2.67  2005/05/16 14:40:32  dsandras
 * Make the connection fail when there is no authentication information present
 * and authentication is required.
 *
 * Revision 2.66  2005/05/12 19:47:29  dsandras
 * Fixed indentation.
 *
 * Revision 2.65  2005/05/06 07:37:06  csoutheren
 * Various changed while working with SIP carrier
 *   - remove assumption that authentication realm is a domain name.
 *   - stopped rewrite of "To" field when proxy being used
 *   - fix Contact field in REGISTER to match actual port used when Symmetric NATin use
 *   - lots of formatting changes and cleanups
 *
 * Revision 2.64  2005/05/04 17:09:40  dsandras
 * Re-Invite happens during the "EstablishedPhase". Ignore duplicate INVITEs
 * due to retransmission.
 *
 * Revision 2.63  2005/05/02 21:31:54  dsandras
 * Reinvite only if connectedPhase.
 *
 * Revision 2.62  2005/05/02 21:23:22  dsandras
 * Set default contact port to the first listener port.
 *
 * Revision 2.61  2005/05/02 20:43:03  dsandras
 * Remove the via parameters when updating to the via address in the transport.
 *
 * Revision 2.60  2005/04/28 20:22:54  dsandras
 * Applied big sanity patch for SIP thanks to Ted Szoczei <tszoczei@microtronix.ca>.
 * Thanks a lot!
 *
 * Revision 2.59  2005/04/28 07:59:37  dsandras
 * Applied patch from Ted Szoczei to fix problem when answering to PDUs containing
 * multiple Via fields in the message header. Thanks!
 *
 * Revision 2.58  2005/04/18 17:07:17  dsandras
 * Fixed cut and paste error in last commit thanks to Ted Szoczei.
 *
 * Revision 2.57  2005/04/16 18:57:36  dsandras
 * Use a TO header without tag when restarting an INVITE.
 *
 * Revision 2.56  2005/04/11 11:12:38  dsandras
 * Added Method_MESSAGE support for future use.
 *
 * Revision 2.55  2005/04/11 10:23:58  dsandras
 * 1) Added support for SIP ReINVITE without codec changes.
 * 2) Added support for GetRemotePartyCallbackURL.
 * 3) Added support for call hold (local and remote).
 * 4) Fixed missing tag problem when sending BYE requests.
 * 5) Added support for Blind Transfer (without support for being transfered).
 *
 * Revision 2.54  2005/03/11 18:12:09  dsandras
 * Added support to specify the realm when registering. That way softphones already know what authentication information to use when required. The realm/domain can also be used in the From field.
 *
 * Revision 2.53  2005/02/19 22:48:48  dsandras
 * Added the possibility to register to several registrars and be able to do authenticated calls to each of them. Added SUBSCRIBE/NOTIFY support for Message Waiting Indications.
 *
 * Revision 2.52  2005/02/19 22:26:09  dsandras
 * Ignore TO tag added by OPAL. Reported by Nick Noath.
 *
 * Revision 2.51  2005/01/16 11:28:05  csoutheren
 * Added GetIdentifier virtual function to OpalConnection, and changed H323
 * and SIP descendants to use this function. This allows an application to
 * obtain a GUID for any connection regardless of the protocol used
 *
 * Revision 2.50  2005/01/15 19:42:33  csoutheren
 * Added temporary workaround for deadlock that occurs with incoming calls
 *
 * Revision 2.49  2005/01/09 03:42:46  rjongbloed
 * Fixed warning about unused parameter
 *
 * Revision 2.48  2004/12/25 20:43:42  dsandras
 * Attach the RFC2833 handlers when we are in connected state to ensure
 * OpalMediaPatch exist. Fixes problem for DTMF sending.
 *
 * Revision 2.47  2004/12/23 20:43:27  dsandras
 * Only start the media streams if we are in the phase connectedPhase.
 *
 * Revision 2.46  2004/12/22 18:55:08  dsandras
 * Added support for Call Forwarding the "302 Moved Temporarily" SIP response.
 *
 * Revision 2.45  2004/12/18 03:54:43  rjongbloed
 * Added missing call of callback virtual for SIP 100 Trying response
 *
 * Revision 2.44  2004/12/12 13:40:45  dsandras
 * - Correctly update the remote party name, address and applications at various strategic places.
 * - If no outbound proxy authentication is provided, then use the registrar authentication parameters when proxy authentication is required.
 * - Fixed use of connectedTime.
 * - Correctly set the displayName in the FROM field for outgoing connections.
 * - Added 2 "EndedBy" cases when a connection ends.
 *
 * Revision 2.43  2004/11/29 08:18:32  csoutheren
 * Added support for setting the SIP authentication domain/realm as needed for many service
 *  providers
 *
 * Revision 2.42  2004/08/20 12:13:32  rjongbloed
 * Added correct handling of SIP 180 response
 *
 * Revision 2.41  2004/08/14 07:56:43  rjongbloed
 * Major revision to utilise the PSafeCollection classes for the connections and calls.
 *
 * Revision 2.40  2004/04/26 05:40:39  rjongbloed
 * Added RTP statistics callback to SIP
 *
 * Revision 2.39  2004/04/25 08:34:08  rjongbloed
 * Fixed various GCC 3.4 warnings
 *
 * Revision 2.38  2004/03/16 12:03:33  rjongbloed
 * Fixed poropagating port of proxy to connection target address.
 *
 * Revision 2.37  2004/03/15 12:33:48  rjongbloed
 * Fixed proxy override in URL
 *
 * Revision 2.36  2004/03/14 11:32:20  rjongbloed
 * Changes to better support SIP proxies.
 *
 * Revision 2.35  2004/03/14 10:09:54  rjongbloed
 * Moved transport on SIP top be constructed by endpoint as any transport created on
 *   an endpoint can receive data for any connection.
 *
 * Revision 2.34  2004/03/13 06:29:27  rjongbloed
 * Slight rearrangement of local party name and alias list to beter match common
 *   behaviour in ancestor.
 * Abstracted local party name for endpoint into ancestor from H.,323.
 * Changed parameter in UDP write function to void * from PObject *.
 *
 * Revision 2.33  2004/02/24 11:33:47  rjongbloed
 * Normalised RTP session management across protocols
 * Added support for NAT (via STUN)
 *
 * Revision 2.32  2004/02/15 03:12:10  rjongbloed
 * More patches from Ted Szoczei, fixed shutting down of RTP session if INVITE fails. Added correct return if no meda formats can be matched.
 *
 * Revision 2.31  2004/02/14 22:52:51  csoutheren
 * Re-ordered initialisations to remove warning on Linux
 *
 * Revision 2.30  2004/02/07 02:23:21  rjongbloed
 * Changed to allow opening of more than just audio streams.
 * Assured that symmetric codecs are used as required by "basic" SIP.
 * Made media format list subject to adjust \\ment by remoe/order lists.
 *
 * Revision 2.29  2004/01/08 22:23:23  csoutheren
 * Fixed problem with not using session ID when constructing SDP lists
 *
 * Revision 2.28  2003/12/20 12:21:18  rjongbloed
 * Applied more enhancements, thank you very much Ted Szoczei
 *
 * Revision 2.27  2003/12/15 11:56:17  rjongbloed
 * Applied numerous bug fixes, thank you very much Ted Szoczei
 *
 * Revision 2.26  2003/03/17 10:27:00  robertj
 * Added video support.
 *
 * Revision 2.25  2003/03/07 05:52:35  robertj
 * Made sure connection is locked with all function calls that are across
 *   the "call" object.
 *
 * Revision 2.24  2003/03/06 03:57:47  robertj
 * IVR support (work in progress) requiring large changes everywhere.
 *
 * Revision 2.23  2002/11/10 11:33:20  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.22  2002/09/12 06:58:34  robertj
 * Removed protocol prefix strings as static members as has problems with
 *   use in DLL environment.
 *
 * Revision 2.21  2002/07/01 04:56:33  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 2.20  2002/06/16 02:26:37  robertj
 * Fixed creation of RTP session for incoming calls, thanks Ted Szoczei
 *
 * Revision 2.19  2002/04/17 07:22:54  robertj
 * Added identifier for conenction in OnReleased trace message.
 *
 * Revision 2.18  2002/04/16 09:04:18  robertj
 * Fixed setting of target URI from Contact regardless of route set
 *
 * Revision 2.17  2002/04/16 07:53:15  robertj
 * Changes to support calls through proxies.
 *
 * Revision 2.16  2002/04/15 08:53:58  robertj
 * Fixed correct setting of jitter buffer size in RTP media stream.
 * Fixed starting incoming (not outgoing!) media on call progress.
 *
 * Revision 2.15  2002/04/10 06:58:31  robertj
 * Fixed incorrect return value when starting INVITE transactions.
 *
 * Revision 2.14  2002/04/10 03:14:35  robertj
 * Moved code for handling media bypass address resolution into ancestor as
 *   now done ths same way in both SIP and H.323.
 * Major changes to RTP session management when initiating an INVITE.
 * Improvements in error handling and transaction cancelling.
 *
 * Revision 2.13  2002/04/09 07:26:20  robertj
 * Added correct error return if get transport failure to remote endpoint.
 *
 * Revision 2.12  2002/04/09 01:19:41  robertj
 * Fixed typo on changing "callAnswered" to better description of "originating".
 *
 * Revision 2.11  2002/04/09 01:02:14  robertj
 * Fixed problems with restarting INVITE on  authentication required response.
 *
 * Revision 2.10  2002/04/08 02:40:13  robertj
 * Fixed issues with using double originate call, eg from simple app command line.
 *
 * Revision 2.9  2002/04/05 10:42:04  robertj
 * Major changes to support transactions (UDP timeouts and retries).
 *
 * Revision 2.8  2002/03/18 08:13:42  robertj
 * Added more logging.
 * Removed proxyusername/proxypassword parameters from URL passed on.
 * Fixed GNU warning.
 *
 * Revision 2.7  2002/03/15 10:55:28  robertj
 * Added ability to specify proxy username/password in URL.
 *
 * Revision 2.6  2002/03/08 06:28:03  craigs
 * Changed to allow Authorisation to be included in other PDUs
 *
 * Revision 2.5  2002/02/19 07:53:17  robertj
 * Restructured media bypass functions to fix problems with RFC2833.
 *
 * Revision 2.4  2002/02/13 04:55:44  craigs
 * Fixed problem with endless loop if proxy keeps failing authentication with 407
 *
 * Revision 2.3  2002/02/11 07:33:36  robertj
 * Changed SDP to use OpalTransport for hosts instead of IP addresses/ports
 * Added media bypass for streams between compatible protocols.
 *
 * Revision 2.2  2002/02/01 05:47:55  craigs
 * Removed :: from in front of isspace that confused gcc 2.96
 *
 * Revision 2.1  2002/02/01 04:53:01  robertj
 * Added (very primitive!) SIP support.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "sipcon.h"
#endif

#include <sip/sipcon.h>

#include <sip/sipep.h>
#include <codec/rfc2833.h>
#include <opal/manager.h>
#include <opal/call.h>
#include <opal/patch.h>
#include <ptclib/random.h>              // for local dialog tag
#include <ptclib/pdns.h>

#if OPAL_T38FAX
#include <t38/t38proto.h>
#endif

#define new PNEW

typedef void (SIPConnection::* SIPMethodFunction)(SIP_PDU & pdu);

static const char ApplicationDTMFRelayKey[]       = "application/dtmf-relay";
static const char ApplicationDTMFKey[]            = "application/dtmf";

#if OPAL_VIDEO
static const char ApplicationMediaControlXMLKey[] = "application/media_control+xml";
#endif


static const struct {
  SIP_PDU::StatusCodes          code;
  OpalConnection::CallEndReason reason;
  unsigned                      q931Cause;
}
//
// This table comes from RFC 3398 para 7.2.4.1
//
ReasonToSIPCode[] = {
  { SIP_PDU::Failure_NotFound                   , OpalConnection::EndedByNoUser            ,   1 }, // Unallocated number
  { SIP_PDU::Failure_NotFound                   , OpalConnection::EndedByNoUser            ,   2 }, // no route to network
  { SIP_PDU::Failure_NotFound                   , OpalConnection::EndedByNoUser            ,   3 }, // no route to destination
  { SIP_PDU::Failure_BusyHere                   , OpalConnection::EndedByLocalBusy         ,  17 }, // user busy                            
  { SIP_PDU::Failure_RequestTimeout             , OpalConnection::EndedByNoUser            ,  18 }, // no user responding                   
  { SIP_PDU::Failure_TemporarilyUnavailable     , OpalConnection::EndedByNoAnswer          ,  19 }, // no answer from the user              
  { SIP_PDU::Failure_TemporarilyUnavailable     , OpalConnection::EndedByNoUser            ,  20 }, // subscriber absent                    
  { SIP_PDU::Failure_Forbidden                  , OpalConnection::EndedByNoUser            ,  21 }, // call rejected                        
  { SIP_PDU::Failure_Gone                       , OpalConnection::EndedByNoUser            ,  22 }, // number changed (w/o diagnostic)      
  { SIP_PDU::Redirection_MovedPermanently       , OpalConnection::EndedByNoUser            ,  22 }, // number changed (w/ diagnostic)       
  { SIP_PDU::Failure_Gone                       , OpalConnection::EndedByNoUser            ,  23 }, // redirection to new destination       
  { SIP_PDU::Failure_NotFound                   , OpalConnection::EndedByNoUser            ,  26 }, // non-selected user clearing           
  { SIP_PDU::Failure_BadGateway                 , OpalConnection::EndedByNoUser            ,  27 }, // destination out of order             
  { SIP_PDU::Failure_AddressIncomplete          , OpalConnection::EndedByNoUser            ,  28 }, // address incomplete                   
  { SIP_PDU::Failure_NotImplemented             , OpalConnection::EndedByNoUser            ,  29 }, // facility rejected                    
  { SIP_PDU::Failure_TemporarilyUnavailable     , OpalConnection::EndedByNoUser            ,  31 }, // normal unspecified                   
  { SIP_PDU::Failure_ServiceUnavailable         , OpalConnection::EndedByNoUser            ,  34 }, // no circuit available                 
  { SIP_PDU::Failure_ServiceUnavailable         , OpalConnection::EndedByNoUser            ,  38 }, // network out of order                 
  { SIP_PDU::Failure_ServiceUnavailable         , OpalConnection::EndedByNoUser            ,  41 }, // temporary failure                    
  { SIP_PDU::Failure_ServiceUnavailable         , OpalConnection::EndedByNoUser            ,  42 }, // switching equipment congestion       
  { SIP_PDU::Failure_ServiceUnavailable         , OpalConnection::EndedByNoUser            ,  47 }, // resource unavailable                 
  { SIP_PDU::Failure_Forbidden                  , OpalConnection::EndedByNoUser            ,  55 }, // incoming calls barred within CUG     
  { SIP_PDU::Failure_Forbidden                  , OpalConnection::EndedByNoUser            ,  57 }, // bearer capability not authorized     
  { SIP_PDU::Failure_ServiceUnavailable         , OpalConnection::EndedByNoUser            ,  58 }, // bearer capability not presently available
  { SIP_PDU::Failure_NotAcceptableHere          , OpalConnection::EndedByNoUser            ,  65 }, // bearer capability not implemented
  { SIP_PDU::Failure_NotAcceptableHere          , OpalConnection::EndedByNoUser            ,  70 }, // only restricted digital avail    
  { SIP_PDU::Failure_NotImplemented             , OpalConnection::EndedByNoUser            ,  79 }, // service or option not implemented
  { SIP_PDU::Failure_Forbidden                  , OpalConnection::EndedByNoUser            ,  87 }, // user not member of CUG           
  { SIP_PDU::Failure_ServiceUnavailable         , OpalConnection::EndedByNoUser            ,  88 }, // incompatible destination         
  { SIP_PDU::Failure_ServerTimeout              , OpalConnection::EndedByNoUser            , 102 }, // recovery of timer expiry         
  { SIP_PDU::Failure_InternalServerError        , OpalConnection::EndedByNoUser            , 111 }, // protocol error                   
  { SIP_PDU::Failure_InternalServerError        , OpalConnection::EndedByNoUser            , 127 }, // interworking unspecified         
  { SIP_PDU::Failure_RequestTerminated          , OpalConnection::EndedByCallerAbort             },
  { SIP_PDU::Failure_UnsupportedMediaType       , OpalConnection::EndedByCapabilityExchange      },
  { SIP_PDU::Redirection_MovedTemporarily       , OpalConnection::EndedByCallForwarded           },
  { SIP_PDU::GlobalFailure_Decline              , OpalConnection::EndedByAnswerDenied            },
},

//
// This table comes from RFC 3398 para 8.2.6.1
//
SIPCodeToReason[] = {
  { SIP_PDU::Failure_BadRequest                 , OpalConnection::EndedByQ931Cause         ,  41 }, // Temporary Failure
  { SIP_PDU::Failure_UnAuthorised               , OpalConnection::EndedBySecurityDenial    ,  21 }, // Call rejected (*)
  { SIP_PDU::Failure_PaymentRequired            , OpalConnection::EndedByQ931Cause         ,  21 }, // Call rejected
  { SIP_PDU::Failure_Forbidden                  , OpalConnection::EndedBySecurityDenial    ,  21 }, // Call rejected
  { SIP_PDU::Failure_NotFound                   , OpalConnection::EndedByNoUser            ,   1 }, // Unallocated number
  { SIP_PDU::Failure_MethodNotAllowed           , OpalConnection::EndedByQ931Cause         ,  63 }, // Service or option unavailable
  { SIP_PDU::Failure_NotAcceptable              , OpalConnection::EndedByQ931Cause         ,  79 }, // Service/option not implemented (+)
  { SIP_PDU::Failure_ProxyAuthenticationRequired, OpalConnection::EndedByQ931Cause         ,  21 }, // Call rejected (*)
  { SIP_PDU::Failure_RequestTimeout             , OpalConnection::EndedByTemporaryFailure  , 102 }, // Recovery on timer expiry
  { SIP_PDU::Failure_Gone                       , OpalConnection::EndedByQ931Cause         ,  22 }, // Number changed (w/o diagnostic)
  { SIP_PDU::Failure_RequestEntityTooLarge      , OpalConnection::EndedByQ931Cause         , 127 }, // Interworking (+)
  { SIP_PDU::Failure_RequestURITooLong          , OpalConnection::EndedByQ931Cause         , 127 }, // Interworking (+)
  { SIP_PDU::Failure_UnsupportedMediaType       , OpalConnection::EndedByCapabilityExchange,  79 }, // Service/option not implemented (+)
  { SIP_PDU::Failure_UnsupportedURIScheme       , OpalConnection::EndedByQ931Cause         , 127 }, // Interworking (+)
  { SIP_PDU::Failure_BadExtension               , OpalConnection::EndedByQ931Cause         , 127 }, // Interworking (+)
  { SIP_PDU::Failure_ExtensionRequired          , OpalConnection::EndedByQ931Cause         , 127 }, // Interworking (+)
  { SIP_PDU::Failure_IntervalTooBrief           , OpalConnection::EndedByQ931Cause         , 127 }, // Interworking (+)
  { SIP_PDU::Failure_TemporarilyUnavailable     , OpalConnection::EndedByTemporaryFailure  ,  18 }, // No user responding
  { SIP_PDU::Failure_TransactionDoesNotExist    , OpalConnection::EndedByQ931Cause         ,  41 }, // Temporary Failure
  { SIP_PDU::Failure_LoopDetected               , OpalConnection::EndedByQ931Cause         ,  25 }, // Exchange - routing error
  { SIP_PDU::Failure_TooManyHops                , OpalConnection::EndedByQ931Cause         ,  25 }, // Exchange - routing error
  { SIP_PDU::Failure_AddressIncomplete          , OpalConnection::EndedByQ931Cause         ,  28 }, // Invalid Number Format (+)
  { SIP_PDU::Failure_Ambiguous                  , OpalConnection::EndedByNoUser            ,   1 }, // Unallocated number
  { SIP_PDU::Failure_BusyHere                   , OpalConnection::EndedByRemoteBusy        ,  17 }, // User busy
  { SIP_PDU::Failure_InternalServerError        , OpalConnection::EndedByQ931Cause         ,  41 }, // Temporary failure
  { SIP_PDU::Failure_NotImplemented             , OpalConnection::EndedByQ931Cause         ,  79 }, // Not implemented, unspecified
  { SIP_PDU::Failure_BadGateway                 , OpalConnection::EndedByQ931Cause         ,  38 }, // Network out of order
  { SIP_PDU::Failure_ServiceUnavailable         , OpalConnection::EndedByQ931Cause         ,  41 }, // Temporary failure
  { SIP_PDU::Failure_ServerTimeout              , OpalConnection::EndedByQ931Cause         , 102 }, // Recovery on timer expiry
  { SIP_PDU::Failure_SIPVersionNotSupported     , OpalConnection::EndedByQ931Cause         , 127 }, // Interworking (+)
  { SIP_PDU::Failure_MessageTooLarge            , OpalConnection::EndedByQ931Cause         , 127 }, // Interworking (+)
  { SIP_PDU::GlobalFailure_BusyEverywhere       , OpalConnection::EndedByQ931Cause         ,  17 }, // User busy
  { SIP_PDU::GlobalFailure_Decline              , OpalConnection::EndedByRefusal           ,  21 }, // Call rejected
  { SIP_PDU::GlobalFailure_DoesNotExistAnywhere , OpalConnection::EndedByNoUser            ,   1 }, // Unallocated number
};


////////////////////////////////////////////////////////////////////////////

SIPConnection::SIPConnection(OpalCall & call,
                             SIPEndPoint & ep,
                             const PString & token,
                             const SIPURL & destination,
                             OpalTransport * newTransport,
                             unsigned int options,
                             OpalConnection::StringOptions * stringOptions)
  : OpalConnection(call, ep, token, options, stringOptions),
    endpoint(ep),
    transport(newTransport),
    pduSemaphore(0, P_MAX_INDEX)
{
  targetAddress = destination;

  // Look for a "proxy" parameter to override default proxy
  PStringToString params = targetAddress.GetParamVars();
  SIPURL proxy;
  if (params.Contains("proxy")) {
    proxy.Parse(params("proxy"));
    targetAddress.SetParamVar("proxy", PString::Empty());
  }

  if (proxy.IsEmpty())
    proxy = endpoint.GetProxy();

  // Default routeSet if there is a proxy
  if (!proxy.IsEmpty()) 
    routeSet += "sip:" + proxy.GetHostName() + ':' + PString(proxy.GetPort()) + ";lr";
  
  // Update remote party parameters
  remotePartyAddress = targetAddress.AsQuotedString();
  UpdateRemotePartyNameAndNumber();
  
  originalInvite = NULL;
  pduHandler = NULL;
  lastSentCSeq.SetValue(0);
  releaseMethod = ReleaseWithNothing;

  forkedInvitations.DisallowDeleteObjects();

  referTransaction = (SIPTransaction *)NULL;
  local_hold = FALSE;
  remote_hold = FALSE;

  ackTimer.SetNotifier(PCREATE_NOTIFIER(OnAckTimeout));

  PTRACE(4, "SIP\tCreated connection.");
}


void SIPConnection::UpdateRemotePartyNameAndNumber()
{
  SIPURL url(remotePartyAddress);
  remotePartyName   = url.GetDisplayName ();
  remotePartyNumber = url.GetUserName();
}

SIPConnection::~SIPConnection()
{
  delete originalInvite;
  delete transport;

  if (pduHandler) 
    delete pduHandler;

  PTRACE(4, "SIP\tDeleted connection.");
}

void SIPConnection::OnReleased()
{
  PTRACE(3, "SIP\tOnReleased: " << *this << ", phase = " << phase);
  
  // OpalConnection::Release sets the phase to Releasing in the SIP Handler 
  // thread
  if (GetPhase() >= ReleasedPhase){
    PTRACE(2, "SIP\tOnReleased: already released");
    return;
  };

  SetPhase(ReleasingPhase);

  PSafePtr<SIPTransaction> byeTransaction;

  switch (releaseMethod) {
    case ReleaseWithNothing :
      break;

    case ReleaseWithResponse :
      {
        SIP_PDU::StatusCodes sipCode = SIP_PDU::Failure_BadGateway;

        // Try find best match for return code
        for (PINDEX i = 0; i < PARRAYSIZE(ReasonToSIPCode); i++) {
          if (ReasonToSIPCode[i].q931Cause == GetQ931Cause()) {
            sipCode = ReasonToSIPCode[i].code;
            break;
          }
          if (ReasonToSIPCode[i].reason == callEndReason) {
            sipCode = ReasonToSIPCode[i].code;
            break;
          }
        }

        // EndedByCallForwarded is a special case because it needs extra paramater
        SendInviteResponse(sipCode, NULL, callEndReason == EndedByCallForwarded ? (const char *)forwardParty : NULL);
      }
      break;

    case ReleaseWithBYE :
      // create BYE now & delete it later to prevent memory access errors
      byeTransaction = new SIPTransaction(*this, *transport, SIP_PDU::Method_BYE);
      break;

    case ReleaseWithCANCEL :
      {
        PTRACE(3, "SIP\tCancelling " << forkedInvitations.GetSize() << " transactions.");
        for (PSafePtr<SIPTransaction> invitation(forkedInvitations, PSafeReference); invitation != NULL; ++invitation)
          invitation->Cancel();
      }
  }

  // Close media
  CloseMediaStreams();

  // Sent a BYE, wait for it to complete
  if (byeTransaction != NULL)
    byeTransaction->WaitForCompletion();

  // Wait until all INVITEs have completed
  for (PSafePtr<SIPTransaction> invitation(forkedInvitations, PSafeReference); invitation != NULL; ++invitation)
    invitation->WaitForCompletion();
  forkedInvitations.RemoveAll();

  SetPhase(ReleasedPhase);

  if (pduHandler != NULL) {
    pduSemaphore.Signal();
    pduHandler->WaitForTermination();
  }
  
  OpalConnection::OnReleased();

  if (transport != NULL)
    transport->CloseWait();
}


void SIPConnection::TransferConnection(const PString & remoteParty, const PString & callIdentity)
{
  // There is still an ongoing REFER transaction 
  if (referTransaction != NULL) 
    return;
 
  referTransaction = new SIPRefer(*this, *transport, remoteParty, callIdentity);
  referTransaction->Start ();
}

BOOL SIPConnection::ConstructSDP(SDPSessionDescription & sdpOut)
{
  BOOL sdpOK;

  // get the remote media formats, if any
  if (originalInvite->HasSDP()) {
    remoteSDP = originalInvite->GetSDP();

    // Use |= to avoid McCarthy boolean || from not calling video/fax
    sdpOK  = OnSendSDPMediaDescription(remoteSDP, SDPMediaDescription::Audio, OpalMediaFormat::DefaultAudioSessionID, sdpOut);
#if OPAL_VIDEO
    sdpOK |= OnSendSDPMediaDescription(remoteSDP, SDPMediaDescription::Video, OpalMediaFormat::DefaultVideoSessionID, sdpOut);
#endif
#if OPAL_T38FAX
    sdpOK |= OnSendSDPMediaDescription(remoteSDP, SDPMediaDescription::Image, OpalMediaFormat::DefaultDataSessionID,  sdpOut);
#endif
  }
  
  else {

    // construct offer as per RFC 3261, para 14.2
    // Use |= to avoid McCarthy boolean || from not calling video/fax
    SDPSessionDescription *sdp = (SDPSessionDescription *) &sdpOut;
    sdpOK  = BuildSDP(sdp, rtpSessions, OpalMediaFormat::DefaultAudioSessionID);
#if OPAL_VIDEO
    sdpOK |= BuildSDP(sdp, rtpSessions, OpalMediaFormat::DefaultVideoSessionID);
#endif
#if OPAL_T38FAX
    sdpOK |= BuildSDP(sdp, rtpSessions, OpalMediaFormat::DefaultDataSessionID);
#endif
  }

  if (sdpOK)
    return phase < ReleasingPhase; // abort if already in releasing phase

  Release(EndedByCapabilityExchange);
  return FALSE;
}

BOOL SIPConnection::SetAlerting(const PString & /*calleeName*/, BOOL withMedia)
{
  if (IsOriginating()) {
    PTRACE(2, "SIP\tSetAlerting ignored on call we originated.");
    return TRUE;
  }

  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked())
    return FALSE;
  
  PTRACE(3, "SIP\tSetAlerting");

  if (phase != SetUpPhase) 
    return FALSE;

  if (!withMedia) 
    SendInviteResponse(SIP_PDU::Information_Ringing);
  else {
    SDPSessionDescription sdpOut(GetLocalAddress());
    if (!ConstructSDP(sdpOut)) {
      SendInviteResponse(SIP_PDU::Failure_NotAcceptableHere);
      return FALSE;
    }
    if (!SendInviteResponse(SIP_PDU::Information_Session_Progress, NULL, NULL, &sdpOut))
      return FALSE;
  }

  SetPhase(AlertingPhase);

  return TRUE;
}


BOOL SIPConnection::SetConnected()
{
  if (transport == NULL) {
    Release(EndedByTransportFail);
    return FALSE;
  }

  if (IsOriginating()) {
    PTRACE(2, "SIP\tSetConnected ignored on call we originated.");
    return TRUE;
  }

  PSafeLockReadWrite safeLock(*this);
  if (!safeLock.IsLocked())
    return FALSE;
  
  if (GetPhase() >= ConnectedPhase) {
    PTRACE(2, "SIP\tSetConnected ignored on already connected call.");
    return FALSE;
  }
  
  PTRACE(3, "SIP\tSetConnected");

  SDPSessionDescription sdpOut(GetLocalAddress());

  if (!ConstructSDP(sdpOut)) {
    SendInviteResponse(SIP_PDU::Failure_NotAcceptableHere);
    return FALSE;
  }
    
  // update the route set and the target address according to 12.1.1
  // requests in a dialog do not modify the route set according to 12.2
  if (phase < ConnectedPhase) {
    routeSet.RemoveAll();
    routeSet = originalInvite->GetMIME().GetRecordRoute();
    PString originalContact = originalInvite->GetMIME().GetContact();
    if (!originalContact.IsEmpty()) {
      targetAddress = originalContact;
    }
  }

  // send the 200 OK response
  SendInviteOK(sdpOut);

  // switch phase 
  SetPhase(ConnectedPhase);
  connectedTime = PTime ();
  
  return TRUE;
}


RTP_UDP *SIPConnection::OnUseRTPSession(const unsigned rtpSessionId, const OpalTransportAddress & mediaAddress, OpalTransportAddress & localAddress)
{
  RTP_UDP *rtpSession = NULL;
  RTP_DataFrame::PayloadTypes ntePayloadCode = RTP_DataFrame::IllegalPayloadType;

  {
    PSafeLockReadOnly m(ownerCall);
    PSafePtr<OpalConnection> otherParty = GetCall().GetOtherPartyConnection(*this);
    if (otherParty == NULL) {
      PTRACE(2, "H323\tCowardly refusing to create an RTP channel with only one connection");
      return NULL;
     }

    // if doing media bypass, we need to set the local address
    // otherwise create an RTP session
    if (ownerCall.IsMediaBypassPossible(*this, rtpSessionId)) {
      OpalConnection * otherParty = GetCall().GetOtherPartyConnection(*this);
      if (otherParty != NULL) {
        MediaInformation info;
        if (otherParty->GetMediaInformation(rtpSessionId, info)) {
          localAddress = info.data;
          ntePayloadCode = info.rfc2833;
        }
      }
      mediaTransportAddresses.SetAt(rtpSessionId, new OpalTransportAddress(mediaAddress));
    }
    else {
      // create an RTP session
      rtpSession = (RTP_UDP *)UseSession(GetTransport(), rtpSessionId, NULL);
      if (rtpSession == NULL) {
        return NULL;
      }
      
      // Set user data
      if (rtpSession->GetUserData() == NULL)
        rtpSession->SetUserData(new SIP_RTP_Session(*this));
  
      // Local Address of the session
      localAddress = GetLocalAddress(rtpSession->GetLocalDataPort());
    }
  }
  
  return rtpSession;
}


BOOL SIPConnection::OnSendSDPMediaDescription(const SDPSessionDescription & sdpIn,
                                              SDPMediaDescription::MediaType rtpMediaType,
                                              unsigned rtpSessionId,
                                              SDPSessionDescription & sdpOut)
{
  RTP_UDP * rtpSession = NULL;

  // if no matching media type, return FALSE
  SDPMediaDescription * incomingMedia = sdpIn.GetMediaDescription(rtpMediaType);
  if (incomingMedia == NULL) {
    PTRACE(2, "SIP\tCould not find matching media type for session " << rtpSessionId);
    return FALSE;
  }

  if (incomingMedia->GetMediaFormats(rtpSessionId).GetSize() == 0) {
    PTRACE(1, "SIP\tCould not find media formats in SDP media description for session " << rtpSessionId);
    return FALSE;
  }
  
  // Create the list of Opal format names for the remote end.
  // We will answer with the media format that will be opened.
  // When sending an answer SDP, remove media formats that we do not support.
  remoteFormatList += incomingMedia->GetMediaFormats(rtpSessionId);
  remoteFormatList.Remove(endpoint.GetManager().GetMediaFormatMask());
  if (remoteFormatList.GetSize() == 0) {
    Release(EndedByCapabilityExchange);
    return FALSE;
  }

  SDPMediaDescription * localMedia = NULL;
  OpalTransportAddress mediaAddress = incomingMedia->GetTransportAddress();

  {
    // find the payload type used for telephone-event, if present
    const SDPMediaFormatList & sdpMediaList = incomingMedia->GetSDPMediaFormats();
    BOOL hasTelephoneEvent = FALSE;
#if OPAL_T38FAX
    BOOL hasNSE = FALSE;
#endif
    PINDEX i;
    for (i = 0; !hasTelephoneEvent && (i < sdpMediaList.GetSize()); i++) {
      if (sdpMediaList[i].GetEncodingName() == "telephone-event") {
        rfc2833Handler->SetPayloadType(sdpMediaList[i].GetPayloadType());
        hasTelephoneEvent = TRUE;
        remoteFormatList += OpalRFC2833;
      }
#if OPAL_T38FAX
      if (sdpMediaList[i].GetEncodingName() == "nse") {
        ciscoNSEHandler->SetPayloadType(sdpMediaList[i].GetPayloadType());
        hasNSE = TRUE;
        remoteFormatList += OpalCiscoNSE;
      }
#endif
    }

    // Create the RTPSession if required
    OpalTransportAddress localAddress;
    rtpSession = OnUseRTPSession(rtpSessionId, mediaAddress, localAddress);
    if (rtpSession == NULL && !ownerCall.IsMediaBypassPossible(*this, rtpSessionId)) {
      if (rtpSessionId == OpalMediaFormat::DefaultAudioSessionID) 
        Release(EndedByTransportFail);
      return FALSE;
    }

    // set the remote address
    PIPSocket::Address ip;
    WORD port;
    if (!mediaAddress.GetIpAndPort(ip, port) || (rtpSession && !rtpSession->SetRemoteSocketInfo(ip, port, TRUE))) {
      PTRACE(1, "SIP\tCannot set remote ports on RTP session");
      if (rtpSessionId == OpalMediaFormat::DefaultAudioSessionID) 
        Release(EndedByTransportFail);
      return FALSE;
    }

    // construct a new media session list 
    localMedia = new SDPMediaDescription(localAddress, rtpMediaType);
  
    // create map for RTP payloads
    incomingMedia->CreateRTPMap(rtpSessionId, rtpPayloadMap);

    // Open the streams and the reverse media streams
    BOOL reverseStreamsFailed = TRUE;
    reverseStreamsFailed = OnOpenSourceMediaStreams(remoteFormatList, rtpSessionId, localMedia);
  
    // Add in the RFC2833 handler, if used
    if (hasTelephoneEvent) {
      localMedia->AddSDPMediaFormat(new SDPMediaFormat(OpalRFC2833, rfc2833Handler->GetPayloadType(), OpalDefaultNTEString));
    }
#if OPAL_T38FAX
    if (hasNSE) {
      localMedia->AddSDPMediaFormat(new SDPMediaFormat(OpalCiscoNSE, ciscoNSEHandler->GetPayloadType(), OpalDefaultNSEString));
    }
#endif

    // No stream opened for this session, use the default SDP
    if (reverseStreamsFailed) {
      SDPSessionDescription *sdp = (SDPSessionDescription *) &sdpOut;
      if (!BuildSDP(sdp, rtpSessions, rtpSessionId)) {
        delete localMedia;
        return FALSE;
      }
      return TRUE;
    }
  }

  localMedia->SetDirection(GetDirection(rtpSessionId));
  sdpOut.AddMediaDescription(localMedia);

  return TRUE;
}


BOOL SIPConnection::OnOpenSourceMediaStreams(const OpalMediaFormatList & remoteFormatList, unsigned sessionId, SDPMediaDescription *localMedia)
{
  BOOL reverseStreamsFailed = TRUE;

  ownerCall.OpenSourceMediaStreams(*this, remoteFormatList, sessionId);

  OpalMediaFormatList otherList;
  {
    PSafePtr<OpalConnection> otherParty = GetCall().GetOtherPartyConnection(*this);
    if (otherParty == NULL) {
      PTRACE(1, "SIP\tCannot get other connection");
      return FALSE;
    }
    otherList = otherParty->GetMediaFormats();
  }

  for (PINDEX i = 0; i < mediaStreams.GetSize(); i++) {
    OpalMediaStream & mediaStream = mediaStreams[i];
    if (mediaStream.GetSessionID() == sessionId) {
      if (OpenSourceMediaStream(otherList, sessionId) && localMedia) {
        localMedia->AddMediaFormat(mediaStream.GetMediaFormat(), rtpPayloadMap);
        reverseStreamsFailed = FALSE;
      }
    }
  }

  return reverseStreamsFailed;
}


SDPMediaDescription::Direction SIPConnection::GetDirection(
#if OPAL_VIDEO
  unsigned sessionId
#else
  unsigned 
#endif
  )
{
  if (remote_hold)
    return SDPMediaDescription::RecvOnly;

  if (local_hold)
    return SDPMediaDescription::SendOnly;

#if OPAL_VIDEO
  if (sessionId == OpalMediaFormat::DefaultVideoSessionID) {
    if (endpoint.GetManager().CanAutoStartTransmitVideo() && !endpoint.GetManager().CanAutoStartReceiveVideo())
      return SDPMediaDescription::SendOnly;
    if (!endpoint.GetManager().CanAutoStartTransmitVideo() && endpoint.GetManager().CanAutoStartReceiveVideo())
      return SDPMediaDescription::RecvOnly;
  }
#endif
  
  return SDPMediaDescription::Undefined;
}


OpalTransportAddress SIPConnection::GetLocalAddress(WORD port) const
{
  PIPSocket::Address localIP;
  if (!transport->GetLocalAddress().GetIpAddress(localIP)) {
    PTRACE(1, "SIP\tNot using IP transport");
    return OpalTransportAddress();
  }

  PIPSocket::Address remoteIP;
  if (!transport->GetRemoteAddress().GetIpAddress(remoteIP)) {
    PTRACE(1, "SIP\tNot using IP transport");
    return OpalTransportAddress();
  }

  endpoint.GetManager().TranslateIPAddress(localIP, remoteIP);
  return OpalTransportAddress(localIP, port, "udp");
}


OpalMediaFormatList SIPConnection::GetMediaFormats() const
{
  return remoteFormatList;
}


BOOL SIPConnection::OpenSourceMediaStream(const OpalMediaFormatList & mediaFormats,
                                          unsigned sessionID)
{
  // The remote user is in recvonly mode or in inactive mode for that session
  switch (remoteSDP.GetDirection(sessionID)) {
    case SDPMediaDescription::Inactive :
    case SDPMediaDescription::RecvOnly :
      return FALSE;

    default :
      return OpalConnection::OpenSourceMediaStream(mediaFormats, sessionID);
  }
}


OpalMediaStream * SIPConnection::OpenSinkMediaStream(OpalMediaStream & source)
{
  // The remote user is in sendonly mode or in inactive mode for that session
  switch (remoteSDP.GetDirection(source.GetSessionID())) {
    case SDPMediaDescription::Inactive :
    case SDPMediaDescription::SendOnly :
      return NULL;

    default :
      return OpalConnection::OpenSinkMediaStream(source);
  }
}


OpalMediaStream * SIPConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat,
                                                   unsigned sessionID,
                                                   BOOL isSource)
{
  // Use a NULL stream if media is bypassing us, 
  if (ownerCall.IsMediaBypassPossible(*this, sessionID)) {
    PTRACE(3, "SIP\tBypassing media for session " << sessionID);
    return new OpalNullMediaStream(*this, mediaFormat, sessionID, isSource);
  }

  // if no RTP sessions matching this session ID, then nothing to do
  if (rtpSessions.GetSession(sessionID) == NULL)
    return NULL;

  return new OpalRTPMediaStream(*this, mediaFormat, isSource, *rtpSessions.GetSession(sessionID),
                                GetMinAudioJitterDelay(),
                                GetMaxAudioJitterDelay());
}

void SIPConnection::OnPatchMediaStream(BOOL isSource, OpalMediaPatch & patch)
{
  OpalConnection::OnPatchMediaStream(isSource, patch);
  if(patch.GetSource().GetSessionID() == OpalMediaFormat::DefaultAudioSessionID) {
    AttachRFC2833HandlerToPatch(isSource, patch);
  }
}


void SIPConnection::OnConnected ()
{
  OpalConnection::OnConnected ();
}


BOOL SIPConnection::IsMediaBypassPossible(unsigned sessionID) const
{
  return sessionID == OpalMediaFormat::DefaultAudioSessionID ||
         sessionID == OpalMediaFormat::DefaultVideoSessionID;
}


BOOL SIPConnection::WriteINVITE(OpalTransport & transport, void * param)
{
  SIPConnection & connection = *(SIPConnection *)param;

  connection.SetLocalPartyAddress();

  SIPTransaction * invite = new SIPInvite(connection, transport);
  
  // It may happen that constructing the INVITE causes the connection
  // to be released (e.g. there are no UDP ports available for the RTP sessions)
  // Since the connection is released immediately, a INVITE must not be
  // sent out.
  if (connection.GetPhase() >= OpalConnection::ReleasingPhase) {
    PTRACE(2, "SIP\tAborting INVITE transaction since connection is in releasing phase");
    delete invite; // Before Start() is called we are responsible for deletion
    return FALSE;
  }
  
  if (invite->Start()) {
    connection.forkedInvitations.Append(invite);
    return TRUE;
  }

  PTRACE(2, "SIP\tDid not start INVITE transaction on " << transport);
  return FALSE;
}


BOOL SIPConnection::SetUpConnection()
{
  PTRACE(3, "SIP\tSetUpConnection: " << remotePartyAddress);

  ApplyStringOptions();

  SIPURL transportAddress;

  PStringList routeSet = GetRouteSet();
  if (!routeSet.IsEmpty()) 
    transportAddress = routeSet[0];
  else {
    transportAddress = targetAddress;
    transportAddress.AdjustToDNS(); // Do a DNS SRV lookup
    PTRACE(4, "SIP\tConnecting to " << targetAddress << " via " << transportAddress);
  }

  originating = TRUE;

  delete transport;
  transport = endpoint.CreateTransport(transportAddress.GetHostAddress());
  if (transport == NULL) {
    Release(EndedByUnreachable);
    return FALSE;
  }

  if (!transport->WriteConnect(WriteINVITE, this)) {
    PTRACE(1, "SIP\tCould not write to " << transportAddress << " - " << transport->GetErrorText());
    Release(EndedByTransportFail);
    return FALSE;
  }

  releaseMethod = ReleaseWithCANCEL;
  return TRUE;
}


void SIPConnection::HoldConnection()
{
  if (local_hold || transport == NULL)
    return;

  PTRACE(3, "SIP\tWill put connection on hold");

  local_hold = TRUE;

  SIPTransaction * invite = new SIPInvite(*this, *transport, rtpSessions);
  if (invite->Start()) {
    // Pause the media streams
    PauseMediaStreams(TRUE);
    
    // Signal the manager that there is a hold
    endpoint.OnHold(*this);
  }
  else
    local_hold = FALSE;
}


void SIPConnection::RetrieveConnection()
{
  if (!local_hold)
    return;

  local_hold = FALSE;

  if (transport == NULL)
    return;

  PTRACE(3, "SIP\tWill retrieve connection from hold");

  SIPTransaction * invite = new SIPInvite(*this, *transport, rtpSessions);
  if (invite->Start()) {
    // Un-Pause the media streams
    PauseMediaStreams(FALSE);

    // Signal the manager that there is a hold
    endpoint.OnHold(*this);
  }
}


BOOL SIPConnection::IsConnectionOnHold()
{
  return (local_hold || remote_hold);
}

static void SetNXEPayloadCode(SDPMediaDescription * localMedia, 
                      RTP_DataFrame::PayloadTypes & nxePayloadCode, 
                              OpalRFC2833Proto    * handler,
                            const OpalMediaFormat &  mediaFormat,
                                       const char * defaultNXEString, 
                                       const char * PTRACE_PARAM(label))
{
  if (nxePayloadCode != RTP_DataFrame::IllegalPayloadType) {
    PTRACE(3, "SIP\tUsing bypass RTP payload " << nxePayloadCode << " for " << label);
    localMedia->AddSDPMediaFormat(new SDPMediaFormat(mediaFormat, nxePayloadCode, defaultNXEString));
  }
  else {
    nxePayloadCode = handler->GetPayloadType();
    if (nxePayloadCode == RTP_DataFrame::IllegalPayloadType) {
      nxePayloadCode = mediaFormat.GetPayloadType();
    }
    if (nxePayloadCode != RTP_DataFrame::IllegalPayloadType) {
      PTRACE(3, "SIP\tUsing RTP payload " << nxePayloadCode << " for " << label);

      // create and add the NXE media format
      localMedia->AddSDPMediaFormat(new SDPMediaFormat(mediaFormat, nxePayloadCode, defaultNXEString));
    }
    else {
      PTRACE(2, "SIP\tCould not allocate dynamic RTP payload for " << label);
    }
  }

  handler->SetPayloadType(nxePayloadCode);
}

BOOL SIPConnection::BuildSDP(SDPSessionDescription * & sdp, 
                             RTP_SessionManager & rtpSessions,
                             unsigned rtpSessionId)
{
  OpalTransportAddress localAddress;
  RTP_DataFrame::PayloadTypes ntePayloadCode = RTP_DataFrame::IllegalPayloadType;
#if OPAL_T38FAX
  RTP_DataFrame::PayloadTypes nsePayloadCode = RTP_DataFrame::IllegalPayloadType;
#endif

#if OPAL_VIDEO
  if (rtpSessionId == OpalMediaFormat::DefaultVideoSessionID &&
           !endpoint.GetManager().CanAutoStartReceiveVideo() &&
           !endpoint.GetManager().CanAutoStartTransmitVideo())
    return FALSE;
#endif

  OpalMediaFormatList formats = GetLocalMediaFormats();

  // See if any media formats of this session id, so don't create unused RTP session
  PINDEX i;
  for (i = 0; i < formats.GetSize(); i++) {
    OpalMediaFormat & fmt = formats[i];
    if (fmt.GetDefaultSessionID() == rtpSessionId &&
            (rtpSessionId == OpalMediaFormat::DefaultDataSessionID || fmt.IsTransportable()))
      break;
  }
  if (i >= formats.GetSize()) {
    PTRACE(3, "SIP\tNo media formats for session id " << rtpSessionId << ", not adding SDP");
    return FALSE;
  }

  if (ownerCall.IsMediaBypassPossible(*this, rtpSessionId)) {
    PSafePtr<OpalConnection> otherParty = GetCall().GetOtherPartyConnection(*this);
    if (otherParty != NULL) {
      MediaInformation info;
      if (otherParty->GetMediaInformation(rtpSessionId, info)) {
        localAddress = info.data;
        ntePayloadCode = info.rfc2833;
#if OPAL_T38FAX
        nsePayloadCode = info.ciscoNSE;
#endif
      }
    }
  }

  if (localAddress.IsEmpty()) {

    /* We are not doing media bypass, so must have some media session
       Due to the possibility of several INVITEs going out, all with different
       transport requirements, we actually need to use an rtpSession dictionary
       for each INVITE and not the one for the connection. Once an INVITE is
       accepted the rtpSessions for that INVITE is put into the connection. */
    RTP_Session * rtpSession = rtpSessions.UseSession(rtpSessionId);
    if (rtpSession == NULL) {

      // Not already there, so create one
      rtpSession = CreateSession(GetTransport(), rtpSessionId, NULL);
      if (rtpSession == NULL) {
        PTRACE(1, "SIP\tCould not create session id " << rtpSessionId << ", released " << *this);
        Release(OpalConnection::EndedByTransportFail);
        return FALSE;
      }

      rtpSession->SetUserData(new SIP_RTP_Session(*this));

      // add the RTP session to the RTP session manager in INVITE
      rtpSessions.AddSession(rtpSession);
    }

    if (rtpSession != NULL)
      localAddress = GetLocalAddress(((RTP_UDP *)rtpSession)->GetLocalDataPort());
  } 

  if (sdp == NULL)
    sdp = new SDPSessionDescription(localAddress);

  SDPMediaDescription * localMedia = NULL;
  switch (rtpSessionId) {
    case OpalMediaFormat::DefaultAudioSessionID:
      localMedia = new SDPMediaDescription(localAddress, SDPMediaDescription::Audio);
      break;

#if OPAL_VIDEO
    case OpalMediaFormat::DefaultVideoSessionID:
      localMedia = new SDPMediaDescription(localAddress, SDPMediaDescription::Video);
      break;
#endif

#if OPAL_T38FAX
    case OpalMediaFormat::DefaultDataSessionID:
#endif
      if (!localAddress.IsEmpty()) 
        localMedia = new SDPMediaDescription(localAddress, SDPMediaDescription::Image);
      else {
        PTRACE(2, "SIP\tRefusing to add SDP media description for session id " << rtpSessionId << " with no transport address");
      }
      break;

    default:
      break;
  }

  // add media formats first, as Mediatrix gateways barf if RFC2833 is first
  if (localMedia != NULL)
    localMedia->AddMediaFormats(formats, rtpSessionId, rtpPayloadMap);

  // Set format if we have an RTP payload type for RFC2833 and/or NSE
  if (rtpSessionId == OpalMediaFormat::DefaultAudioSessionID && (localMedia != NULL)) {

    // RFC 2833
    SetNXEPayloadCode(localMedia, ntePayloadCode, rfc2833Handler,  OpalRFC2833, OpalDefaultNTEString, "NTE");

#if OPAL_T38FAX
    // Cisco NSE
    SetNXEPayloadCode(localMedia, nsePayloadCode, ciscoNSEHandler, OpalCiscoNSE, OpalDefaultNSEString, "NSE");
#endif
  }
  
  if (localMedia != NULL) {
    localMedia->SetDirection(GetDirection(rtpSessionId));
    sdp->AddMediaDescription(localMedia);
  }

  return TRUE;
}


void SIPConnection::SetLocalPartyAddress()
{
  SIPURL registeredPartyName = endpoint.GetRegisteredPartyName(remotePartyAddress);
  localPartyAddress = registeredPartyName.AsQuotedString() + ";tag=" + OpalGloballyUniqueID().AsString();

  // allow callers to override the From field
  if (stringOptions != NULL) {
    SIPURL newFrom(GetLocalPartyAddress());
    PString number((*stringOptions)("Calling-Party-Number"));
    if (!number.IsEmpty())
      newFrom.SetUserName(number);

    PString name((*stringOptions)("Calling-Party-Name"));
    if (!name.IsEmpty())
      newFrom.SetDisplayName(name);

    explicitFrom = newFrom.AsQuotedString();

    PTRACE(1, "SIP\tChanging From from " << GetLocalPartyAddress() << " to " << explicitFrom << " using " << name << " and " << number);
  }
}


const PString SIPConnection::GetRemotePartyCallbackURL() const
{
  SIPURL url = GetRemotePartyAddress();
  url.AdjustForRequestURI();
  return url.AsString();
}


void SIPConnection::OnTransactionFailed(SIPTransaction & transaction)
{
  if (transaction.GetMethod() != SIP_PDU::Method_INVITE)
    return;

  // If we are releasing then I can safely ignore failed
  // transactions - otherwise I'll deadlock.
  if (phase >= ReleasingPhase)
    return;

  {
    // The connection stays alive unless all INVITEs have failed
    for (PSafePtr<SIPTransaction> invitation(forkedInvitations, PSafeReference); invitation != NULL; ++invitation) {
      if (!invitation->IsFailed())
        return;
    }
  }

  // All invitations failed, die now
  releaseMethod = ReleaseWithNothing;
  Release(EndedByConnectFail);
}


void SIPConnection::OnReceivedPDU(SIP_PDU & pdu)
{
  PTRACE(4, "SIP\tHandling PDU " << pdu);

  if (!LockReadWrite())
    return;

  switch (pdu.GetMethod()) {
    case SIP_PDU::Method_INVITE :
      OnReceivedINVITE(pdu);
      break;
    case SIP_PDU::Method_ACK :
      OnReceivedACK(pdu);
      break;
    case SIP_PDU::Method_CANCEL :
      OnReceivedCANCEL(pdu);
      break;
    case SIP_PDU::Method_BYE :
      OnReceivedBYE(pdu);
      break;
    case SIP_PDU::Method_OPTIONS :
      OnReceivedOPTIONS(pdu);
      break;
    case SIP_PDU::Method_NOTIFY :
      OnReceivedNOTIFY(pdu);
      break;
    case SIP_PDU::Method_REFER :
      OnReceivedREFER(pdu);
      break;
    case SIP_PDU::Method_INFO :
      OnReceivedINFO(pdu);
      break;
    case SIP_PDU::Method_PING :
      OnReceivedPING(pdu);
      break;
    case SIP_PDU::Method_MESSAGE :
    case SIP_PDU::Method_SUBSCRIBE :
    case SIP_PDU::Method_REGISTER :
    case SIP_PDU::Method_PUBLISH :
      // Shouldn't have got this!
      break;
    case SIP_PDU::NumMethods :  // if no method, must be response
      {
        UnlockReadWrite(); // Need to avoid deadlocks with transaction mutex
        PSafePtr<SIPTransaction> transaction = endpoint.GetTransaction(pdu.GetTransactionID(), PSafeReference);
        if (transaction != NULL)
          transaction->OnReceivedResponse(pdu);
      }
      return;
  }

  UnlockReadWrite();
}


void SIPConnection::OnReceivedResponse(SIPTransaction & transaction, SIP_PDU & response)
{
  PSafeLockReadWrite lock(*this);
  if (!lock.IsLocked())
    return;

  PINDEX i;
  BOOL ignoreErrorResponse = FALSE;
  BOOL reInvite = FALSE;

  if (transaction.GetMethod() == SIP_PDU::Method_INVITE) {
    // See if this is an initial INVITE or a re-INVITE
    reInvite = TRUE;
    for (PSafePtr<SIPTransaction> invitation(forkedInvitations, PSafeReference); invitation != NULL; ++invitation) {
      if (invitation == &transaction) {
        reInvite = FALSE;
        break;
      }
    }

    if (!reInvite && response.GetStatusCode()/100 <= 2) {
      if (response.GetStatusCode()/100 == 2) {
        // Have a final response to the INVITE, so cancel all the other invitations sent.
        for (PSafePtr<SIPTransaction> invitation(forkedInvitations, PSafeReference); invitation != NULL; ++invitation) {
          if (invitation != &transaction)
            invitation->Cancel();
        }

        // And end connect mode on the transport
        transport->EndConnect(transaction.GetTransport().GetInterface());
      }

      // Save the sessions etc we are actually using
      // If we are in the EstablishedPhase, then the 
      // sessions are kept identical because the response is the
      // response to a hold/retrieve
      rtpSessions = ((SIPInvite &)transaction).GetSessionManager();
      localPartyAddress = transaction.GetMIME().GetFrom();
      remotePartyAddress = response.GetMIME().GetTo();
      UpdateRemotePartyNameAndNumber();

      response.GetMIME().GetProductInfo(remoteProductInfo);

      // get the route set from the Record-Route response field (in reverse order)
      // according to 12.1.2
      // requests in a dialog do not modify the initial route set fo according 
      // to 12.2
      if (phase < ConnectedPhase) {
        PStringList recordRoute = response.GetMIME().GetRecordRoute();
        routeSet.RemoveAll();
        for (i = recordRoute.GetSize(); i > 0; i--)
          routeSet.AppendString(recordRoute[i-1]);
      }

      // If we are in a dialog or create one, then targetAddress needs to be set
      // to the contact field in the 2xx/1xx response for a target refresh 
      // request
      PString contact = response.GetMIME().GetContact();
      if (!contact.IsEmpty()) {
        targetAddress = contact;
        PTRACE(4, "SIP\tSet targetAddress to " << targetAddress);
      }
    }

    // Send the ack if not pending
    if (response.GetStatusCode()/100 != 1)
      SendACK(transaction, response);

    if (phase < ConnectedPhase) {
      // Final check to see if we have forked INVITEs still running, don't
      // release connection until all of them have failed.
      for (PSafePtr<SIPTransaction> invitation(forkedInvitations, PSafeReference); invitation != NULL; ++invitation) {
        if (invitation->IsInProgress()) {
          ignoreErrorResponse = TRUE;
          break;
        }
      }
    }
    else {
      // This INVITE is from a different "dialog", any errors do not cause a release
      ignoreErrorResponse = localPartyAddress != response.GetMIME().GetFrom() || remotePartyAddress != response.GetMIME().GetTo();
    }
  }

  // Break out to virtual functions for some special cases.
  switch (response.GetStatusCode()) {
    case SIP_PDU::Information_Trying :
      OnReceivedTrying(response);
      break;

    case SIP_PDU::Information_Ringing :
      OnReceivedRinging(response);
      break;

    case SIP_PDU::Information_Session_Progress :
      OnReceivedSessionProgress(response);
      break;

    case SIP_PDU::Failure_UnAuthorised :
    case SIP_PDU::Failure_ProxyAuthenticationRequired :
      if (OnReceivedAuthenticationRequired(transaction, response))
        return;
      break;

    case SIP_PDU::Redirection_MovedTemporarily :
      OnReceivedRedirection(response);
      break;

    default :
      break;
  }

  switch (response.GetStatusCode()/100) {
    case 1 : // Do nothing for Provisional responses
      return;

    case 2 : // Successful esponse - there really is only 200 OK
      OnReceivedOK(transaction, response);
      return;

    case 3 : // Redirection response
      return;
  }

  // If we are doing a local hold, and fail, we do not release the conneciton
  if (reInvite && local_hold) {
    local_hold = FALSE;       // It failed
    PauseMediaStreams(FALSE); // Un-Pause the media streams
    endpoint.OnHold(*this);   // Signal the manager that there is no more hold
    return;
  }

  // We don't always release the connection, eg not till all forked invites have completed
  if (ignoreErrorResponse)
    return;

  // All other responses are errors, see if they should cause a Release()
  for (i = 0; i < PARRAYSIZE(SIPCodeToReason); i++) {
    if (response.GetStatusCode() == SIPCodeToReason[i].code) {
      releaseMethod = ReleaseWithNothing;
      SetQ931Cause(SIPCodeToReason[i].q931Cause);
      Release(SIPCodeToReason[i].reason);
      break;
    }
  }
}


void SIPConnection::OnReceivedINVITE(SIP_PDU & request)
{
  BOOL isReinvite;

  const SIPMIMEInfo & requestMIME = request.GetMIME();
  PString requestTo = requestMIME.GetTo();
  PString requestFrom = requestMIME.GetFrom();

  if (IsOriginating()) {
    if (remotePartyAddress != requestFrom || localPartyAddress != requestTo) {
      PTRACE(2, "SIP\tIgnoring INVITE from " << request.GetURI() << " when originated call.");
      SIP_PDU response(request, SIP_PDU::Failure_LoopDetected);
      SendPDU(response, request.GetViaAddress(endpoint));
      return;
    }

    // We originated the call, so any INVITE must be a re-INVITE, 
    isReinvite = TRUE;
  }
  else if (originalInvite == NULL) {
    isReinvite = FALSE; // First time incoming call
    PTRACE(4, "SIP\tInitial INVITE from " << request.GetURI());
  }
  else {
    // Have received multiple INVITEs, three possibilities
    const SIPMIMEInfo & originalMIME = originalInvite->GetMIME();

    if (originalMIME.GetCSeq() == requestMIME.GetCSeq()) {
      // Same sequence number means it is a retransmission
      PTRACE(3, "SIP\tIgnoring duplicate INVITE from " << request.GetURI());
      return;
    }

    if (originalMIME.GetTo() != requestTo || originalMIME.GetFrom() != requestFrom) {
      // Different "dialog" determined by the tags in the to and from fields indicate forking
      PTRACE(3, "SIP\tIgnoring forked INVITE from " << request.GetURI());
      SIP_PDU response(request, SIP_PDU::Failure_LoopDetected);
      response.GetMIME().SetProductInfo(endpoint.GetUserAgent(), GetProductInfo());
      SendPDU(response, request.GetViaAddress(endpoint));
      return;
    }

    // A new INVITE in the same "dialog" but different cseq must be a re-INVITE
    isReinvite = TRUE;
  }
   

  // originalInvite should contain the first received INVITE for
  // this connection
  delete originalInvite;
  originalInvite = new SIP_PDU(request);

  if (request.HasSDP())
    remoteSDP = request.GetSDP();

  // We received a Re-INVITE for a current connection
  if (isReinvite) { 
    OnReceivedReINVITE(request);
    return;
  }
  
  releaseMethod = ReleaseWithResponse;

  // Fill in all the various connection info
  SIPMIMEInfo & mime = originalInvite->GetMIME();
  remotePartyAddress = mime.GetFrom(); 
  UpdateRemotePartyNameAndNumber();
  mime.GetProductInfo(remoteProductInfo);
  localPartyAddress  = mime.GetTo() + ";tag=" + OpalGloballyUniqueID().AsString(); // put a real random 
  mime.SetTo(localPartyAddress);

  // get the called destination
  calledDestinationName   = originalInvite->GetURI().GetDisplayName(FALSE);   
  calledDestinationNumber = originalInvite->GetURI().GetUserName();
  calledDestinationURL    = originalInvite->GetURI().AsString();

  // update the target address
  PString contact = mime.GetContact();
  if (!contact.IsEmpty()) 
    targetAddress = contact;
  targetAddress.AdjustForRequestURI();
  PTRACE(4, "SIP\tSet targetAddress to " << targetAddress);

  // get the address that remote end *thinks* it is using from the Contact field
  PIPSocket::Address sigAddr;
  PIPSocket::GetHostAddress(targetAddress.GetHostName(), sigAddr);  

  // get the local and peer transport addresses
  PIPSocket::Address peerAddr, localAddr;
  transport->GetRemoteAddress().GetIpAddress(peerAddr);
  transport->GetLocalAddress().GetIpAddress(localAddr);

  // allow the application to determine if RTP NAT is enabled or not
  remoteIsNAT = IsRTPNATEnabled(localAddr, peerAddr, sigAddr, TRUE);

  // indicate the other is to start ringing (but look out for clear calls)
  if (!OnIncomingConnection(0, NULL)) {
    PTRACE(1, "SIP\tOnIncomingConnection failed for INVITE from " << request.GetURI() << " for " << *this);
    Release();
    return;
  }

  PTRACE(3, "SIP\tOnIncomingConnection succeeded for INVITE from " << request.GetURI() << " for " << *this);
  SetPhase(SetUpPhase);

  if (!OnOpenIncomingMediaChannels()) {
    PTRACE(1, "SIP\tOnOpenIncomingMediaChannels failed for INVITE from " << request.GetURI() << " for " << *this);
    Release();
    return;
  }
}


void SIPConnection::OnReceivedReINVITE(SIP_PDU & request)
{
  if (phase != EstablishedPhase) {
    PTRACE(2, "SIP\tRe-INVITE from " << request.GetURI() << " received before initial INVITE completed on " << *this);
    SIP_PDU response(request, SIP_PDU::Failure_NotAcceptableHere);
    SendPDU(response, request.GetViaAddress(endpoint));
    return;
  }

  PTRACE(3, "SIP\tReceived re-INVITE from " << request.GetURI() << " for " << *this);

  // always send Trying for Re-INVITE
  SendInviteResponse(SIP_PDU::Information_Trying);

  remoteFormatList.RemoveAll();
  SDPSessionDescription sdpOut(GetLocalAddress());

  // get the remote media formats, if any
  if (originalInvite->HasSDP()) {

    SDPSessionDescription & sdpIn = originalInvite->GetSDP();
    // The Re-INVITE can be sent to change the RTP Session parameters,
    // the current codecs, or to put the call on hold
    if (sdpIn.GetDirection(OpalMediaFormat::DefaultAudioSessionID) == SDPMediaDescription::SendOnly &&
        sdpIn.GetDirection(OpalMediaFormat::DefaultVideoSessionID) == SDPMediaDescription::SendOnly) {

      PTRACE(3, "SIP\tRemote hold detected");
      remote_hold = TRUE;
      PauseMediaStreams(TRUE);
      endpoint.OnHold(*this);
    }
    else {

      // If we receive a consecutive reinvite without the SendOnly
      // parameter, then we are not on hold anymore
      if (remote_hold) {
        PTRACE(3, "SIP\tRemote retrieve from hold detected");
        remote_hold = FALSE;
        PauseMediaStreams(FALSE);
        endpoint.OnHold(*this);
      }
    }
  }
  else {
    if (remote_hold) {
      PTRACE(3, "SIP\tRemote retrieve from hold without SDP detected");
      remote_hold = FALSE;
      PauseMediaStreams(FALSE);
      endpoint.OnHold(*this);
    }
  }
  
  // If it is a RE-INVITE that doesn't correspond to a HOLD, then
  // Close all media streams, they will be reopened.
  if (!IsConnectionOnHold()) {
    RemoveMediaStreams();
    GetCall().RemoveMediaStreams();
    ReleaseSession(OpalMediaFormat::DefaultAudioSessionID, TRUE);
#if OPAL_VIDEO
    ReleaseSession(OpalMediaFormat::DefaultVideoSessionID, TRUE);
#endif
#if OPAL_T38FAX
    ReleaseSession(OpalMediaFormat::DefaultDataSessionID, TRUE);
#endif
  }

  BOOL sdpOK;

  if (originalInvite->HasSDP()) {

    // Try to send SDP media description for audio and video
    // Use |= to avoid McCarthy boolean || from not calling video/fax
    SDPSessionDescription & sdpIn = originalInvite->GetSDP();
    sdpOK  = OnSendSDPMediaDescription(sdpIn, SDPMediaDescription::Audio, OpalMediaFormat::DefaultAudioSessionID, sdpOut);
#if OPAL_VIDEO
    sdpOK |= OnSendSDPMediaDescription(sdpIn, SDPMediaDescription::Video, OpalMediaFormat::DefaultVideoSessionID, sdpOut);
#endif
#if OPAL_T38FAX
    sdpOK |= OnSendSDPMediaDescription(sdpIn, SDPMediaDescription::Image, OpalMediaFormat::DefaultDataSessionID,  sdpOut);
#endif
  }

  else {

    // construct offer as per RFC 3261, para 14.2
    // Use |= to avoid McCarthy boolean || from not calling video/fax
    SDPSessionDescription *sdp = &sdpOut;
    sdpOK  = BuildSDP(sdp, rtpSessions, OpalMediaFormat::DefaultAudioSessionID);
#if OPAL_VIDEO
    sdpOK |= BuildSDP(sdp, rtpSessions, OpalMediaFormat::DefaultVideoSessionID);
#endif
#if OPAL_T38FAX
    sdpOK |= BuildSDP(sdp, rtpSessions, OpalMediaFormat::DefaultDataSessionID);
#endif
  }

  // send the 200 OK response
  if (sdpOK) 
    SendInviteOK(sdpOut);
  else
    SendInviteResponse(SIP_PDU::Failure_NotAcceptableHere);
}


BOOL SIPConnection::OnOpenIncomingMediaChannels()
{
  ApplyStringOptions();

  // in some circumstances, the peer OpalConnection needs to see the newly arrived media formats
  // before it knows what what formats can support. 
  if (originalInvite != NULL) {

    OpalMediaFormatList previewFormats;

    SDPSessionDescription & sdp = originalInvite->GetSDP();
    static struct PreviewType {
      SDPMediaDescription::MediaType mediaType;
      unsigned sessionID;
    } previewTypes[] = 
    { 
      { SDPMediaDescription::Audio, OpalMediaFormat::DefaultAudioSessionID },
#if OPAL_VIDEO
      { SDPMediaDescription::Video, OpalMediaFormat::DefaultVideoSessionID },
#endif
#if OPAL_T38FAX
      { SDPMediaDescription::Image, OpalMediaFormat::DefaultDataSessionID },
#endif
    };
    PINDEX i;
    for (i = 0; i < (PINDEX) (sizeof(previewTypes)/sizeof(previewTypes[0])); ++i) {
      SDPMediaDescription * mediaDescription = sdp.GetMediaDescription(previewTypes[i].mediaType);
      if (mediaDescription != NULL) {
        previewFormats += mediaDescription->GetMediaFormats(previewTypes[i].sessionID);
        OpalTransportAddress mediaAddress = mediaDescription->GetTransportAddress();
        mediaTransportAddresses.SetAt(previewTypes[i].sessionID, new OpalTransportAddress(mediaAddress));
      }
    }

    if (previewFormats.GetSize() != 0) 
      ownerCall.GetOtherPartyConnection(*this)->PreviewPeerMediaFormats(previewFormats);
  }

  ownerCall.OnSetUp(*this);

  AnsweringCall(OnAnswerCall(remotePartyAddress));
  return TRUE;
}


void SIPConnection::AnsweringCall(AnswerCallResponse response)
{
  switch (phase) {
    case SetUpPhase:
    case AlertingPhase:
      switch (response) {
        case AnswerCallNow:
        case AnswerCallProgress:
          SetConnected();
          break;

        case AnswerCallDenied:
          PTRACE(2, "SIP\tApplication has declined to answer incoming call");
          Release(EndedByAnswerDenied);
          break;

        case AnswerCallPending:
          SetAlerting(GetLocalPartyName(), FALSE);
          break;

        case AnswerCallAlertWithMedia:
          SetAlerting(GetLocalPartyName(), TRUE);
          break;

        default:
          break;
      }
      break;

    // 
    // cannot answer call in any of the following phases
    //
    case ConnectedPhase:
    case UninitialisedPhase:
    case EstablishedPhase:
    case ReleasingPhase:
    case ReleasedPhase:
    default:
      break;
  }
}


void SIPConnection::OnReceivedACK(SIP_PDU & response)
{
  if (originalInvite == NULL) {
    PTRACE(2, "SIP\tACK from " << response.GetURI() << " received before INVITE!");
    return;
  }

  if (originalInvite->GetMIME().GetTo()   != response.GetMIME().GetTo() ||
      originalInvite->GetMIME().GetFrom() != response.GetMIME().GetFrom()) {
    PTRACE(3, "SIP\tACK received for forked INVITE from " << response.GetURI());
    return;
  }

  PTRACE(3, "SIP\tACK received: " << phase);

  ackTimer.Stop();
  
  OnReceivedSDP(response);

  // If we receive an ACK in established phase, perhaps it
  // is a re-INVITE
  if (phase == EstablishedPhase && !IsConnectionOnHold()) {
    OpalConnection::OnConnected ();
    StartMediaStreams();
  }
  
  releaseMethod = ReleaseWithBYE;
  if (phase != ConnectedPhase)  
    return;
  
  SetPhase(EstablishedPhase);
  OnEstablished();

  // HACK HACK HACK: this is a work-around for a deadlock that can occur
  // during incoming calls. What is needed is a proper resequencing that
  // avoids this problem
  // start all of the media threads for the connection
  StartMediaStreams();
}


void SIPConnection::OnReceivedOPTIONS(SIP_PDU & /*request*/)
{
  PTRACE(2, "SIP\tOPTIONS not yet supported");
}


void SIPConnection::OnReceivedNOTIFY(SIP_PDU & pdu)
{
  PCaselessString event, state;
  
  if (referTransaction == NULL){
    PTRACE(2, "SIP\tNOTIFY in a connection only supported for REFER requests");
    return;
  }
  
  event = pdu.GetMIME().GetEvent();
  
  // We could also compare the To and From tags
  if (pdu.GetMIME().GetCallID() != referTransaction->GetMIME().GetCallID()
      || event.Find("refer") == P_MAX_INDEX) {

    SIP_PDU response(pdu, SIP_PDU::Failure_BadEvent);
    SendPDU(response, pdu.GetViaAddress(endpoint));
    return;
  }

  state = pdu.GetMIME().GetSubscriptionState();
  
  SIP_PDU response(pdu, SIP_PDU::Successful_OK);
  SendPDU(response, pdu.GetViaAddress(endpoint));
  
  // The REFER is over
  if (state.Find("terminated") != P_MAX_INDEX) {
    referTransaction->WaitForCompletion();
    referTransaction = (SIPTransaction *)NULL;

    // Release the connection
    if (phase < ReleasingPhase) 
    {
      releaseMethod = ReleaseWithBYE;
      Release(OpalConnection::EndedByCallForwarded);
    }
  }

  // The REFER is not over yet, ignore the state of the REFER for now
}


void SIPConnection::OnReceivedREFER(SIP_PDU & pdu)
{
  SIPTransaction *notifyTransaction = NULL;
  PString referto = pdu.GetMIME().GetReferTo();
  
  if (referto.IsEmpty()) {
    SIP_PDU response(pdu, SIP_PDU::Failure_BadEvent);
    SendPDU(response, pdu.GetViaAddress(endpoint));
    return;
  }    

  SIP_PDU response(pdu, SIP_PDU::Successful_Accepted);
  SendPDU(response, pdu.GetViaAddress(endpoint));
  releaseMethod = ReleaseWithNothing;

  endpoint.SetupTransfer(GetToken(),  
                         PString (), 
                         referto,  
                         NULL);
  
  // Send a Final NOTIFY,
  notifyTransaction = new SIPReferNotify(*this, *transport, SIP_PDU::Successful_Accepted);
  notifyTransaction->WaitForCompletion();
}


void SIPConnection::OnReceivedBYE(SIP_PDU & request)
{
  PTRACE(3, "SIP\tBYE received for call " << request.GetMIME().GetCallID());
  SIP_PDU response(request, SIP_PDU::Successful_OK);
  SendPDU(response, request.GetViaAddress(endpoint));
  
  if (phase >= ReleasingPhase) {
    PTRACE(2, "SIP\tAlready released " << *this);
    return;
  }
  releaseMethod = ReleaseWithNothing;
  
  remotePartyAddress = request.GetMIME().GetFrom();
  UpdateRemotePartyNameAndNumber();
  response.GetMIME().GetProductInfo(remoteProductInfo);

  Release(EndedByRemoteUser);
}


void SIPConnection::OnReceivedCANCEL(SIP_PDU & request)
{
  PString origTo;

  // Currently only handle CANCEL requests for the original INVITE that
  // created this connection, all else ignored
  // Ignore the tag added by OPAL
  if (originalInvite != NULL) {
    origTo = originalInvite->GetMIME().GetTo();
    origTo.Delete(origTo.Find(";tag="), P_MAX_INDEX);
  }
  if (originalInvite == NULL || 
      request.GetMIME().GetTo() != origTo || 
      request.GetMIME().GetFrom() != originalInvite->GetMIME().GetFrom() || 
      request.GetMIME().GetCSeqIndex() != originalInvite->GetMIME().GetCSeqIndex()) {
    PTRACE(2, "SIP\tUnattached " << request << " received for " << *this);
    SIP_PDU response(request, SIP_PDU::Failure_TransactionDoesNotExist);
    SendPDU(response, request.GetViaAddress(endpoint));
    return;
  }

  PTRACE(3, "SIP\tCancel received for " << *this);

  SIP_PDU response(request, SIP_PDU::Successful_OK);
  SendPDU(response, request.GetViaAddress(endpoint));
  
  if (!IsOriginating())
    Release(EndedByCallerAbort);
}


void SIPConnection::OnReceivedTrying(SIP_PDU & /*response*/)
{
  PTRACE(3, "SIP\tReceived Trying response");
}


void SIPConnection::OnReceivedRinging(SIP_PDU & /*response*/)
{
  PTRACE(3, "SIP\tReceived Ringing response");

  if (phase < AlertingPhase)
  {
    SetPhase(AlertingPhase);
    OnAlerting();
  }
}


void SIPConnection::OnReceivedSessionProgress(SIP_PDU & response)
{
  PTRACE(3, "SIP\tReceived Session Progress response");

  OnReceivedSDP(response);

  if (phase < AlertingPhase)
  {
    SetPhase(AlertingPhase);
    OnAlerting();
  }

  PTRACE(4, "SIP\tStarting receive media to annunciate remote progress tones");
  OnConnected();                        // start media streams
}


void SIPConnection::OnReceivedRedirection(SIP_PDU & response)
{
  targetAddress = response.GetMIME().GetContact();
  remotePartyAddress = targetAddress.AsQuotedString();
  PINDEX j;
  if ((j = remotePartyAddress.Find (';')) != P_MAX_INDEX)
    remotePartyAddress = remotePartyAddress.Left(j);

  endpoint.ForwardConnection (*this, remotePartyAddress);
}


BOOL SIPConnection::OnReceivedAuthenticationRequired(SIPTransaction & transaction,
                                                     SIP_PDU & response)
{
  BOOL isProxy = response.GetStatusCode() == SIP_PDU::Failure_ProxyAuthenticationRequired;
  SIPURL proxy;
  SIPAuthentication auth;
  PString lastUsername;
  PString lastNonce;

#if PTRACING
  const char * proxyTrace = isProxy ? "Proxy " : "";
#endif
  
  if (transaction.GetMethod() != SIP_PDU::Method_INVITE) {
    PTRACE(1, "SIP\tCannot do " << proxyTrace << "Authentication Required for non INVITE");
    return FALSE;
  }

  PTRACE(3, "SIP\tReceived " << proxyTrace << "Authentication Required response");

  PCaselessString authenticateTag = isProxy ? "Proxy-Authenticate" : "WWW-Authenticate";

  // Received authentication required response, try to find authentication
  // for the given realm if no proxy
  if (!auth.Parse(response.GetMIME()(authenticateTag), isProxy)) {
    return FALSE;
  }

  // Save the username, realm and nonce
  lastUsername = auth.GetUsername();
  lastNonce = auth.GetNonce();

  // Try to find authentication parameters for the given realm,
  // if not, use the proxy authentication parameters (if any)
  if (!endpoint.GetAuthentication(auth.GetAuthRealm(), authentication)) {
    PTRACE (3, "SIP\tCouldn't find authentication information for realm " << auth.GetAuthRealm()
            << ", will use SIP Outbound Proxy authentication settings, if any");
    if (endpoint.GetProxy().IsEmpty())
      return FALSE;

    authentication.SetUsername(endpoint.GetProxy().GetUserName());
    authentication.SetPassword(endpoint.GetProxy().GetPassword());
  }

  if (!authentication.Parse(response.GetMIME()(authenticateTag), isProxy))
    return FALSE;
  
  if (!authentication.IsValid() || (lastUsername == authentication.GetUsername() && lastNonce == authentication.GetNonce())) {
    PTRACE(2, "SIP\tAlready done INVITE for " << proxyTrace << "Authentication Required");
    return FALSE;
  }

  // Restart the transaction with new authentication info
  // and start with a fresh To tag
  // Section 8.1.3.5 of RFC3261 tells that the authenticated
  // request SHOULD have the same value of the Call-ID, To and From.
  PINDEX j;
  if ((j = remotePartyAddress.Find (';')) != P_MAX_INDEX)
    remotePartyAddress = remotePartyAddress.Left(j);
  
  if (proxy.IsEmpty())
    proxy = endpoint.GetProxy();

  // Default routeSet if there is a proxy
  if (!proxy.IsEmpty() && routeSet.GetSize() == 0) 
    routeSet += "sip:" + proxy.GetHostName() + ':' + PString(proxy.GetPort()) + ";lr";

  RTP_SessionManager & origRtpSessions = ((SIPInvite &)transaction).GetSessionManager();
  SIPTransaction * invite = new SIPInvite(*this, *transport, origRtpSessions);
  if (invite->Start())
  {
    forkedInvitations.Append(invite);
    return TRUE;
  }

  PTRACE(2, "SIP\tCould not restart INVITE for " << proxyTrace << "Authentication Required");
  return FALSE;
}


void SIPConnection::OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response)
{
  if (transaction.GetMethod() != SIP_PDU::Method_INVITE) {
    PTRACE(2, "SIP\tReceived OK response for non INVITE");
    return;
  }

  PTRACE(3, "SIP\tReceived INVITE OK response");

  OnReceivedSDP(response);

  releaseMethod = ReleaseWithBYE;

  connectedTime = PTime ();
  OnConnected();                        // start media streams

  if (phase == EstablishedPhase)
    return;

  SetPhase(EstablishedPhase);
  OnEstablished();
}


void SIPConnection::OnReceivedSDP(SIP_PDU & pdu)
{
  if (!pdu.HasSDP())
    return;

  remoteSDP = pdu.GetSDP();
  OnReceivedSDPMediaDescription(remoteSDP, SDPMediaDescription::Audio, OpalMediaFormat::DefaultAudioSessionID);

  remoteFormatList += OpalRFC2833;

#if OPAL_VIDEO
  OnReceivedSDPMediaDescription(remoteSDP, SDPMediaDescription::Video, OpalMediaFormat::DefaultVideoSessionID);
#endif

#if OPAL_T38FAX
  OnReceivedSDPMediaDescription(remoteSDP, SDPMediaDescription::Image, OpalMediaFormat::DefaultDataSessionID);
#endif
}


BOOL SIPConnection::OnReceivedSDPMediaDescription(SDPSessionDescription & sdp,
                                                  SDPMediaDescription::MediaType mediaType,
                                                  unsigned rtpSessionId)
{
  RTP_UDP *rtpSession = NULL;
  SDPMediaDescription * mediaDescription = sdp.GetMediaDescription(mediaType);
  
  if (mediaDescription == NULL) {
    PTRACE(mediaType <= SDPMediaDescription::Video ? 2 : 3, "SIP\tCould not find SDP media description for " << mediaType);
    return FALSE;
  }

  {
    // see if the remote supports this media
    OpalMediaFormatList mediaFormatList = mediaDescription->GetMediaFormats(rtpSessionId);
    if (mediaFormatList.GetSize() == 0) {
      PTRACE(1, "SIP\tCould not find media formats in SDP media description for session " << rtpSessionId);
      return FALSE;
    }

    // create the RTPSession
    OpalTransportAddress localAddress;
    OpalTransportAddress address = mediaDescription->GetTransportAddress();
    rtpSession = OnUseRTPSession(rtpSessionId, address, localAddress);
    if (rtpSession == NULL && !ownerCall.IsMediaBypassPossible(*this, rtpSessionId)) {
      if (rtpSessionId == OpalMediaFormat::DefaultAudioSessionID) 
        Release(EndedByTransportFail);
      return FALSE;
    }

    // set the remote address 
    PIPSocket::Address ip;
    WORD port;
    if (!address.GetIpAndPort(ip, port) || (rtpSession && !rtpSession->SetRemoteSocketInfo(ip, port, TRUE))) {
      PTRACE(1, "SIP\tCannot set remote ports on RTP session");
      if (rtpSessionId == OpalMediaFormat::DefaultAudioSessionID) 
        Release(EndedByTransportFail);
      return FALSE;
    }

    // When receiving an answer SDP, keep the remote SDP media formats order
    // but remove the media formats we do not support.
    remoteFormatList += mediaFormatList;
    remoteFormatList.Remove(endpoint.GetManager().GetMediaFormatMask());

    // create map for RTP payloads
    mediaDescription->CreateRTPMap(rtpSessionId, rtpPayloadMap);
    
    // Open the streams and the reverse streams
    if (!OnOpenSourceMediaStreams(remoteFormatList, rtpSessionId, NULL))
      return FALSE;
  }

  return TRUE;
}


void SIPConnection::OnCreatingINVITE(SIP_PDU & /*request*/)
{
  PTRACE(3, "SIP\tCreating INVITE request");
}


void SIPConnection::QueuePDU(SIP_PDU * pdu)
{
  if (PAssertNULL(pdu) == NULL)
    return;

  if (phase >= ReleasedPhase) {
    if(pdu->GetMethod() != SIP_PDU::NumMethods)
    {
      PTRACE(4, "SIP\tIgnoring PDU: " << *pdu);
    }
    else
    {
      PTRACE(4, "SIP\tProcessing PDU: " << *pdu);
      OnReceivedPDU(*pdu);
    }
    delete pdu;
  }
  else {
    PTRACE(4, "SIP\tQueueing PDU: " << *pdu);
    pduQueue.Enqueue(pdu);
    pduSemaphore.Signal();

    if (pduHandler == NULL) {
      SafeReference();
      pduHandler = PThread::Create(PCREATE_NOTIFIER(HandlePDUsThreadMain), 0,
                                   PThread::NoAutoDeleteThread,
                                   PThread::NormalPriority,
                                   "SIP Handler:%x");
    }
  }
}


void SIPConnection::HandlePDUsThreadMain(PThread &, INT)
{
  PTRACE(4, "SIP\tPDU handler thread started.");

  while (phase != ReleasedPhase) {
    PTRACE(4, "SIP\tAwaiting next PDU.");
    pduSemaphore.Wait();

    if (!LockReadWrite())
      break;

    SIP_PDU * pdu = pduQueue.Dequeue();

    UnlockReadWrite();

    if (pdu != NULL) {
      OnReceivedPDU(*pdu);
      delete pdu;
    }
  }

  SafeDereference();

  PTRACE(4, "SIP\tPDU handler thread finished.");
}


void SIPConnection::OnAckTimeout(PThread &, INT)
{
  releaseMethod = ReleaseWithBYE;
  Release(EndedByTemporaryFailure);
}


BOOL SIPConnection::ForwardCall (const PString & fwdParty)
{
  if (fwdParty.IsEmpty ())
    return FALSE;
  
  forwardParty = fwdParty;
  PTRACE(2, "SIP\tIncoming SIP connection will be forwarded to " << forwardParty);
  Release(EndedByCallForwarded);

  return TRUE;
}


BOOL SIPConnection::SendInviteOK(const SDPSessionDescription & sdp)
{
  SIPURL localPartyURL(GetLocalPartyAddress());
  PString userName = endpoint.GetRegisteredPartyName(localPartyURL).GetUserName();
  SIPURL contact = endpoint.GetContactURL(*transport, userName, localPartyURL.GetHostName());

  return SendInviteResponse(SIP_PDU::Successful_OK, (const char *) contact.AsQuotedString(), NULL, &sdp);
}

BOOL SIPConnection::SendACK(SIPTransaction & invite, SIP_PDU & response)
{
  if (invite.GetMethod() != SIP_PDU::Method_INVITE) { // Sanity check
    return FALSE;
  }

  SIP_PDU ack;
  // ACK Constructed following 17.1.1.3
  if (response.GetStatusCode()/100 != 2) {
    ack = SIPAck(endpoint, invite, response); 
  } else { 
    ack = SIPAck(invite);
  }

  // Send the PDU using the connection transport
  if (!SendPDU(ack, ack.GetSendAddress(ack.GetMIME().GetRoute()))) {
    Release(EndedByTransportFail);
    return FALSE;
  }

  return TRUE;
}

BOOL SIPConnection::SendInviteResponse(SIP_PDU::StatusCodes code, const char * contact, const char * extra, const SDPSessionDescription * sdp)
{
  if (NULL == originalInvite)
    return FALSE;

  SIP_PDU response(*originalInvite, code, contact, extra);
  if (NULL != sdp)
    response.SetSDP(*sdp);
  response.GetMIME().SetProductInfo(endpoint.GetUserAgent(), GetProductInfo());

  if (response.GetStatusCode()/100 != 1)
    ackTimer = endpoint.GetAckTimeout();

  return SendPDU(response, originalInvite->GetViaAddress(endpoint)); 
}


BOOL SIPConnection::SendPDU(SIP_PDU & pdu, const OpalTransportAddress & address)
{
  PWaitAndSignal m(transportMutex); 
  return transport != NULL && pdu.Write(*transport, address);
}

void SIPConnection::OnRTPStatistics(const RTP_Session & session) const
{
  endpoint.OnRTPStatistics(*this, session);
}

void SIPConnection::OnReceivedINFO(SIP_PDU & pdu)
{
  SIP_PDU::StatusCodes status = SIP_PDU::Failure_UnsupportedMediaType;
  SIPMIMEInfo & mimeInfo = pdu.GetMIME();
  PString contentType = mimeInfo.GetContentType();

  if (contentType *= ApplicationDTMFRelayKey) {
    PStringArray lines = pdu.GetEntityBody().Lines();
    PINDEX i;
    char tone = -1;
    int duration = -1;
    for (i = 0; i < lines.GetSize(); ++i) {
      PStringArray tokens = lines[i].Tokenise('=', FALSE);
      PString val;
      if (tokens.GetSize() > 1)
        val = tokens[1].Trim();
      if (tokens.GetSize() > 0) {
        if (tokens[0] *= "signal")
          tone = val[0];
        else if (tokens[0] *= "duration")
          duration = val.AsInteger();
      }
    }
    if (tone != -1)
      OnUserInputTone(tone, duration == 0 ? 100 : tone);
    status = SIP_PDU::Successful_OK;
  }

  else if (contentType *= ApplicationDTMFKey) {
    OnUserInputString(pdu.GetEntityBody().Trim());
    status = SIP_PDU::Successful_OK;
  }

#if OPAL_VIDEO
  else if (contentType *= ApplicationMediaControlXMLKey) {
    if (OnMediaControlXML(pdu))
      return;
    status = SIP_PDU::Failure_UnsupportedMediaType;
  }
#endif

  else 
    status = SIP_PDU::Failure_UnsupportedMediaType;

  SIP_PDU response(pdu, status);
  SendPDU(response, pdu.GetViaAddress(endpoint));
}


void SIPConnection::OnReceivedPING(SIP_PDU & pdu)
{
  PTRACE(3, "SIP\tReceived PING");
  SIP_PDU response(pdu, SIP_PDU::Successful_OK);
  SendPDU(response, pdu.GetViaAddress(endpoint));
}


OpalConnection::SendUserInputModes SIPConnection::GetRealSendUserInputMode() const
{
  switch (sendUserInputMode) {
    case SendUserInputAsString:
    case SendUserInputAsTone:
      return sendUserInputMode;
    default:
      break;
  }

  return SendUserInputAsInlineRFC2833;
}

BOOL SIPConnection::SendUserInputTone(char tone, unsigned duration)
{
  SendUserInputModes mode = GetRealSendUserInputMode();

  PTRACE(3, "SIP\tSendUserInputTone('" << tone << "', " << duration << "), using mode " << mode);

  switch (mode) {
    case SendUserInputAsTone:
    case SendUserInputAsString:
      {
        PSafePtr<SIPTransaction> infoTransaction = new SIPTransaction(*this, *transport, SIP_PDU::Method_INFO);
        SIPMIMEInfo & mimeInfo = infoTransaction->GetMIME();
        PStringStream str;
        if (mode == SendUserInputAsTone) {
          mimeInfo.SetContentType(ApplicationDTMFRelayKey);
          str << "Signal=" << tone << "\r\n" << "Duration=" << duration << "\r\n";
        }
        else {
          mimeInfo.SetContentType("application/dtmf-relay");
          mimeInfo.SetContentType("application/dtmf");
          str << tone;
        }
        infoTransaction->GetEntityBody() = str;
        infoTransaction->WaitForCompletion();
        return !infoTransaction->IsFailed();
      }

    // anything else - send as RFC 2833
    case SendUserInputAsProtocolDefault:
    default:
      break;
  }

  return OpalConnection::SendUserInputTone(tone, duration);
}

#if OPAL_VIDEO
class QDXML 
{
  public:
    struct statedef {
      int currState;
      const char * str;
      int newState;
    };

    virtual ~QDXML() {}

    bool ExtractNextElement(std::string & str)
    {
      while (isspace(*ptr))
        ++ptr;
      if (*ptr != '<')
        return false;
      ++ptr;
      if (*ptr == '\0')
        return false;
      const char * start = ptr;
      while (*ptr != '>') {
        if (*ptr == '\0')
          return false;
        ++ptr;
      }
      ++ptr;
      str = std::string(start, ptr-start-1);
      return true;
    }

    int Parse(const std::string & xml, const statedef * states, unsigned numStates)
    {
      ptr = xml.c_str(); 
      state = 0;
      std::string str;
      while ((state >= 0) && ExtractNextElement(str)) {
        cout << state << "  " << str << endl;
        unsigned i;
        for (i = 0; i < numStates; ++i) {
          cout << "comparing '" << str << "' to '" << states[i].str << "'" << endl;
          if ((state == states[i].currState) && (str.compare(0, strlen(states[i].str), states[i].str) == 0)) {
            state = states[i].newState;
            break;
          }
        }
        if (i == numStates) {
          cout << "unknown string " << str << " in state " << state << endl;
          state = -1;
          break;
        }
        if (!OnMatch(str)) {
          state = -1;
          break; 
        }
      }
      return state;
    }

    virtual bool OnMatch(const std::string & )
    { return true; }

  protected:
    int state;
    const char * ptr;
};

class VFUXML : public QDXML
{
  public:
    bool vfu;

    VFUXML()
    { vfu = false; }

    BOOL Parse(const std::string & xml)
    {
      static const struct statedef states[] = {
        { 0, "?xml",                1 },
        { 1, "media_control",       2 },
        { 2, "vc_primitive",        3 },
        { 3, "to_encoder",          4 },
        { 4, "picture_fast_update", 5 },
        { 5, "/to_encoder",         6 },
        { 6, "/vc_primitive",       7 },
        { 7, "/media_control",      255 },
      };
      const int numStates = sizeof(states)/sizeof(states[0]);
      return QDXML::Parse(xml, states, numStates) == 255;
    }

    bool OnMatch(const std::string &)
    {
      if (state == 5)
        vfu = true;
      return true;
    }
};

BOOL SIPConnection::OnMediaControlXML(SIP_PDU & pdu)
{
  VFUXML vfu;
  if (!vfu.Parse(pdu.GetEntityBody())) {
    SIP_PDU response(pdu, SIP_PDU::Failure_Undecipherable);
    response.GetEntityBody() = 
      "<?xml version=\"1.0\" encoding=\"utf-8\" ?>\n"
      "<media_control>\n"
      "  <general_error>\n"
      "  Unable to parse XML request\n"
      "   </general_error>\n"
      "</media_control>\n";
    SendPDU(response, pdu.GetViaAddress(endpoint));
  }
  else if (vfu.vfu) {
    if (!LockReadWrite())
      return FALSE;

    OpalMediaStream * encodingStream = GetMediaStream(OpalMediaFormat::DefaultVideoSessionID, TRUE);

    if (!encodingStream){
      OpalVideoUpdatePicture updatePictureCommand;
      encodingStream->ExecuteCommand(updatePictureCommand);
    }

    SIP_PDU response(pdu, SIP_PDU::Successful_OK);
    SendPDU(response, pdu.GetViaAddress(endpoint));
    
    UnlockReadWrite();
  }

  return TRUE;
}
#endif


PString SIPConnection::GetExplicitFrom() const
{
  if (!explicitFrom.IsEmpty())
    return explicitFrom;
  return GetLocalPartyAddress();
}

/////////////////////////////////////////////////////////////////////////////

SIP_RTP_Session::SIP_RTP_Session(const SIPConnection & conn)
  : connection(conn)
{
}


void SIP_RTP_Session::OnTxStatistics(const RTP_Session & session) const
{
  connection.OnRTPStatistics(session);
}


void SIP_RTP_Session::OnRxStatistics(const RTP_Session & session) const
{
  connection.OnRTPStatistics(session);
}

#if OPAL_VIDEO
void SIP_RTP_Session::OnRxIntraFrameRequest(const RTP_Session & session) const
{
  // We got an intra frame request control packet, alert the encoder.
  // We're going to grab the call, find the other connection, then grab the
  // encoding stream
  PSafePtr<OpalConnection> otherConnection = connection.GetCall().GetOtherPartyConnection(connection);
  if (otherConnection == NULL)
    return; // No other connection.  Bail.

  // Found the encoding stream, send an OpalVideoFastUpdatePicture
  OpalMediaStream * encodingStream = otherConnection->GetMediaStream(session.GetSessionID(), TRUE);
  if (encodingStream) {
    OpalVideoUpdatePicture updatePictureCommand;
    encodingStream->ExecuteCommand(updatePictureCommand);
  }
}

void SIP_RTP_Session::OnTxIntraFrameRequest(const RTP_Session & /*session*/) const
{
}
#endif

// End of file ////////////////////////////////////////////////////////////////
