/*
 * gkserver.cxx
 *
 * Gatekeeper client protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1999-2000 Equivalence Pty. Ltd.
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
 * This code was based on original code from OpenGate of Egoboo Ltd. thanks
 * to Ashley Unitt for his efforts.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: gkserver.cxx,v $
 * Revision 1.2012  2003/01/07 04:39:53  robertj
 * Updated to OpenH323 v1.11.2
 *
 * Revision 2.10  2002/11/10 11:33:18  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.9  2002/09/13 07:13:14  robertj
 * Fixed numerous incorrect transport address protocol types.
 *
 * Revision 2.8  2002/09/04 06:01:48  robertj
 * Updated to OpenH323 v1.9.6
 *
 * Revision 2.7  2002/07/01 04:56:31  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 2.6  2002/03/22 06:57:49  robertj
 * Updated to OpenH323 version 1.8.2
 *
 * Revision 2.5  2002/02/11 09:32:12  robertj
 * Updated to openH323 v1.8.0
 *
 * Revision 2.4  2002/01/14 06:35:57  robertj
 * Updated to OpenH323 v1.7.9
 *
 * Revision 2.3  2001/11/13 04:29:47  robertj
 * Changed OpalTransportAddress CreateTransport and CreateListsner functions
 *   to have extra parameter to control local binding of sockets.
 *
 * Revision 2.2  2001/10/05 00:22:13  robertj
 * Updated to PWLib 1.2.0 and OpenH323 1.7.0
 *
 * Revision 2.1  2001/08/13 05:10:39  robertj
 * Updates from OpenH323 v1.6.0 release.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.109  2002/12/19 06:20:56  robertj
 * Rearranged crypto check to be at the end of the H323RegisteredEndPoint
 *   version of OnAdmission() to make it easier for overrides to do something
 *   with teh endpoint if it fails the security,
 *
 * Revision 1.108  2002/12/19 01:01:24  robertj
 * Fixed missing read/write lock when doing IRR update on call state.
 *
 * Revision 1.107  2002/12/17 01:25:26  robertj
 * Added call backs on gk server when ep indicates alerting or connect.
 *
 * Revision 1.106  2002/12/13 03:59:32  robertj
 * Fixed incorrect field identifier used in including optional field for
 *   balance in ARJ
 *
 * Revision 1.105  2002/12/12 03:50:42  robertj
 * Fixed incorrect condition for detecting suportset RRQ
 *
 * Revision 1.104  2002/12/02 07:42:35  robertj
 * Fixed GNU warning
 *
 * Revision 1.103  2002/12/02 06:44:21  robertj
 * Changed to allow for an already registered endpoint to send a full RRQ to add
 *   but not subtract aliases or signalling addresses. If tries to subtract then
 *   will reject the RRQ and de-register the endpoint.
 *
 * Revision 1.102  2002/11/28 05:46:20  robertj
 * Fixed bug so can set total bandwidth while calls are in progress.
 *
 * Revision 1.101  2002/11/28 04:41:48  robertj
 * Added support for RAS ServiceControlIndication command.
 *
 * Revision 1.100  2002/11/27 06:54:56  robertj
 * Added Service Control Session management as per Annex K/H.323 via RAS
 *   only at this stage.
 * Added H.248 ASN and very primitive infrastructure for linking into the
 *   Service Control Session management system.
 * Added basic infrastructure for Annex K/H.323 HTTP transport system.
 * Added Call Credit Service Control to display account balances.
 *
 * Revision 1.99  2002/11/25 03:27:28  robertj
 * Added sanity checking to the alerting, connected and end call times may
 *   receive from endpoint, don't accept if in future or before call started!
 *
 * Revision 1.98  2002/11/21 06:40:00  robertj
 * Changed promiscuous mode to be three way. Fixes race condition in gkserver
 *   which can cause crashes or more PDUs to be sent to the wrong place.
 *
 * Revision 1.97  2002/11/19 01:50:35  robertj
 * Added expiditing of alerting and connect time statistics if ep can do UUIEs
 *
 * Revision 1.96  2002/11/18 23:41:25  robertj
 * Changed to use the H323EndPoint::CreateAuthenticators() function, we
 *   should use only one.
 *
 * Revision 1.95  2002/11/15 03:26:40  robertj
 * Fixed crash when removing alias in URQ and it isn't registered with it.
 *
 * Revision 1.94  2002/11/12 11:36:07  robertj
 * Added function to find endpoint by partial alias.
 * Optimised endpoint "indexes" to use sorted lists instead of dictionaries.
 *
 * Revision 1.93  2002/11/11 07:20:12  robertj
 * Minor clean up of API for doing RAS requests suing authentication.
 *
 * Revision 1.92  2002/11/10 08:10:43  robertj
 * Moved constants for "well known" ports to better place (OPAL change).
 *
 * Revision 1.91  2002/11/05 05:56:09  robertj
 * Super slight optimisation!
 *
 * Revision 1.90  2002/11/04 23:48:48  robertj
 * Fixed memory leak on ep unregistration, pointed out by Ravelli Rossano.
 *
 * Revision 1.89  2002/10/29 00:14:08  robertj
 * Improved setting of usage info, included getting it from BRQ etc.
 *
 * Revision 1.88  2002/10/24 07:04:41  robertj
 * Changed use of translate address so can see previous dstHost value before
 *   it might change it.
 *
 * Revision 1.87  2002/10/24 05:54:18  robertj
 * Improved looking up alias as DNS host and port.
 *
 * Revision 1.86  2002/10/21 01:20:36  robertj
 * Moved setting of aliases in RCF to after OnFullRegistration() returns so an
 *   application can alter the aliases an endpoint registers as.
 *
 * Revision 1.85  2002/10/21 00:55:46  robertj
 * Added function to unregister an endpoint via user interface.
 * Added reason code to call disengage.
 * Added ability to find call by a description string.
 * Fixed LCF having RAS address in it.
 *
 * Revision 1.84  2002/10/17 05:28:11  robertj
 * Supported some dumb old version endpoints that could not ignore fields
 *   that they don't understand.
 *
 * Revision 1.83  2002/10/17 02:20:56  robertj
 * Fixed problem with older clients sending unsolicited IRR's using the
 *   seqnum==1 technique instead of explicit flag.
 *
 * Revision 1.82  2002/10/16 07:29:41  robertj
 * Added support for endpoints that do not support RRQ timeToLive parameter,
 *   will actively go see if ep is there using IRQ before expiring the ep.
 *
 * Revision 1.81  2002/10/16 01:53:09  robertj
 * Removed trace on starting slow PDU handler thread, cannot use pointer
 *   returned if is auto-delete thread as it can go out of scope any time.
 *
 * Revision 1.80  2002/10/16 01:00:02  robertj
 * Fixed gk server so can listen on localhost if listening on INADDR_ANY.
 *
 * Revision 1.79  2002/10/10 05:32:36  robertj
 * Minor changes for Tornado 2 compiler compatibility, thanks Martijn Roest
 *
 * Revision 1.78  2002/10/09 08:01:01  robertj
 * Changed registration so get URJ if calls are in progress.
 * Fixed call of registered eps OnRegistration() to before ep is removed.
 *
 * Revision 1.77  2002/10/02 09:33:24  robertj
 * Previous fix had to be done in two places.
 *
 * Revision 1.76  2002/10/01 08:27:52  robertj
 * Fixed (benign) race condition between client and server both wanting to do
 *   a DRQ of a call at the same time. Caused an Assert, but no crash.
 *
 * Revision 1.75  2002/10/01 06:40:52  robertj
 * Removed GNU compiler warning
 *
 * Revision 1.74  2002/10/01 02:44:58  robertj
 * Fixed problem where a non lightweight RRQ with endpointIdentifer does not
 *   overwrite old H323RegisteredEndpoint instance, but just updates it
 *   preventing loss of calls if ep is just changing things like aliases etc.
 *
 * Revision 1.73  2002/09/29 23:39:30  craigs
 * Rearranged expression to avoid MSVC compiler compiler internal error
 *
 * Revision 1.72  2002/09/25 14:33:05  robertj
 * Added special code for innovaphone's broken behaviour with unsolicited IRR's
 *   having no perCallInfo. For them this apparently means "all calls ok".
 *
 * Revision 1.71  2002/09/24 08:03:00  robertj
 * Added H.225 RAS protocol version number to registered endpoint structure.
 *
 * Revision 1.70  2002/09/20 05:39:22  robertj
 * Added Cisco non-standard version of connect time indication in IRR.
 *
 * Revision 1.69  2002/09/20 02:40:08  robertj
 * Fixed compatibility with buggy Cisco's that do not indicate the direction
 *   of the call in the IRR perCallInfo structure.
 *
 * Revision 1.68  2002/09/19 09:13:27  robertj
 * Fixed problem with making (and assuring with multi-threading) IRQ and DRQ
 *   requests are sent to the correct endpoint address, thanks Martijn Roest.
 *
 * Revision 1.67  2002/08/29 07:57:09  robertj
 * Added some statistics to gatekeeper server.
 *
 * Revision 1.66  2002/08/29 06:57:26  robertj
 * Removed redundent thread member variable from request info.
 * Added optimisation in does not need to look up endpoint on slow response
 *   sub-thread if already have looked it up duing the first pass.
 * Added optimisation and possible thread mutex issue in setting authenticator
 *   remote and local user names in RRQ, only do on full reg not keepAlive.
 *
 * Revision 1.65  2002/08/16 02:43:18  robertj
 * Fixed bug where call can exist in database even though was rejected.
 *
 * Revision 1.64  2002/08/16 02:32:58  robertj
 * Fixed possible deadlock in heartbeat
 *
 * Revision 1.63  2002/08/16 02:30:25  robertj
 * Fixed call heartbeat so if IRQ succeeds but indicates it does not know the
 *   call then the call in the gk is removed.
 *
 * Revision 1.62  2002/08/14 02:04:00  robertj
 * Added trace log for heartbeat timeout on call.
 *
 * Revision 1.61  2002/08/12 08:12:45  robertj
 * Added extra hint to help with ARQ using separate credentials from RRQ.
 *
 * Revision 1.60  2002/08/12 05:38:24  robertj
 * Changes to the RAS subsystem to support ability to make requests to client
 *   from gkserver without causing bottlenecks and race conditions.
 *
 * Revision 1.59  2002/08/11 23:20:42  robertj
 * Fixed GNU compatibility error
 *
 * Revision 1.58  2002/08/05 10:03:47  robertj
 * Cosmetic changes to normalise the usage of pragma interface/implementation.
 *
 * Revision 1.57  2002/08/05 05:17:41  robertj
 * Fairly major modifications to support different authentication credentials
 *   in ARQ to the logged in ones on RRQ. For both client and server.
 * Various other H.235 authentication bugs and anomalies fixed on the way.
 *
 * Revision 1.56  2002/07/25 05:22:22  robertj
 * Fixed possible race condition where call is removed between first ARQ
 *   processing and slow PDU handler thread starting!
 *
 * Revision 1.55  2002/07/24 02:57:44  robertj
 * Assured all calls for an ep are removed if ep is removed due to timeout
 *   rather than via an unregister.
 * Added tolerance to lightweight RRQ timer.
 * Added several missing mutex calls.
 *
 * Revision 1.54  2002/07/19 00:31:13  robertj
 * Allowed a zero IRR rate time to disable IRR hearbeat checks.
 *
 * Revision 1.53  2002/07/17 04:42:14  robertj
 * Fixed filling in callSignallingAddress in RCF
 *
 * Revision 1.52  2002/07/17 04:01:00  robertj
 * Fixed RRJ reason of duplicateAlias having correct choice field in PDU.
 *
 * Revision 1.51  2002/07/16 13:47:56  robertj
 * Added missing lock when removing call from endpoint.
 * Fixed setting of credentials when sending Disengage.
 * improved some logging messages.
 *
 * Revision 1.50  2002/07/16 09:20:56  robertj
 * Fixed bug where call was not removed from gk when Disengage called.
 * Fixed correct setting of unsolicited flag if get IRR from old v2 ep.
 *
 * Revision 1.49  2002/07/11 09:37:00  robertj
 * Added support for early versions of unsolicited IRR.
 * Fixed fail safe IRQ if does not receive unsolicited IRR in time.
 * Fixed removal of call if IRR failure indicates call is no longer valid.
 *
 * Revision 1.48  2002/07/11 07:01:41  robertj
 * Added Disengage() function to force call drop from gk server.
 * Added InfoRequest() function to force client to send an IRR.
 * Added ability to automatically clear calls if do not get IRR for it.
 *
 * Revision 1.47  2002/07/10 01:09:21  robertj
 * Fixed bug where did not use URQ endpoint id for look up.
 *
 * Revision 1.46  2002/07/10 00:05:47  robertj
 * Fixed bug where doing a URQ does not actually remove the endpoint.
 *
 * Revision 1.45  2002/07/09 00:16:26  robertj
 * Fixed incorrect allocation of bandwidth in ARQ, thanks Dave Giffin
 *
 * Revision 1.44  2002/06/24 07:01:05  robertj
 * Fixed correct adding of call to endpoint structure, only if call confirmed.
 *
 * Revision 1.43  2002/06/21 02:52:47  robertj
 * Fixed problem with double checking H.235 hashing, this causes failure as
 *   the authenticator thinks it is a replay attack.
 *
 * Revision 1.42  2002/06/19 05:03:11  robertj
 * Changed gk code to allow for H.235 security on an endpoint by endpoint basis.
 *
 * Revision 1.41  2002/06/12 03:55:21  robertj
 * Added function to add/remove multiple listeners in one go comparing against
 *   what is already running so does not interrupt unchanged listeners.
 *
 * Revision 1.40  2002/06/07 08:12:38  robertj
 * Added check to reject ARQ if does not contain a call identifier.
 *
 * Revision 1.39  2002/06/07 06:19:21  robertj
 * Fixed GNU warning
 *
 * Revision 1.38  2002/05/29 00:03:19  robertj
 * Fixed unsolicited IRR support in gk client and server,
 *   including support for IACK and INAK.
 *
 * Revision 1.37  2002/05/17 03:41:46  robertj
 * Fixed problems with H.235 authentication on RAS for server and client.
 *
 * Revision 1.36  2002/05/09 02:53:20  robertj
 * Fixed some problems with setting host/alias field in call on ARQ
 *
 * Revision 1.35  2002/05/08 02:06:07  robertj
 * Fixed failure to remove call if is rejected on slow reply.
 * Fixed overwriting of application infor on lightweight RRQ.
 * Fixed inclusion of adjusted aliases in RCF.
 *
 * Revision 1.34  2002/05/07 06:25:20  robertj
 * Fixed GNU compiler compatibility.
 *
 * Revision 1.33  2002/05/07 06:15:54  robertj
 * Changed ARQ processing so rejects multiple ARQ with same call id, not this
 *   does NOT include the transport level retries (based on sequence number).
 *
 * Revision 1.32  2002/05/07 03:18:15  robertj
 * Added application info (name/version etc) into registered endpoint data.
 *
 * Revision 1.31  2002/05/06 09:05:01  craigs
 * Fixed compile problem on Linux
 *
 * Revision 1.30  2002/05/06 00:56:41  robertj
 * Sizeable rewrite of gatekeeper server code to make more bulletproof against
 *   multithreaded operation. Especially when using slow response/RIP feature.
 * Also changed the call indexing to use call id and direction as key.
 *
 * Revision 1.29  2002/04/09 02:34:14  robertj
 * Fixed problem with duplicate entries in endpoint list if ARQ gets retried,
 *   thanks Horacio J. Peña
 *
 * Revision 1.28  2002/03/06 02:51:31  robertj
 * Fixed race condition when starting slow server response thread.
 *
 * Revision 1.27  2002/03/06 02:01:26  craigs
 * Add log message when SlowHandler thread started
 *
 * Revision 1.26  2002/03/05 03:42:40  robertj
 * Fixed calling of endpoints OnUnregister() function, thanks Francisco Olarte Sanz
 *
 * Revision 1.25  2002/03/04 07:57:57  craigs
 * Changed log level of ageing message
 * Fixed assert caused by accessing byIdentifier with integer
 *
 * Revision 1.24  2002/03/03 21:34:54  robertj
 * Added gatekeeper monitor thread.
 *
 * Revision 1.23  2002/03/02 05:59:01  robertj
 * Fixed possible bandwidth leak (thanks Francisco Olarte Sanz) and in
 *   the process added OnBandwidth function to H323GatekeeperCall class.
 *
 * Revision 1.22  2002/03/01 04:09:35  robertj
 * Fixed problems with keeping track of calls. Calls are now indexed by call-id
 *   alone and maintain both endpoints of call in its structure. Fixes problem
 *   with calls form an endpoint to itself, and having two objects being tracked
 *   where there is really only one call.
 *
 * Revision 1.21  2002/02/25 10:26:39  robertj
 * Added ARQ with zero bandwidth as a request for a default, thanks Federico Pinna
 *
 * Revision 1.20  2002/02/25 10:16:21  robertj
 * Fixed bad mistake in getting new aliases from ARQ, thanks HoraPe
 *
 * Revision 1.19  2002/02/25 09:49:02  robertj
 * Fixed possible "bandwidth leak" on ARQ, thanks Francisco Olarte Sanz
 *
 * Revision 1.18  2002/02/11 05:19:21  robertj
 * Allowed for multiple ARQ requests for same call ID. Reuses existing call
 *   object and sends another ACF.
 *
 * Revision 1.17  2002/02/04 05:21:27  robertj
 * Lots of changes to fix multithreaded slow response code (RIP).
 * Fixed problem with having two entries for same call in call list.
 *
 * Revision 1.16  2002/01/31 06:45:44  robertj
 * Added more checking for invalid list processing in calls database.
 *
 * Revision 1.15  2002/01/30 05:07:54  robertj
 * Some robustness changes, temporarily removed threads in RIP.
 *
 * Revision 1.14  2001/12/15 10:10:48  robertj
 * GCC compatibility
 *
 * Revision 1.13  2001/12/15 08:09:08  robertj
 * Added alerting, connect and end of call times to be sent to RAS server.
 *
 * Revision 1.12  2001/12/14 06:41:17  robertj
 * Added call end reason codes in DisengageRequest for GK server use.
 *
 * Revision 1.11  2001/12/13 11:08:45  robertj
 * Significant changes to support slow request handling, automatically sending
 *   RIP and spawning thread to handle time consuming operation.
 *
 * Revision 1.10  2001/12/06 06:44:42  robertj
 * Removed "Win32 SSL xxx" build configurations in favour of system
 *   environment variables to select optional libraries.
 *
 * Revision 1.9  2001/11/21 13:35:23  craigs
 * Removed deadlock with empty registered endpoint list
 *
 * Revision 1.8  2001/11/19 06:56:44  robertj
 * Added prefix strings for gateways registered with this gk, thanks Mikael Stolt
 *
 * Revision 1.7  2001/09/18 10:06:54  robertj
 * Added trace for if RRJ on security because user has no password.
 *
 * Revision 1.6  2001/09/13 01:15:20  robertj
 * Added flag to H235Authenticator to determine if gkid and epid is to be
 *   automatically set as the crypto token remote id and local id.
 *
 * Revision 1.5  2001/08/10 11:03:52  robertj
 * Major changes to H.235 support in RAS to support server.
 *
 * Revision 1.4  2001/08/06 07:44:55  robertj
 * Fixed problems with building without SSL
 *
 * Revision 1.3  2001/08/06 03:18:38  robertj
 * Fission of h323.h to h323ep.h & h323con.h, h323.h now just includes files.
 * Improved access to H.235 secure RAS functionality.
 * Changes to H.323 secure RAS contexts to help use with gk server.
 *
 * Revision 1.2  2001/07/24 10:12:33  craigs
 * Fixed problem with admission requests not having src signalling addresses
 *
 * Revision 1.1  2001/07/24 02:31:07  robertj
 * Added gatekeeper RAS protocol server classes.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "gkserver.h"
#endif

#include <h323/gkserver.h>

#include <h323/h323ep.h>
#include <h323/h323pdu.h>

const char AnswerCallStr[] = "-Answer";
const char OriginateCallStr[] = "-Originate";


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

H323GatekeeperRequest::H323GatekeeperRequest(H323GatekeeperListener & ras,
                                             const H323RasPDU & pdu)
  : rasChannel(ras),
    replyAddresses(1),
    request(pdu)
{
  H323TransportAddress senderAddress = ras.GetTransport().GetLastReceivedAddress();
  replyAddresses.SetAt(0, new H323TransportAddress(senderAddress));

  authenticity = UnknownAuthentication;
  fastResponseRequired = TRUE;
}


BOOL H323GatekeeperRequest::HandlePDU()
{
  int response = OnHandlePDU();
  switch (response) {
    case Ignore :
      return FALSE;

    case Confirm :
      WritePDU(confirm);
      return FALSE;

    case Reject :
      WritePDU(reject);
      return FALSE;
  }

  H323RasPDU pdu;
  pdu.BuildRequestInProgress(request.GetSequenceNumber(), response);
  if (!WritePDU(pdu))
    return FALSE;

  if (fastResponseRequired) {
    fastResponseRequired = FALSE;
    PThread::Create(PCREATE_NOTIFIER(SlowHandler), 0,
                                     PThread::AutoDeleteThread,
                                     PThread::NormalPriority,
                                     "GkSrvr:%x");
  }

  return TRUE;
}


void H323GatekeeperRequest::SlowHandler(PThread &, INT)
{
  PTRACE(3, "RAS\tStarted slow PDU handler thread.");

  while (HandlePDU())
    ;

  delete this;

  PTRACE(3, "RAS\tEnded slow PDU handler thread.");
}


BOOL H323GatekeeperRequest::WritePDU(H323RasPDU & pdu)
{
  PTRACE_BLOCK("H323GatekeeperRequest::WritePDU");

  if (endpoint == NULL)
    return rasChannel.WriteTo(pdu, replyAddresses, TRUE);

  pdu.SetAuthenticators(endpoint->GetAuthenticators());
  return rasChannel.WriteTo(pdu, endpoint->GetRASAddresses(), TRUE);
}


BOOL H323GatekeeperRequest::CheckGatekeeperIdentifier()
{
  PString pduGkid = GetGatekeeperIdentifier();
  if (pduGkid.IsEmpty()) // Not present in PDU
    return TRUE;

  PString rasGkid = rasChannel.GetIdentifier();

  // If it is present it has to be correct!
  if (pduGkid == rasGkid)
    return TRUE;

  SetRejectReason(GetGatekeeperRejectTag());
  PTRACE(2, "RAS\t" << GetName() << " rejected, has different identifier,"
            " got \"" << pduGkid << "\", should be \"" << rasGkid << '"');
  return FALSE;
}


BOOL H323GatekeeperRequest::GetRegisteredEndPoint()
{
  if (endpoint != NULL) {
    PTRACE(4, "RAS\tAlready located endpoint: " << *endpoint);
    return TRUE;
  }

  PString id = GetEndpointIdentifier();
  endpoint = rasChannel.GetGatekeeper().FindEndPointByIdentifier(id);
  if (endpoint != NULL) {
    PTRACE(4, "RAS\tLocated endpoint: " << *endpoint);
    return TRUE;
  }

  SetRejectReason(GetRegisteredEndPointRejectTag());
  PTRACE(2, "RAS\t" << GetName() << " rejected,"
            " \"" << id << "\" not registered");
  return FALSE;
}


BOOL H323GatekeeperRequest::CheckCryptoTokens()
{
  return CheckCryptoTokens(endpoint->GetAuthenticators());
}


BOOL H323GatekeeperRequest::CheckCryptoTokens(const H235Authenticators & authenticators)
{
  if (authenticity == UnknownAuthentication) {
    request.SetAuthenticators(authenticators);
    authenticity = request.Validate(GetCryptoTokens(),
                                    GetRawPDU(),
                                    GetCryptoTokensField())
                                ? IsAuthenticated : FailedAuthentication;
  }

  if (authenticity == IsAuthenticated)
    return TRUE;

  SetRejectReason(GetSecurityRejectTag());
  PTRACE(2, "RAS\t" << GetName() << " rejected,"
            " security tokens for \"" << GetEndpointIdentifier() << "\" invalid.");
  return FALSE;
}


/////////////////////////////////////////////////////////////////////////////

H323GatekeeperGRQ::H323GatekeeperGRQ(H323GatekeeperListener & rasChannel,
                                     const H323RasPDU & pdu)
  : H323GatekeeperRequest(rasChannel, pdu),
    grq(request),
    gcf(confirm.BuildGatekeeperConfirm(grq.m_requestSeqNum)),
    grj(reject.BuildGatekeeperReject(grq.m_requestSeqNum,
                                     H225_GatekeeperRejectReason::e_terminalExcluded))
{
  H323TransportAddress addr(grq.m_rasAddress, "udp");
  if (rasChannel.GetTransport().IsCompatibleTransport(addr))
    replyAddresses[0] = addr;
}


#if PTRACING
const char * H323GatekeeperGRQ::GetName() const
{
  return "GRQ";
}
#endif


const PASN_Sequence & H323GatekeeperGRQ::GetRawPDU() const
{
  return grq;
}


PString H323GatekeeperGRQ::GetGatekeeperIdentifier() const
{
  if (grq.HasOptionalField(H225_GatekeeperRequest::e_gatekeeperIdentifier))
    return grq.m_gatekeeperIdentifier;

  return PString::Empty();
}


unsigned H323GatekeeperGRQ::GetGatekeeperRejectTag() const
{
  return H225_GatekeeperRejectReason::e_terminalExcluded;
}


PString H323GatekeeperGRQ::GetEndpointIdentifier() const
{
  return PString::Empty(); // Dummy, should never be used
}


unsigned H323GatekeeperGRQ::GetRegisteredEndPointRejectTag() const
{
  return H225_GatekeeperRejectReason::e_terminalExcluded;
}


const H225_ArrayOf_CryptoH323Token & H323GatekeeperGRQ::GetCryptoTokens() const
{
  return grq.m_cryptoTokens;
}


unsigned H323GatekeeperGRQ::GetCryptoTokensField() const
{
  return H225_GatekeeperRequest::e_cryptoTokens;
}


unsigned H323GatekeeperGRQ::GetSecurityRejectTag() const
{
  return H225_GatekeeperRejectReason::e_securityDenial;
}


void H323GatekeeperGRQ::SetRejectReason(unsigned reasonCode)
{
  grj.m_rejectReason.SetTag(reasonCode);
}


H323GatekeeperRequest::Response H323GatekeeperGRQ::OnHandlePDU()
{
  return rasChannel.OnDiscovery(*this);
}


H323GatekeeperRRQ::H323GatekeeperRRQ(H323GatekeeperListener & rasChannel,
                                     const H323RasPDU & pdu)
  : H323GatekeeperRequest(rasChannel, pdu),
    rrq(request),
    rcf(confirm.BuildRegistrationConfirm(rrq.m_requestSeqNum)),
    rrj(reject.BuildRegistrationReject(rrq.m_requestSeqNum))
{
  replyAddresses.SetSize(rrq.m_rasAddress.GetSize());
  for (PINDEX i = 0; i < rrq.m_rasAddress.GetSize(); i++) {
    H323TransportAddress addr(rrq.m_rasAddress[i], "udp");
    if (rasChannel.GetTransport().IsCompatibleTransport(addr)) {
      replyAddresses.SetAt(i, new H323TransportAddress(addr));
      break;
    }
  }
}


#if PTRACING
const char * H323GatekeeperRRQ::GetName() const
{
  return "RRQ";
}
#endif


const PASN_Sequence & H323GatekeeperRRQ::GetRawPDU() const
{
  return rrq;
}


PString H323GatekeeperRRQ::GetGatekeeperIdentifier() const
{
  if (rrq.HasOptionalField(H225_RegistrationRequest::e_gatekeeperIdentifier))
    return rrq.m_gatekeeperIdentifier;

  return PString::Empty();
}


unsigned H323GatekeeperRRQ::GetGatekeeperRejectTag() const
{
  return H225_RegistrationRejectReason::e_undefinedReason;
}


PString H323GatekeeperRRQ::GetEndpointIdentifier() const
{
  return rrq.m_endpointIdentifier;
}


unsigned H323GatekeeperRRQ::GetRegisteredEndPointRejectTag() const
{
  return H225_RegistrationRejectReason::e_fullRegistrationRequired;
}


const H225_ArrayOf_CryptoH323Token & H323GatekeeperRRQ::GetCryptoTokens() const
{
  return rrq.m_cryptoTokens;
}


unsigned H323GatekeeperRRQ::GetCryptoTokensField() const
{
  return H225_RegistrationRequest::e_cryptoTokens;
}


unsigned H323GatekeeperRRQ::GetSecurityRejectTag() const
{
  return H225_RegistrationRejectReason::e_securityDenial;
}


void H323GatekeeperRRQ::SetRejectReason(unsigned reasonCode)
{
  rrj.m_rejectReason.SetTag(reasonCode);
}


H323GatekeeperRequest::Response H323GatekeeperRRQ::OnHandlePDU()
{
  return rasChannel.OnRegistration(*this);
}


H323GatekeeperURQ::H323GatekeeperURQ(H323GatekeeperListener & rasChannel,
                                     const H323RasPDU & pdu)
  : H323GatekeeperRequest(rasChannel, pdu),
    urq(request),
    ucf(confirm.BuildUnregistrationConfirm(urq.m_requestSeqNum)),
    urj(reject.BuildUnregistrationReject(urq.m_requestSeqNum))
{
}


#if PTRACING
const char * H323GatekeeperURQ::GetName() const
{
  return "URQ";
}
#endif


const PASN_Sequence & H323GatekeeperURQ::GetRawPDU() const
{
  return urq;
}


PString H323GatekeeperURQ::GetGatekeeperIdentifier() const
{
  if (urq.HasOptionalField(H225_UnregistrationRequest::e_gatekeeperIdentifier))
    return urq.m_gatekeeperIdentifier;

  return PString::Empty();
}


unsigned H323GatekeeperURQ::GetGatekeeperRejectTag() const
{
  return H225_GatekeeperRejectReason::e_terminalExcluded;
}


PString H323GatekeeperURQ::GetEndpointIdentifier() const
{
  return urq.m_endpointIdentifier;
}


unsigned H323GatekeeperURQ::GetRegisteredEndPointRejectTag() const
{
  return H225_UnregRejectReason::e_notCurrentlyRegistered;
}


const H225_ArrayOf_CryptoH323Token & H323GatekeeperURQ::GetCryptoTokens() const
{
  return urq.m_cryptoTokens;
}


unsigned H323GatekeeperURQ::GetCryptoTokensField() const
{
  return H225_UnregistrationRequest::e_cryptoTokens;
}


unsigned H323GatekeeperURQ::GetSecurityRejectTag() const
{
  return H225_UnregRejectReason::e_securityDenial;
}


void H323GatekeeperURQ::SetRejectReason(unsigned reasonCode)
{
  urj.m_rejectReason.SetTag(reasonCode);
}


H323GatekeeperRequest::Response H323GatekeeperURQ::OnHandlePDU()
{
  return rasChannel.OnUnregistration(*this);
}


H323GatekeeperARQ::H323GatekeeperARQ(H323GatekeeperListener & rasChannel,
                                     const H323RasPDU & pdu)
  : H323GatekeeperRequest(rasChannel, pdu),
    arq(request),
    acf(confirm.BuildAdmissionConfirm(arq.m_requestSeqNum)),
    arj(reject.BuildAdmissionReject(arq.m_requestSeqNum))
{
}


#if PTRACING
const char * H323GatekeeperARQ::GetName() const
{
  return "ARQ";
}
#endif


const PASN_Sequence & H323GatekeeperARQ::GetRawPDU() const
{
  return arq;
}


PString H323GatekeeperARQ::GetGatekeeperIdentifier() const
{
  if (arq.HasOptionalField(H225_AdmissionRequest::e_gatekeeperIdentifier))
    return arq.m_gatekeeperIdentifier;

  return PString::Empty();
}


unsigned H323GatekeeperARQ::GetGatekeeperRejectTag() const
{
  return H225_AdmissionRejectReason::e_undefinedReason;
}


PString H323GatekeeperARQ::GetEndpointIdentifier() const
{
  return arq.m_endpointIdentifier;
}


unsigned H323GatekeeperARQ::GetRegisteredEndPointRejectTag() const
{
  return H225_AdmissionRejectReason::e_invalidEndpointIdentifier;
}


const H225_ArrayOf_CryptoH323Token & H323GatekeeperARQ::GetCryptoTokens() const
{
  return arq.m_cryptoTokens;
}


unsigned H323GatekeeperARQ::GetCryptoTokensField() const
{
  return H225_AdmissionRequest::e_cryptoTokens;
}


unsigned H323GatekeeperARQ::GetSecurityRejectTag() const
{
  return H225_AdmissionRejectReason::e_securityDenial;
}


void H323GatekeeperARQ::SetRejectReason(unsigned reasonCode)
{
  arj.m_rejectReason.SetTag(reasonCode);
}


H323GatekeeperRequest::Response H323GatekeeperARQ::OnHandlePDU()
{
  H323GatekeeperRequest::Response response = rasChannel.OnAdmission(*this);
  if (response != Reject)
    return response;

  H323GatekeeperServer & server = rasChannel.GetGatekeeper();
  PSafePtr<H323GatekeeperCall> call = server.FindCall(arq.m_callIdentifier.m_guid, arq.m_answerCall);
  if (call != NULL)
    server.RemoveCall(call);

  return Reject;
}


H323GatekeeperDRQ::H323GatekeeperDRQ(H323GatekeeperListener & rasChannel,
                                     const H323RasPDU & pdu)
  : H323GatekeeperRequest(rasChannel, pdu),
    drq(request),
    dcf(confirm.BuildDisengageConfirm(drq.m_requestSeqNum)),
    drj(reject.BuildDisengageReject(drq.m_requestSeqNum))
{
}


#if PTRACING
const char * H323GatekeeperDRQ::GetName() const
{
  return "DRQ";
}
#endif


const PASN_Sequence & H323GatekeeperDRQ::GetRawPDU() const
{
  return drq;
}


PString H323GatekeeperDRQ::GetGatekeeperIdentifier() const
{
  if (drq.HasOptionalField(H225_DisengageRequest::e_gatekeeperIdentifier))
    return drq.m_gatekeeperIdentifier;

  return PString::Empty();
}


unsigned H323GatekeeperDRQ::GetGatekeeperRejectTag() const
{
  return H225_DisengageRejectReason::e_notRegistered;
}


PString H323GatekeeperDRQ::GetEndpointIdentifier() const
{
  return drq.m_endpointIdentifier;
}


unsigned H323GatekeeperDRQ::GetRegisteredEndPointRejectTag() const
{
  return H225_DisengageRejectReason::e_notRegistered;
}


const H225_ArrayOf_CryptoH323Token & H323GatekeeperDRQ::GetCryptoTokens() const
{
  return drq.m_cryptoTokens;
}


unsigned H323GatekeeperDRQ::GetCryptoTokensField() const
{
  return H225_DisengageRequest::e_cryptoTokens;
}


unsigned H323GatekeeperDRQ::GetSecurityRejectTag() const
{
  return H225_DisengageRejectReason::e_securityDenial;
}


void H323GatekeeperDRQ::SetRejectReason(unsigned reasonCode)
{
  drj.m_rejectReason.SetTag(reasonCode);
}


H323GatekeeperRequest::Response H323GatekeeperDRQ::OnHandlePDU()
{
  return rasChannel.OnDisengage(*this);
}


H323GatekeeperBRQ::H323GatekeeperBRQ(H323GatekeeperListener & rasChannel,
                                     const H323RasPDU & pdu)
  : H323GatekeeperRequest(rasChannel, pdu),
    brq(request),
    bcf(confirm.BuildBandwidthConfirm(brq.m_requestSeqNum)),
    brj(reject.BuildBandwidthReject(brq.m_requestSeqNum))
{
}


#if PTRACING
const char * H323GatekeeperBRQ::GetName() const
{
  return "BRQ";
}
#endif


const PASN_Sequence & H323GatekeeperBRQ::GetRawPDU() const
{
  return brq;
}


PString H323GatekeeperBRQ::GetGatekeeperIdentifier() const
{
  if (brq.HasOptionalField(H225_BandwidthRequest::e_gatekeeperIdentifier))
    return brq.m_gatekeeperIdentifier;

  return PString::Empty();
}


unsigned H323GatekeeperBRQ::GetGatekeeperRejectTag() const
{
  return H225_BandRejectReason::e_undefinedReason;
}


PString H323GatekeeperBRQ::GetEndpointIdentifier() const
{
  return brq.m_endpointIdentifier;
}


unsigned H323GatekeeperBRQ::GetRegisteredEndPointRejectTag() const
{
  return H225_BandRejectReason::e_notBound;
}


const H225_ArrayOf_CryptoH323Token & H323GatekeeperBRQ::GetCryptoTokens() const
{
  return brq.m_cryptoTokens;
}


unsigned H323GatekeeperBRQ::GetCryptoTokensField() const
{
  return H225_BandwidthRequest::e_cryptoTokens;
}


unsigned H323GatekeeperBRQ::GetSecurityRejectTag() const
{
  return H225_BandRejectReason::e_securityDenial;
}


void H323GatekeeperBRQ::SetRejectReason(unsigned reasonCode)
{
  brj.m_rejectReason.SetTag(reasonCode);
}


H323GatekeeperRequest::Response H323GatekeeperBRQ::OnHandlePDU()
{
  return rasChannel.OnBandwidth(*this);
}


H323GatekeeperLRQ::H323GatekeeperLRQ(H323GatekeeperListener & rasChannel,
                                     const H323RasPDU & pdu)
  : H323GatekeeperRequest(rasChannel, pdu),
    lrq(request),
    lcf(confirm.BuildLocationConfirm(lrq.m_requestSeqNum)),
    lrj(reject.BuildLocationReject(lrq.m_requestSeqNum))
{
  H323TransportAddress addr(lrq.m_replyAddress, "udp");
  if (rasChannel.GetTransport().IsCompatibleTransport(addr))
    replyAddresses[0] = addr;
}


#if PTRACING
const char * H323GatekeeperLRQ::GetName() const
{
  return "LRQ";
}
#endif


const PASN_Sequence & H323GatekeeperLRQ::GetRawPDU() const
{
  return lrq;
}


PString H323GatekeeperLRQ::GetGatekeeperIdentifier() const
{
  if (lrq.HasOptionalField(H225_LocationRequest::e_gatekeeperIdentifier))
    return lrq.m_gatekeeperIdentifier;

  return PString::Empty();
}


unsigned H323GatekeeperLRQ::GetGatekeeperRejectTag() const
{
  return H225_LocationRejectReason::e_undefinedReason;
}


PString H323GatekeeperLRQ::GetEndpointIdentifier() const
{
  return lrq.m_endpointIdentifier;
}


unsigned H323GatekeeperLRQ::GetRegisteredEndPointRejectTag() const
{
  return H225_LocationRejectReason::e_notRegistered;
}


const H225_ArrayOf_CryptoH323Token & H323GatekeeperLRQ::GetCryptoTokens() const
{
  return lrq.m_cryptoTokens;
}


unsigned H323GatekeeperLRQ::GetCryptoTokensField() const
{
  return H225_LocationRequest::e_cryptoTokens;
}


unsigned H323GatekeeperLRQ::GetSecurityRejectTag() const
{
  return H225_LocationRejectReason::e_securityDenial;
}


void H323GatekeeperLRQ::SetRejectReason(unsigned reasonCode)
{
  lrj.m_rejectReason.SetTag(reasonCode);
}


H323GatekeeperRequest::Response H323GatekeeperLRQ::OnHandlePDU()
{
  return rasChannel.OnLocation(*this);
}


H323GatekeeperIRR::H323GatekeeperIRR(H323GatekeeperListener & rasChannel,
                                     const H323RasPDU & pdu)
  : H323GatekeeperRequest(rasChannel, pdu),
    irr(request),
    iack(confirm.BuildInfoRequestAck(irr.m_requestSeqNum)),
    inak(reject.BuildInfoRequestNak(irr.m_requestSeqNum))
{
}


#if PTRACING
const char * H323GatekeeperIRR::GetName() const
{
  return "IRR";
}
#endif


const PASN_Sequence & H323GatekeeperIRR::GetRawPDU() const
{
  return irr;
}


PString H323GatekeeperIRR::GetGatekeeperIdentifier() const
{
  return PString::Empty();
}


unsigned H323GatekeeperIRR::GetGatekeeperRejectTag() const
{
  return H225_GatekeeperRejectReason::e_terminalExcluded;
}


PString H323GatekeeperIRR::GetEndpointIdentifier() const
{
  return irr.m_endpointIdentifier;
}


unsigned H323GatekeeperIRR::GetRegisteredEndPointRejectTag() const
{
  return H225_InfoRequestNakReason::e_notRegistered;
}


const H225_ArrayOf_CryptoH323Token & H323GatekeeperIRR::GetCryptoTokens() const
{
  return irr.m_cryptoTokens;
}


unsigned H323GatekeeperIRR::GetCryptoTokensField() const
{
  return H225_InfoRequestResponse::e_cryptoTokens;
}


unsigned H323GatekeeperIRR::GetSecurityRejectTag() const
{
  return H225_InfoRequestNakReason::e_securityDenial;
}


void H323GatekeeperIRR::SetRejectReason(unsigned reasonCode)
{
  inak.m_nakReason.SetTag(reasonCode);
}


H323GatekeeperRequest::Response H323GatekeeperIRR::OnHandlePDU()
{
  return rasChannel.OnInfoResponse(*this);
}


/////////////////////////////////////////////////////////////////////////////

H323GatekeeperCall::H323GatekeeperCall(H323GatekeeperServer& gk,
                                       const OpalGloballyUniqueID & id,
                                       Direction dir)
  : gatekeeper(gk),
    callIdentifier(id),
    conferenceIdentifier(NULL),
    alertingTime(0),
    connectedTime(0),
    callEndTime(0)
{
  endpoint = NULL;
  rasChannel = NULL;
  direction = dir;
  callReference = 0;

  drqReceived = FALSE;
  callEndReason = H323Connection::NumCallEndReasons;

  bandwidthUsed = 0;
  infoResponseRate = gatekeeper.GetInfoResponseRate();
}


H323GatekeeperCall::~H323GatekeeperCall()
{
}


PObject::Comparison H323GatekeeperCall::Compare(const PObject & obj) const
{
  // Do not have to lock the object as these fields should never change for
  // life of the object

  PAssert(obj.IsDescendant(H323GatekeeperCall::Class()), PInvalidCast);
  const H323GatekeeperCall & other = (const H323GatekeeperCall &)obj;
  Comparison result = callIdentifier.Compare(other.callIdentifier);
  if (result != EqualTo)
    return result;

  if (direction == UnknownDirection || other.direction == UnknownDirection)
    return EqualTo;

  if (direction > other.direction)
    return GreaterThan;
  if (direction < other.direction)
    return LessThan;
  return EqualTo;
}


void H323GatekeeperCall::PrintOn(ostream & strm) const
{
  // Do not have to lock the object as these fields should never change for
  // life of the object

  strm << callIdentifier;

  switch (direction) {
    case AnsweringCall :
      strm << AnswerCallStr;
      break;
    case OriginatingCall :
      strm << OriginateCallStr;
      break;
    default :
      break;
  }
}


H323GatekeeperRequest::Response H323GatekeeperCall::OnAdmission(H323GatekeeperARQ & info)
{
  PTRACE_BLOCK("H323GatekeeperCall::OnAdmission");

  if (endpoint != NULL) {
    info.SetRejectReason(H225_AdmissionRejectReason::e_resourceUnavailable);
    PTRACE(2, "RAS\tARQ rejected, multiple use of same call id.");
    return H323GatekeeperRequest::Reject;
  }

  PINDEX i;

  LockReadWrite();

  PTRACE(3, "RAS\tProcessing OnAdmission for " << *this);

  endpoint = info.endpoint;
  rasChannel = &info.GetRasChannel();

  callReference = info.arq.m_callReferenceValue;
  conferenceIdentifier = info.arq.m_conferenceID;

  for (i = 0; i < info.arq.m_srcInfo.GetSize(); i++) {
    PString alias = H323GetAliasAddressString(info.arq.m_srcInfo[i]);
    if (srcAliases.GetValuesIndex(alias) == P_MAX_INDEX)
      srcAliases += alias;
  }

  srcNumber = H323GetAliasAddressE164(info.arq.m_srcInfo);

  if (info.arq.HasOptionalField(H225_AdmissionRequest::e_srcCallSignalAddress))
    srcHost = info.arq.m_srcCallSignalAddress;
  else
    srcHost = info.GetReplyAddress();

  if (info.arq.HasOptionalField(H225_AdmissionRequest::e_destinationInfo)) {
    for (i = 0; i < info.arq.m_destinationInfo.GetSize(); i++) {
      PString alias = H323GetAliasAddressString(info.arq.m_destinationInfo[i]);
      if (dstAliases.GetValuesIndex(alias) == P_MAX_INDEX)
        dstAliases += alias;
    }

    dstNumber = H323GetAliasAddressE164(info.arq.m_destinationInfo);
  }

  if (info.arq.HasOptionalField(H225_AdmissionRequest::e_destCallSignalAddress))
    dstHost = info.arq.m_destCallSignalAddress;

  UnlockReadWrite();

  if (direction == AnsweringCall) {
    // See if our policies allow the call
    BOOL denied = TRUE;
    for (i = 0; i < info.arq.m_srcInfo.GetSize(); i++) {
      if (gatekeeper.CheckAliasAddressPolicy(*endpoint, info.arq, info.arq.m_srcInfo[i])) {
        denied = FALSE;
        break;
      }
    }

    if (info.arq.HasOptionalField(H225_AdmissionRequest::e_srcCallSignalAddress)) {
      if (gatekeeper.CheckSignalAddressPolicy(*endpoint, info.arq, info.arq.m_srcCallSignalAddress))
        denied = FALSE;
    }

    if (denied) {
      info.SetRejectReason(H225_AdmissionRejectReason::e_securityDenial);
      PTRACE(2, "RAS\tARQ rejected, not allowed to answer call");
      return H323GatekeeperRequest::Reject;
    }
  }
  else {
    PSafePtr<H323RegisteredEndPoint> destEP;

    // Do lookup by alias
    if (info.arq.HasOptionalField(H225_AdmissionRequest::e_destinationInfo)) {

      // Have provided an alias, see if we can route it.
      BOOL denied = TRUE;
      BOOL noAddress = TRUE;
      for (i = 0; i < info.arq.m_destinationInfo.GetSize(); i++) {
        if (gatekeeper.CheckAliasAddressPolicy(*endpoint, info.arq, info.arq.m_destinationInfo[i])) {
          denied = FALSE;
          if (gatekeeper.TranslateAliasAddressToSignalAddress(info.arq.m_destinationInfo[i], dstHost)) {
            destEP = gatekeeper.FindEndPointByAliasAddress(info.arq.m_destinationInfo[i]);
            LockReadWrite();
            UnlockReadWrite();
            noAddress = FALSE;
            break;
          }
        }
      }

      if (denied) {
        info.SetRejectReason(H225_AdmissionRejectReason::e_securityDenial);
        PTRACE(2, "RAS\tARQ rejected, not allowed to make call");
        return H323GatekeeperRequest::Reject;
      }

      if (noAddress) {
        info.SetRejectReason(H225_AdmissionRejectReason::e_calledPartyNotRegistered);
        PTRACE(2, "RAS\tARQ rejected, destination alias not registered");
        return H323GatekeeperRequest::Reject;
      }

      if (destEP != NULL) {
        destEP.SetSafetyMode(PSafeReadOnly);
        LockReadWrite();
        dstAliases.RemoveAll();
        dstNumber = PString::Empty();
        for (i = 0; i < destEP->GetAliasCount(); i++) {
          PString alias = destEP->GetAlias(i);
          dstAliases += alias;
          if (strspn(alias, "0123456789*#") == strlen(alias))
            dstNumber = alias;
        }
        UnlockReadWrite();
        destEP.SetSafetyMode(PSafeReference);
      }
    }

    // Has provided an alias and signal address, see if agree
    if (destEP != NULL && info.arq.HasOptionalField(H225_AdmissionRequest::e_destCallSignalAddress)) {
      // Note nested if's used because some compilers (Tornado 2) cannot handle it
      if (gatekeeper.FindEndPointBySignalAddress(info.arq.m_destCallSignalAddress) != destEP) {
        info.SetRejectReason(H225_AdmissionRejectReason::e_aliasesInconsistent);
        PTRACE(2, "RAS\tARQ rejected, destination address not for specified alias");
        return H323GatekeeperRequest::Reject;
      }
    }

    // Have no address from anywhere
    if (dstHost.IsEmpty()) {
      info.SetRejectReason(H225_AdmissionRejectReason::e_incompleteAddress);
      PTRACE(2, "RAS\tARQ rejected, must have destination address or alias");
      return H323GatekeeperRequest::Reject;
    }

    // Now see if security policy allows connection to that address
    if (!gatekeeper.CheckSignalAddressPolicy(*endpoint, info.arq, dstHost)) {
      info.SetRejectReason(H225_AdmissionRejectReason::e_securityDenial);
      PTRACE(2, "RAS\tARQ rejected, not allowed to make call");
      return H323GatekeeperRequest::Reject;
    }
  }

  // See if have bandwidth
  unsigned requestedBandwidth = info.arq.m_bandWidth;
  if (requestedBandwidth == 0)
    requestedBandwidth = gatekeeper.GetDefaultBandwidth();

  unsigned bandwidthAllocated = gatekeeper.AllocateBandwidth(requestedBandwidth);
  if (bandwidthAllocated == 0) {
    info.SetRejectReason(H225_AdmissionRejectReason::e_requestDenied);
    PTRACE(2, "RAS\tARQ rejected, not enough bandwidth");
    return H323GatekeeperRequest::Reject;
  }

  bandwidthUsed = bandwidthAllocated;  // Do we need to lock for atomic assignment?
  info.acf.m_bandWidth = bandwidthUsed;

  // Set the rate for getting unsolicited IRR's
  if (infoResponseRate > 0 && endpoint->GetProtocolVersion() > 2) {
    info.acf.IncludeOptionalField(H225_AdmissionConfirm::e_irrFrequency);
    info.acf.m_irrFrequency = infoResponseRate;
  }
  info.acf.m_willRespondToIRR = TRUE;

  // Set the destination to fixed value of gk routed
  if (gatekeeper.IsGatekeeperRouted())
    info.acf.m_callModel.SetTag(H225_CallModel::e_gatekeeperRouted);
  dstHost.SetPDU(info.acf.m_destCallSignalAddress);

  // If ep can do UUIE's then use that to expidite getting some statistics
  if (info.arq.m_willSupplyUUIEs) {
    info.acf.m_uuiesRequested.m_alerting = TRUE;
    info.acf.m_uuiesRequested.m_connect = TRUE;
  }

  return H323GatekeeperRequest::Confirm;
}


BOOL H323GatekeeperCall::Disengage(int reason)
{
  LockReadWrite();

  if (drqReceived) {
    UnlockReadWrite();
    PTRACE(1, "RAS\tAlready disengaged call " << *this);
    return FALSE;
  }

  drqReceived = TRUE;

  PTRACE(2, "RAS\tDisengage of call " << *this);

  UnlockReadWrite();

  if (reason == -1)
    reason = H225_DisengageReason::e_forcedDrop;

  BOOL ok;
  // Send DRQ to endpoint(s)
  if (rasChannel != NULL)
    ok = rasChannel->DisengageRequest(*this, reason);
  else {
    // Can't do DRQ as have no RAS channel to use (probably logic error)
    PAssertAlways("Tried to disengage call we did not receive ARQ for!");
    ok = FALSE;
  }

  gatekeeper.RemoveCall(this);

  return ok;
}


H323GatekeeperRequest::Response H323GatekeeperCall::OnDisengage(H323GatekeeperDRQ & info)
{
  PTRACE_BLOCK("H323GatekeeperCall::OnDisengage");

  LockReadWrite();

  if (drqReceived) {
    UnlockReadWrite();
    info.SetRejectReason(H225_DisengageRejectReason::e_requestToDropOther);
    PTRACE(2, "RAS\tDRQ rejected, already disengaged call " << *this);
    return H323GatekeeperRequest::Reject;
  }

  drqReceived = TRUE;

  if (info.drq.HasOptionalField(H225_DisengageRequest::e_usageInformation))
    SetUsageInfo(info.drq.m_usageInformation);

  if (info.drq.HasOptionalField(H225_DisengageRequest::e_terminationCause)) {
    if (info.drq.m_terminationCause.GetTag() == H225_CallTerminationCause::e_releaseCompleteReason) {
      H225_ReleaseCompleteReason & reason = info.drq.m_terminationCause;
      callEndReason = H323TranslateToCallEndReason(Q931::ErrorInCauseIE, reason);
    }
    else {
      PASN_OctetString & cause = info.drq.m_terminationCause;
      H225_ReleaseCompleteReason dummy;
      callEndReason = H323TranslateToCallEndReason((Q931::CauseValues)(cause[1]&0x7f), dummy);
    }
  }

  UnlockReadWrite();

  return H323GatekeeperRequest::Confirm;
}


H323GatekeeperRequest::Response H323GatekeeperCall::OnBandwidth(H323GatekeeperBRQ & info)
{
  PTRACE_BLOCK("H323GatekeeperCall::OnBandwidth");

  if (endpoint != info.endpoint) {
    info.SetRejectReason(H225_BandRejectReason::e_invalidPermission);
    PTRACE(2, "RAS\tBRQ rejected, call is not owned by endpoint");
    return H323GatekeeperRequest::Reject;
  }

  unsigned newBandwidth = gatekeeper.AllocateBandwidth(info.brq.m_bandWidth, bandwidthUsed);
  if (newBandwidth == 0) {
    info.SetRejectReason(H225_BandRejectReason::e_insufficientResources);
    PTRACE(2, "RAS\tBRQ rejected, no bandwidth");
    return H323GatekeeperRequest::Reject;
  }

  bandwidthUsed = newBandwidth;  // Do we need to lock for atomic assignment?
  info.bcf.m_bandWidth = newBandwidth;

  if (info.brq.HasOptionalField(H225_BandwidthRequest::e_usageInformation))
    SetUsageInfo(info.brq.m_usageInformation);

  return H323GatekeeperRequest::Confirm;
}


H323GatekeeperRequest::Response H323GatekeeperCall::OnInfoResponse(H323GatekeeperIRR &,
                                                                   H225_InfoRequestResponse_perCallInfo_subtype & info)
{
  PTRACE_BLOCK("H323GatekeeperCall::OnInfoResponse");

  PTRACE(2, "RAS\tIRR received for call " << *this);

  LockReadWrite();

  PTime now;
  lastInfoResponse = now;

  // Detect if have Cisco non-standard version of connect time indication.
  if (!connectedTime.IsValid() &&
      info.HasOptionalField(H225_InfoRequestResponse_perCallInfo_subtype::e_nonStandardData) &&
      info.m_nonStandardData.m_nonStandardIdentifier.GetTag() == H225_NonStandardIdentifier::e_h221NonStandard) {
    H225_H221NonStandard & id = info.m_nonStandardData.m_nonStandardIdentifier;
    if (id.m_t35CountryCode == 181 && id.m_t35Extension == 0 && id.m_manufacturerCode == 18 && // Cisco
        info.m_nonStandardData.GetSize() == 5 && info.m_nonStandardData.m_data[0] == 0x70) {
      PTime theConnectedTime((info.m_nonStandardData.m_data[1] << 24)|
                            (info.m_nonStandardData.m_data[2] << 16)|
                            (info.m_nonStandardData.m_data[3] << 8 )|
                             info.m_nonStandardData.m_data[4]        );
      if (theConnectedTime > now) {
        connectedTime = now;
        OnConnected();
      }
      else if (theConnectedTime > callStartTime) {
        connectedTime = theConnectedTime;
        OnConnected();
      }
    }
  }

  SetUsageInfo(info.m_usageInformation);

  UnlockReadWrite();

  return H323GatekeeperRequest::Confirm;
}


void H323GatekeeperCall::OnAlerting()
{
}


void H323GatekeeperCall::OnConnected()
{
}


static BOOL CheckTimeSince(PTime & lastTime, unsigned threshold)
{
  if (threshold == 0)
    return TRUE;

  PTime now;
  PTimeInterval delta = now - lastTime;
  return delta.GetSeconds() < (int)(threshold+10);
}


BOOL H323GatekeeperCall::OnHeartbeat()
{
  {
    PSafePtr<H323GatekeeperCall> lock(this, PSafeReadOnly);

    if (CheckTimeSince(lastInfoResponse, infoResponseRate))
      return TRUE;

    // Can't do IRQ as have no RAS channel to use (probably logic error)
    if (rasChannel == NULL) {
      PAssertAlways("Timeout on heartbeat for call we did not receive ARQ for!");
      return FALSE;
    }

  // Exit here unlocks the call
  }

  // Do IRQ and wait for IRR on call
  PTRACE(2, "RAS\tTimeout on heartbeat, doing IRQ for call "<< *this);
  if (!rasChannel->InfoRequest(*endpoint, this))
    return FALSE;

  PSafePtr<H323GatekeeperCall> lock(this, PSafeReadOnly);

  // Return TRUE if got a resonse, ie client does not do unsolicited IRR's
  // otherwise did not get a response from client so return FALSE and
  // (probably) disengage the call.
  return CheckTimeSince(lastInfoResponse, infoResponseRate);
}


PString H323GatekeeperCall::GetCallCreditAmount() const
{
  if (endpoint != NULL)
    return endpoint->GetCallCreditAmount();
  
  return PString::Empty();
}


BOOL H323GatekeeperCall::GetCallCreditMode() const
{
  return endpoint != NULL ? endpoint->GetCallCreditMode() : FALSE;
}


unsigned H323GatekeeperCall::GetDurationLimit() const
{
  return 0;
}


BOOL H323GatekeeperCall::SendCallCreditServiceControl()
{
  PString amount;
  if (endpoint->CanDisplayAmountString())
    amount = GetCallCreditAmount();

  unsigned durationLimit = 0;
  if (endpoint->CanEnforceDurationLimit())
    durationLimit = GetDurationLimit();

  if (amount.IsEmpty() && durationLimit == 0)
    return FALSE;

  H323CallCreditServiceControl credit(amount, GetCallCreditMode(), durationLimit);
  return SendServiceControlSession(credit);
}


BOOL H323GatekeeperCall::AddCallCreditServiceControl(H225_ArrayOf_ServiceControlSession & serviceControl) const
{
  PString amount;
  if (endpoint->CanDisplayAmountString())
    amount = GetCallCreditAmount();

  unsigned durationLimit = 0;
  if (endpoint->CanEnforceDurationLimit())
    durationLimit = GetDurationLimit();

  if (amount.IsEmpty() && durationLimit == 0)
    return FALSE;

  H323CallCreditServiceControl credit(amount, GetCallCreditMode(), durationLimit);
  return endpoint->AddServiceControlSession(credit, serviceControl);
}


BOOL H323GatekeeperCall::SendServiceControlSession(const H323ServiceControlSession & session)
{
  // Send SCI to endpoint(s)
  if (rasChannel == NULL || endpoint == NULL) {
    // Can't do SCI as have no RAS channel to use (probably logic error)
    PAssertAlways("Tried to do SCI to call we did not receive ARQ for!");
    return FALSE;
  }

  return rasChannel->ServiceControlIndication(*endpoint, session, this);
}


static PString MakeAddress(const PString & number,
                           const PStringArray aliases,
                           const H323TransportAddress & host)
{
  PStringStream addr;

  if (!number)
    addr << number;
  else if (!aliases.IsEmpty())
    addr << aliases[0];

  if (!host) {
    if (!addr.IsEmpty())
      addr << '@';
    addr << host;
  }

  return addr;
}


PString H323GatekeeperCall::GetSourceAddress() const
{
  LockReadOnly();
  PString addr = MakeAddress(srcNumber, srcAliases, srcHost);
  UnlockReadOnly();
  return addr;
}


PString H323GatekeeperCall::GetDestinationAddress() const
{
  LockReadOnly();
  PString addr = MakeAddress(dstNumber, dstAliases, dstHost);
  UnlockReadOnly();
  return addr;
}


void H323GatekeeperCall::SetUsageInfo(const H225_RasUsageInformation & usage)
{
  PTime now;

  if (!alertingTime.IsValid() &&
       usage.HasOptionalField(H225_RasUsageInformation::e_alertingTime)) {
    PTime theAlertingTime((unsigned)usage.m_alertingTime);
    if (theAlertingTime > now) {
      alertingTime = now;
      OnAlerting();
    }
    else if (theAlertingTime > callStartTime) {
      alertingTime = theAlertingTime;
      OnAlerting();
    }
  }

  if (!connectedTime.IsValid() &&
       usage.HasOptionalField(H225_RasUsageInformation::e_connectTime)) {
    PTime theConnectedTime((unsigned)usage.m_connectTime);
    if (theConnectedTime > now) {
      connectedTime = now;
      OnConnected();
    }
    else if (theConnectedTime > callStartTime) {
      connectedTime = theConnectedTime;
      OnConnected();
    }
  }

  if (!callEndTime.IsValid() &&
       usage.HasOptionalField(H225_RasUsageInformation::e_endTime)) {
    PTime theCallEndTime = PTime((unsigned)usage.m_endTime);
    if (theCallEndTime > now ||
       (alertingTime.IsValid() && theCallEndTime < alertingTime) ||
       (connectedTime.IsValid() && theCallEndTime < connectedTime) ||
        theCallEndTime < callStartTime)
      callEndTime = now;
    else
      callEndTime = theCallEndTime;
  }
}


/////////////////////////////////////////////////////////////////////////////

H323RegisteredEndPoint::H323RegisteredEndPoint(H323GatekeeperServer & gk,
                                               const PString & id)
  : gatekeeper(gk),
    rasChannel(NULL),
    identifier(id),
    protocolVersion(0),
    canDisplayAmountString(FALSE),
    canEnforceDurationLimit(FALSE),
    timeToLive(0),
    authenticators(gk.GetOwnerEndPoint().CreateAuthenticators())
{
  activeCalls.DisallowDeleteObjects();

  PTRACE(3, "RAS\tCreated registered endpoint: " << id);
}


PObject::Comparison H323RegisteredEndPoint::Compare(const PObject & obj) const
{
  // Do not have to lock the object as these fields should never change for
  // life of the object

  PAssert(obj.IsDescendant(H323RegisteredEndPoint::Class()), PInvalidCast);
  return identifier.Compare(((const H323RegisteredEndPoint &)obj).identifier);
}


void H323RegisteredEndPoint::PrintOn(ostream & strm) const
{
  // Do not have to lock the object as these fields should never change for
  // life of the object

  strm << identifier;
}


void H323RegisteredEndPoint::AddCall(H323GatekeeperCall * call)
{
  LockReadWrite();

  if (activeCalls.GetObjectsIndex(call) == P_MAX_INDEX)
    activeCalls.Append(call);

  UnlockReadWrite();
}


BOOL H323RegisteredEndPoint::RemoveCall(H323GatekeeperCall * call)
{
  LockReadWrite();

  BOOL ok = activeCalls.Remove(call);

  UnlockReadWrite();

  return ok;
}


static H323TransportAddressArray GetTransportAddressArray(const H225_ArrayOf_TransportAddress & pdu)
{
  H323TransportAddressArray addresses;

  PINDEX count = 0;
  for (PINDEX i = 0; i < pdu.GetSize(); i++) {
    H323TransportAddress address(pdu[i]);
    if (!address)
      addresses.SetAt(count++, new H323TransportAddress(address));
  }

  return addresses;
}


static BOOL IsTransportAddressSuperset(const H225_ArrayOf_TransportAddress & pdu,
                                       const H323TransportAddressArray & oldAddresses)
{
  H323TransportAddressArray newAddresses = GetTransportAddressArray(pdu);

  for (PINDEX i = 0; i < oldAddresses.GetSize(); i++) {
    if (newAddresses.GetValuesIndex(oldAddresses[i]) == P_MAX_INDEX)
      return FALSE;
  }

  return TRUE;
}


static PStringArray GetAliasAddressArray(const H225_ArrayOf_AliasAddress & pdu)
{
  PStringArray aliases;

  for (PINDEX i = 0; i < pdu.GetSize(); i++) {
    PString alias = H323GetAliasAddressString(pdu[i]);
    if (!alias)
      aliases.AppendString(alias);
  }

  return aliases;
}


static BOOL IsAliasAddressSuperset(const H225_ArrayOf_AliasAddress & pdu,
                                   const PStringArray & oldAliases)
{
  PStringArray newAliases = GetAliasAddressArray(pdu);

  for (PINDEX i = 0; i < oldAliases.GetSize(); i++) {
    if (newAliases.GetValuesIndex(oldAliases[i]) == P_MAX_INDEX)
      return FALSE;
  }

  return TRUE;
}


H323GatekeeperRequest::Response H323RegisteredEndPoint::OnRegistration(H323GatekeeperRRQ & info)
{
  PTRACE_BLOCK("H323RegisteredEndPoint::OnRegistration");

  LockReadWrite();

  rasChannel = &info.GetRasChannel();
  lastRegistration = PTime();
  protocolVersion = info.rrq.m_protocolIdentifier[5];

  timeToLive = gatekeeper.GetTimeToLive();
  if (info.rrq.HasOptionalField(H225_RegistrationRequest::e_timeToLive) &&
      timeToLive > info.rrq.m_timeToLive)
    timeToLive = info.rrq.m_timeToLive;

  if (timeToLive > 0) {
    info.rcf.IncludeOptionalField(H225_RegistrationRequest::e_timeToLive);
    info.rcf.m_timeToLive = timeToLive;
  }

  info.rcf.m_endpointIdentifier = identifier;

  UnlockReadWrite();

  if (info.rrq.m_keepAlive)
    return H323GatekeeperRequest::Confirm;

  if (info.rrq.HasOptionalField(H225_RegistrationRequest::e_endpointIdentifier)) {
    // Make sure addresses are a superset of previous registration
    if (!IsTransportAddressSuperset(info.rrq.m_rasAddress, rasAddresses) ||
        !IsTransportAddressSuperset(info.rrq.m_callSignalAddress, signalAddresses) ||
        (info.rrq.HasOptionalField(H225_RegistrationRequest::e_terminalAlias) &&
         !IsAliasAddressSuperset(info.rrq.m_terminalAlias, aliases))) {
      info.SetRejectReason(H225_RegistrationRejectReason::e_discoveryRequired);
      PTRACE(2, "RAS\tRRQ rejected, not superset of existing registration.");
      return H323GatekeeperRequest::Reject;
    }
    PTRACE(3, "RAS\tFull RRQ received for already registered endpoint");
  }

  H323GatekeeperRequest::Response response = OnFullRegistration(info);
  if (response != H323GatekeeperRequest::Confirm)
    return response;

  // Final check, the H.235 security
  if (!info.CheckCryptoTokens())
    return H323GatekeeperRequest::Reject;

  PINDEX i;

  info.rcf.m_callSignalAddress.SetSize(signalAddresses.GetSize());
  for (i = 0; i < signalAddresses.GetSize(); i++)
    signalAddresses[i].SetPDU(info.rcf.m_callSignalAddress[i]);

  if (aliases.GetSize() > 0) {
    info.rcf.IncludeOptionalField(H225_RegistrationConfirm::e_terminalAlias);
    info.rcf.m_terminalAlias.SetSize(aliases.GetSize());
    for (i = 0; i < aliases.GetSize(); i++)
      H323SetAliasAddress(aliases[i], info.rcf.m_terminalAlias[i]);
  }

  if (canDisplayAmountString) {
    H323CallCreditServiceControl credit(GetCallCreditAmount(), GetCallCreditMode());
    if (AddServiceControlSession(credit, info.rcf.m_serviceControl))
      info.rcf.IncludeOptionalField(H225_RegistrationConfirm::e_serviceControl);
  }

  return H323GatekeeperRequest::Confirm;
}


H323GatekeeperRequest::Response H323RegisteredEndPoint::OnFullRegistration(H323GatekeeperRRQ & info)
{
  PSafePtr<H323RegisteredEndPoint> lock(this, PSafeReadWrite);

  rasAddresses = GetTransportAddressArray(info.rrq.m_rasAddress);
  if (rasAddresses.IsEmpty()) {
    info.SetRejectReason(H225_RegistrationRejectReason::e_invalidRASAddress);
    PTRACE(2, "RAS\tRRQ rejected, does not have valid RAS address!");
    return H323GatekeeperRequest::Reject;
  }

  signalAddresses = GetTransportAddressArray(info.rrq.m_callSignalAddress);
  if (signalAddresses.IsEmpty()) {
    info.SetRejectReason(H225_RegistrationRejectReason::e_invalidCallSignalAddress);
    return H323GatekeeperRequest::Reject;
  }

  if (info.rrq.HasOptionalField(H225_RegistrationRequest::e_terminalAlias))
    aliases = GetAliasAddressArray(info.rrq.m_terminalAlias);

  const H225_EndpointType & terminalType = info.rrq.m_terminalType;
  if (terminalType.HasOptionalField(H225_EndpointType::e_gateway) &&
      terminalType.m_gateway.HasOptionalField(H225_GatewayInfo::e_protocol)) {
    const H225_ArrayOf_SupportedProtocols & protocols = terminalType.m_gateway.m_protocol;
    for (PINDEX i = 0; i < protocols.GetSize(); i++) {

      // Only voice prefixes are supported
      if (protocols[i].GetTag() == H225_SupportedProtocols::e_voice) {
	H225_VoiceCaps & voiceCaps = protocols[i];
	if (voiceCaps.HasOptionalField(H225_VoiceCaps::e_supportedPrefixes)) {
	  H225_ArrayOf_SupportedPrefix & prefixes = voiceCaps.m_supportedPrefixes;
	  voicePrefixes.SetSize(prefixes.GetSize());
	  for (PINDEX j = 0; j < prefixes.GetSize(); j++) {
	    PString prefix = H323GetAliasAddressString(prefixes[j].m_prefix);
	    voicePrefixes[j] = prefix;
	  }
	}
	break;  // If voice protocol is found, don't look any further
      }
    }
  }

  applicationInfo = H323GetApplicationInfo(info.rrq.m_endpointVendor);

  canDisplayAmountString = FALSE;
  canEnforceDurationLimit = FALSE;
  if (info.rrq.HasOptionalField(H225_RegistrationRequest::e_callCreditCapability)) {
    if (info.rrq.m_callCreditCapability.HasOptionalField(H225_CallCreditCapability::e_canDisplayAmountString))
      canDisplayAmountString = info.rrq.m_callCreditCapability.m_canDisplayAmountString;
    if (info.rrq.m_callCreditCapability.HasOptionalField(H225_CallCreditCapability::e_canEnforceDurationLimit))
      canEnforceDurationLimit = info.rrq.m_callCreditCapability.m_canEnforceDurationLimit;
  }

  return OnSecureRegistration(info);
}


H323GatekeeperRequest::Response H323RegisteredEndPoint::OnSecureRegistration(H323GatekeeperRRQ & info)
{
  for (PINDEX i = 0; i < aliases.GetSize(); i++) {
    PString password;
    if (gatekeeper.GetUsersPassword(aliases[i], password)) {
      if (!password)
        SetPassword(password);
      return H323GatekeeperRequest::Confirm;
    }
  }

  PTRACE(2, "RAS\tRejecting RRQ, no aliases have a password.");
  info.SetRejectReason(H225_RegistrationRejectReason::e_securityDenial);
  return H323GatekeeperRequest::Reject;
}


BOOL H323RegisteredEndPoint::SetPassword(const PString & password)
{
  if (authenticators.IsEmpty() || password.IsEmpty())
    return FALSE;

  for (PINDEX i = 0; i < authenticators.GetSize(); i++) {
    H235Authenticator & authenticator = authenticators[i];
    authenticator.SetPassword(password);
    authenticator.Enable();
  }

  return TRUE;
}


H323GatekeeperRequest::Response H323RegisteredEndPoint::OnUnregistration(H323GatekeeperURQ & info)
{
  PTRACE_BLOCK("H323RegisteredEndPoint::OnUnregistration");

  if (activeCalls.GetSize() > 0) {
    info.SetRejectReason(H225_UnregRejectReason::e_callInProgress);
    return H323GatekeeperRequest::Reject;
  }

  return H323GatekeeperRequest::Confirm;
}


BOOL H323RegisteredEndPoint::Unregister(int reason)
{
  if (reason == -1)
    reason = H225_UnregRequestReason::e_maintenance;

  BOOL ok;

  // Send DRQ to endpoint(s)
  if (rasChannel != NULL)
    ok = rasChannel->UnregistrationRequest(*this, reason);
  else {
    // Can't do DRQ as have no RAS channel to use (probably logic error)
    PAssertAlways("Tried to unregister endpoint we did not receive RRQ for!");
    ok = FALSE;
  }

  gatekeeper.RemoveEndPoint(this);

  return ok;
}


H323GatekeeperRequest::Response H323RegisteredEndPoint::OnInfoResponse(H323GatekeeperIRR & info)
{
  PTRACE_BLOCK("H323RegisteredEndPoint::OnInfoResponse");

  LockReadWrite();
  lastInfoResponse = PTime();
  UnlockReadWrite();

  if (info.irr.HasOptionalField(H225_InfoRequestResponse::e_irrStatus) &&
      info.irr.m_irrStatus.GetTag() == H225_InfoRequestResponseStatus::e_invalidCall) {
    PTRACE(2, "RAS\tIRR for call-id endpoint does not know about");
    return H323GatekeeperRequest::Confirm;
  }
    

  if (!info.irr.HasOptionalField(H225_InfoRequestResponse::e_perCallInfo)) {
    // Special case for Innovaphone clients that do not contain a perCallInfo
    // field and expects that to mean "all calls a normal".
    if (protocolVersion < 4 && applicationInfo.Find("innovaphone") != P_MAX_INDEX) {
      H225_InfoRequestResponse_perCallInfo_subtype fakeCallInfo;
      LockReadOnly();
      for (PINDEX i = 0; i < activeCalls.GetSize(); i++)
        activeCalls[i].OnInfoResponse(info, fakeCallInfo);
      UnlockReadOnly();
    }

    PTRACE(2, "RAS\tIRR for call-id endpoint does not know about");
    return H323GatekeeperRequest::Confirm;
  }

  LockReadOnly();

  for (PINDEX i = 0; i < info.irr.m_perCallInfo.GetSize(); i++) {
    H225_InfoRequestResponse_perCallInfo_subtype & perCallInfo = info.irr.m_perCallInfo[i];

    // Some systems don't say what direction the call is so find it in the
    // list regardless of the direction
    H323GatekeeperCall::Direction callDirection;
    if (!perCallInfo.HasOptionalField(H225_InfoRequestResponse_perCallInfo_subtype::e_originator))
      callDirection = H323GatekeeperCall::UnknownDirection;
    else if (perCallInfo.m_originator)
      callDirection = H323GatekeeperCall::OriginatingCall;
    else
      callDirection = H323GatekeeperCall::AnsweringCall;

    H323GatekeeperCall search(gatekeeper, perCallInfo.m_callIdentifier.m_guid, callDirection);

    PINDEX idx = activeCalls.GetValuesIndex(search);
    if (idx != P_MAX_INDEX) {
      activeCalls[idx].OnInfoResponse(info, perCallInfo);

      if (callDirection == H323GatekeeperCall::UnknownDirection) {
        // There could be two call entries (originator or destination) and the
        // ep did not say which. GetValuesIndex() always give the first one so
        // see if the next one is also a match
        if (idx < activeCalls.GetSize()-1 && activeCalls[idx+1] == search)
          activeCalls[idx+1].OnInfoResponse(info, perCallInfo);
      }
    }
    else {
      PTRACE(2, "RAS\tEndpoint has call-id gatekeeper does not know about: " << search);
    }
  }

  UnlockReadOnly();

  return H323GatekeeperRequest::Confirm;
}


BOOL H323RegisteredEndPoint::OnTimeToLive()
{
  {
    PSafePtr<H323RegisteredEndPoint> lock(this, PSafeReadOnly);

    if (CheckTimeSince(lastRegistration, timeToLive) ||
        CheckTimeSince(lastInfoResponse, timeToLive))
      return TRUE;

    // Can't do IRQ as have no RAS channel to use (probably logic error)
    if (rasChannel == NULL) {
      PAssertAlways("Timeout on time to live for endpoint we did not receive RRQ for!");
      return FALSE;
    }

  // Exit here unlocks the endpoint
  }

  // Do IRQ and wait for IRR on call
  PTRACE(2, "RAS\tTime to live, doing IRQ for endpoint "<< *this);
  if (!rasChannel->InfoRequest(*this))
    return FALSE;

  PSafePtr<H323RegisteredEndPoint> lock(this, PSafeReadOnly);

  // Return TRUE if got a resonse, ie client does not do unsolicited IRR's
  // otherwise did not get a response from client so return FALSE and
  // (probably) disengage the call.
  return CheckTimeSince(lastInfoResponse, timeToLive);
}


PString H323RegisteredEndPoint::GetCallCreditAmount() const
{
  return PString::Empty();
}


BOOL H323RegisteredEndPoint::GetCallCreditMode() const
{
  return TRUE;
}


BOOL H323RegisteredEndPoint::SendServiceControlSession(const H323ServiceControlSession & session)
{
  // Send SCI to endpoint(s)
  if (rasChannel == NULL) {
    // Can't do SCI as have no RAS channel to use (probably logic error)
    PAssertAlways("Tried to do SCI to endpoint we did not receive RRQ for!");
    return FALSE;
  }

  return rasChannel->ServiceControlIndication(*this, session);
}


BOOL H323RegisteredEndPoint::AddServiceControlSession(const H323ServiceControlSession & session,
                                                      H225_ArrayOf_ServiceControlSession & serviceControl)
{
  if (!session.IsValid())
    return FALSE;

  PString type = session.GetServiceControlType();

  H225_ServiceControlSession_reason::Choices reason = H225_ServiceControlSession_reason::e_refresh;
  if (!serviceControlSessions.Contains(type)) {
    PINDEX id = 0;
    PINDEX i = 0;
    while (i < serviceControlSessions.GetSize()) {
      if (id != serviceControlSessions.GetDataAt(i))
        i++;
      else {
        if (++id >= 256)
          return FALSE;
        i = 0;
      }
    }
    serviceControlSessions.SetAt(type, id);
    reason = H225_ServiceControlSession_reason::e_open;
  }

  PINDEX last = serviceControl.GetSize();
  serviceControl.SetSize(last+1);
  H225_ServiceControlSession & pdu = serviceControl[last];

  pdu.m_sessionId = serviceControlSessions[type];
  pdu.m_reason = reason;

  if (session.OnSendingPDU(pdu.m_contents))
    pdu.IncludeOptionalField(H225_ServiceControlSession::e_contents);

  return TRUE;
}



/////////////////////////////////////////////////////////////////////////////

H323GatekeeperListener::H323GatekeeperListener(H323EndPoint & ep,
                                               H323GatekeeperServer & gk,
                                               const PString & id,
                                               H323Transport * trans)
  : H225_RAS(ep, trans),
    gatekeeper(gk)
{
  gatekeeperIdentifier = id;

  transport->SetPromiscuous(H323Transport::AcceptFromAny);

  PTRACE(2, "H323gk\tGatekeeper server created.");
}


H323GatekeeperListener::~H323GatekeeperListener()
{
  PTRACE(2, "H323gk\tGatekeeper server destroyed.");
}


H323GatekeeperRequest::Response H323GatekeeperListener::OnDiscovery(H323GatekeeperGRQ & info)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnDiscovery");

  if (info.grq.m_protocolIdentifier.GetSize() != 6 || info.grq.m_protocolIdentifier[5] < 2) {
    info.SetRejectReason(H225_GatekeeperRejectReason::e_invalidRevision);
    PTRACE(2, "RAS\tGRQ rejected, version 1 not supported");
    return H323GatekeeperRequest::Reject;
  }

  if (!info.CheckGatekeeperIdentifier())
    return H323GatekeeperRequest::Reject;

  H323TransportAddress address = transport->GetLocalAddress();
  PTRACE(3, "RAS\tGRQ accepted on " << address);
  address.SetPDU(info.gcf.m_rasAddress);

  return gatekeeper.OnDiscovery(info);
}


BOOL H323GatekeeperListener::OnReceiveGatekeeperRequest(const H323RasPDU & pdu,
                                                        const H225_GatekeeperRequest & /*grq*/)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnReceiveGatekeeperRequest");

  H323GatekeeperGRQ * info = new H323GatekeeperGRQ(*this, pdu);
  if (!info->HandlePDU())
    delete info;

  return FALSE;
}


H323GatekeeperRequest::Response H323GatekeeperListener::OnRegistration(H323GatekeeperRRQ & info)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnRegistration");

  if (info.rrq.HasOptionalField(H225_RegistrationRequest::e_endpointIdentifier))
    info.endpoint = gatekeeper.FindEndPointByIdentifier(info.rrq.m_endpointIdentifier);

  if (!info.CheckGatekeeperIdentifier())
    return H323GatekeeperRequest::Reject;

  if (info.rrq.m_protocolIdentifier.GetSize() != 6 || info.rrq.m_protocolIdentifier[5] < 2) {
    info.SetRejectReason(H225_RegistrationRejectReason::e_invalidRevision);
    PTRACE(2, "RAS\tRRQ rejected, version 1 not supported");
    return H323GatekeeperRequest::Reject;
  }

  H323GatekeeperRequest::Response response = gatekeeper.OnRegistration(info);
  if (response != H323GatekeeperRequest::Confirm)
    return response;

  // Adjust the authenticator remote ID to endpoint ID
  if (!info.rrq.m_keepAlive) {
    PSafePtr<H323RegisteredEndPoint> lock(info.endpoint, PSafeReadWrite);
    H235Authenticators authenticators = info.endpoint->GetAuthenticators();
    for (PINDEX i = 0; i < authenticators.GetSize(); i++) {
      H235Authenticator & authenticator = authenticators[i];
      if (authenticator.UseGkAndEpIdentifiers()) {
        authenticator.SetRemoteId(info.endpoint->GetIdentifier());
        authenticator.SetLocalId(gatekeeperIdentifier);
      }
    }
  }

  return H323GatekeeperRequest::Confirm;
}


BOOL H323GatekeeperListener::OnReceiveRegistrationRequest(const H323RasPDU & pdu,
                                                          const H225_RegistrationRequest & /*rrq*/)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnReceiveRegistrationRequest");

  H323GatekeeperRRQ * info = new H323GatekeeperRRQ(*this, pdu);
  if (!info->HandlePDU())
    delete info;

  return FALSE;
}


H323GatekeeperRequest::Response H323GatekeeperListener::OnUnregistration(H323GatekeeperURQ & info)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnUnregistration");

  if (info.urq.HasOptionalField(H225_UnregistrationRequest::e_endpointIdentifier))
    info.endpoint = gatekeeper.FindEndPointByIdentifier(info.urq.m_endpointIdentifier);
  else
    info.endpoint = gatekeeper.FindEndPointBySignalAddresses(info.urq.m_callSignalAddress);

  if (info.endpoint == NULL) {
    info.SetRejectReason(H225_UnregRejectReason::e_notCurrentlyRegistered);
    PTRACE(2, "RAS\tURQ rejected, not registered");
    return H323GatekeeperRequest::Reject;
  }

  return gatekeeper.OnUnregistration(info);
}


BOOL H323GatekeeperListener::OnReceiveUnregistrationRequest(const H323RasPDU & pdu,
                                                            const H225_UnregistrationRequest & /*urq*/)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnReceiveUnregistrationRequest");

  H323GatekeeperURQ * info = new H323GatekeeperURQ(*this, pdu);
  if (!info->HandlePDU())
    delete info;

  return FALSE;
}


BOOL H323GatekeeperListener::OnReceiveUnregistrationConfirm(const H225_UnregistrationConfirm & pdu)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnReceiveUnregistrationConfirm");

  if (!H225_RAS::OnReceiveUnregistrationConfirm(pdu))
    return FALSE;

  return TRUE;
}


BOOL H323GatekeeperListener::OnReceiveUnregistrationReject(const H225_UnregistrationReject & pdu)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnReceiveUnregistrationReject");

  if (!H225_RAS::OnReceiveUnregistrationReject(pdu))
    return FALSE;

  return TRUE;
}


H323GatekeeperRequest::Response H323GatekeeperListener::OnAdmission(H323GatekeeperARQ & info)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnAdmission");

  if (!info.CheckGatekeeperIdentifier())
    return H323GatekeeperRequest::Reject;

  if (!info.GetRegisteredEndPoint())
    return H323GatekeeperRequest::Reject;

  if (!info.CheckCryptoTokens()) {
    PString remoteId, localId, password;
    if (!gatekeeper.GetAdmissionRequestAuthentication(info, remoteId, localId, password))
      return H323GatekeeperRequest::Reject;

    PTRACE(3, "RAS\tARQ received with separate credentials.");
    if (!info.CheckCryptoTokens(info.endpoint->GetAuthenticators().Adjust(remoteId, localId, password)))
      return H323GatekeeperRequest::Reject;

    info.alternateSecurityID = remoteId;
  }

  H323GatekeeperRequest::Response response = gatekeeper.OnAdmission(info);
  if (response != H323GatekeeperRequest::Confirm)
    return response;

  if (info.acf.m_callModel.GetTag() == H225_CallModel::e_gatekeeperRouted) {
    H225_ArrayOf_TransportAddress addresses;
    if (SetUpCallSignalAddresses(addresses))
      info.acf.m_destCallSignalAddress = addresses[0];
  }

  return H323GatekeeperRequest::Confirm;
}


BOOL H323GatekeeperListener::OnReceiveAdmissionRequest(const H323RasPDU & pdu,
                                                       const H225_AdmissionRequest & /*arq*/)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnReceiveAdmissionRequest");

  H323GatekeeperARQ * info = new H323GatekeeperARQ(*this, pdu);
  if (!info->HandlePDU())
    delete info;

  return FALSE;
}


BOOL H323GatekeeperListener::UnregistrationRequest(const H323RegisteredEndPoint & ep,
                                                   unsigned reason)
{
  H323RasPDU pdu(ep.GetAuthenticators());
  H225_UnregistrationRequest & urq = pdu.BuildUnregistrationRequest(GetNextSequenceNumber());

  urq.m_callSignalAddress.SetSize(ep.GetSignalAddressCount());
  for (PINDEX i = 0; i < ep.GetSignalAddressCount(); i++)
    ep.GetSignalAddress(i).SetPDU(urq.m_callSignalAddress[i]);

  urq.IncludeOptionalField(H225_UnregistrationRequest::e_endpointIdentifier);
  urq.m_endpointIdentifier = ep.GetIdentifier();
  urq.m_reason.SetTag(reason);

  Request request(urq.m_requestSeqNum, pdu, ep.GetRASAddresses());
  return MakeRequest(request);
}


BOOL H323GatekeeperListener::DisengageRequest(const H323GatekeeperCall & call,
                                              unsigned reason)
{
  H323RegisteredEndPoint & ep = call.GetEndPoint();

  H323RasPDU pdu(ep.GetAuthenticators());
  H225_DisengageRequest & drq = pdu.BuildDisengageRequest(GetNextSequenceNumber());

  drq.m_endpointIdentifier = ep.GetIdentifier();
  drq.m_conferenceID = call.GetConferenceIdentifier();
  drq.m_callReferenceValue = call.GetCallReference();
  drq.m_callIdentifier.m_guid = call.GetCallIdentifier();
  drq.m_disengageReason.SetTag(reason);
  drq.m_answeredCall = call.IsAnsweringCall();

  if (call.AddCallCreditServiceControl(drq.m_serviceControl))
    drq.IncludeOptionalField(H225_DisengageRequest::e_serviceControl);

  Request request(drq.m_requestSeqNum, pdu, ep.GetRASAddresses());
  return MakeRequest(request);
}


H323GatekeeperRequest::Response H323GatekeeperListener::OnDisengage(H323GatekeeperDRQ & info)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnDisengage");

  if (!info.CheckGatekeeperIdentifier())
    return H323GatekeeperRequest::Reject;

  if (!info.GetRegisteredEndPoint())
    return H323GatekeeperRequest::Reject;

  if (!info.CheckCryptoTokens())
    return H323GatekeeperRequest::Reject;

  return gatekeeper.OnDisengage(info);
}


BOOL H323GatekeeperListener::OnReceiveDisengageRequest(const H323RasPDU & pdu,
                                                       const H225_DisengageRequest & /*drq*/)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnReceiveDisengageRequest");

  H323GatekeeperDRQ * info = new H323GatekeeperDRQ(*this, pdu);
  if (!info->HandlePDU())
    delete info;

  return FALSE;
}


BOOL H323GatekeeperListener::OnReceiveDisengageConfirm(const H225_DisengageConfirm & pdu)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnReceiveDisengageConfirm");

  if (!H225_RAS::OnReceiveDisengageConfirm(pdu))
    return FALSE;

  return TRUE;
}


BOOL H323GatekeeperListener::OnReceiveDisengageReject(const H225_DisengageReject & drj)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnReceiveDisengageReject");

  if (!H225_RAS::OnReceiveDisengageReject(drj))
    return FALSE;

  return TRUE;
}


H323GatekeeperRequest::Response H323GatekeeperListener::OnBandwidth(H323GatekeeperBRQ & info)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnBandwidth");

  if (!info.CheckGatekeeperIdentifier())
    return H323GatekeeperRequest::Reject;

  if (!info.GetRegisteredEndPoint())
    return H323GatekeeperRequest::Reject;

  if (!info.CheckCryptoTokens())
    return H323GatekeeperRequest::Reject;

  return gatekeeper.OnBandwidth(info);
}


BOOL H323GatekeeperListener::OnReceiveBandwidthRequest(const H323RasPDU & pdu,
                                                       const H225_BandwidthRequest & /*brq*/)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnReceiveBandwidthRequest");

  H323GatekeeperBRQ * info = new H323GatekeeperBRQ(*this, pdu);
  if (!info->HandlePDU())
    delete info;

  return FALSE;
}


BOOL H323GatekeeperListener::OnReceiveBandwidthConfirm(const H225_BandwidthConfirm & pdu)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnReceiveBandwidthConfirm");

  if (!H225_RAS::OnReceiveBandwidthConfirm(pdu))
    return FALSE;

  return TRUE;
}


BOOL H323GatekeeperListener::OnReceiveBandwidthReject(const H225_BandwidthReject & pdu)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnReceiveBandwidthReject");

  if (!H225_RAS::OnReceiveBandwidthReject(pdu))
    return FALSE;

  return TRUE;
}


H323GatekeeperRequest::Response H323GatekeeperListener::OnLocation(H323GatekeeperLRQ & info)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnLocation");

  if (!info.CheckGatekeeperIdentifier())
    return H323GatekeeperRequest::Reject;

  if (info.lrq.HasOptionalField(H225_LocationRequest::e_endpointIdentifier)) {
    if (!info.GetRegisteredEndPoint())
      return H323GatekeeperRequest::Reject;
    if (!info.CheckCryptoTokens())
      return H323GatekeeperRequest::Reject;
  }

  return gatekeeper.OnLocation(info);
}


BOOL H323GatekeeperListener::OnReceiveLocationRequest(const H323RasPDU & pdu,
                                                      const H225_LocationRequest & /*lrq*/)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnReceiveLocationRequest");

  H323GatekeeperLRQ * info = new H323GatekeeperLRQ(*this, pdu);
  if (!info->HandlePDU())
    delete info;

  return FALSE;
}


BOOL H323GatekeeperListener::InfoRequest(H323RegisteredEndPoint & ep,
                                         H323GatekeeperCall * call)
{
  unsigned callReference = 0;
  const OpalGloballyUniqueID * callIdentifier = NULL;
  if (call != NULL) {
    callReference = call->GetCallReference();
    callIdentifier = &call->GetCallIdentifier();
  }

  // As sequence number 1 is used for backward compatibility on unsolicited
  // IRR's we make sure we never makea  solicited IRQ using that number.
  unsigned seqnum = GetNextSequenceNumber();
  if (seqnum == 1)
    seqnum = GetNextSequenceNumber();

  H323RasPDU pdu(ep.GetAuthenticators());
  H225_InfoRequest & irq = pdu.BuildInfoRequest(seqnum, callReference, callIdentifier);

  H225_RAS::Request request(irq.m_requestSeqNum, pdu, ep.GetRASAddresses());
  return MakeRequest(request);
}


BOOL H323GatekeeperListener::ServiceControlIndication(H323RegisteredEndPoint & ep,
                                                      const H323ServiceControlSession & session,
                                                      H323GatekeeperCall * call)
{
  OpalGloballyUniqueID id = NULL;
  if (call != NULL)
    id = call->GetCallIdentifier();

  H323RasPDU pdu(ep.GetAuthenticators());
  H225_ServiceControlIndication & sci = pdu.BuildServiceControlIndication(GetNextSequenceNumber(), &id);

  ep.AddServiceControlSession(session, sci.m_serviceControl);

  H225_RAS::Request request(sci.m_requestSeqNum, pdu, ep.GetRASAddresses());
  return MakeRequest(request);
}


H323GatekeeperRequest::Response H323GatekeeperListener::OnInfoResponse(H323GatekeeperIRR & info)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnInfoResponse");

  H323GatekeeperRequest::Response response;
  if (info.GetRegisteredEndPoint() && info.CheckCryptoTokens())
    response = gatekeeper.OnInfoResponse(info);
  else
    response = H323GatekeeperRequest::Reject;

  if (info.irr.m_unsolicited)
    return response;

  return H323GatekeeperRequest::Ignore;
}


BOOL H323GatekeeperListener::OnReceiveInfoRequestResponse(const H323RasPDU & pdu,
                                                          const H225_InfoRequestResponse & irr)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnReceiveInfoRequestResponse");

  BOOL unsolicited = irr.m_unsolicited;

  if (!unsolicited) {
    // Got an IRR that is not marked as unsolicited but has sequence number of
    // one, thus according to 7.15.2/H.225.0 it actually IS unsolicited.
    if (irr.m_requestSeqNum == 1)
      unsolicited = TRUE;
    else {
      if (!H225_RAS::OnReceiveInfoRequestResponse(pdu, irr))
        return FALSE;
    }
  }
  else {
    if (SendCachedResponse(pdu))
      return FALSE;
  }

  H323GatekeeperIRR * info = new H323GatekeeperIRR(*this, pdu);

  info->irr.m_unsolicited = unsolicited;

  if (!info->HandlePDU())
    delete info;

  return !unsolicited;
}


BOOL H323GatekeeperListener::OnReceiveResourcesAvailableConfirm(const H225_ResourcesAvailableConfirm & pdu)
{
  PTRACE_BLOCK("H323GatekeeperListener::OnReceiveResourcesAvailableConfirm");

  if (!H225_RAS::OnReceiveResourcesAvailableConfirm(pdu))
    return FALSE;

  return TRUE;
}


/////////////////////////////////////////////////////////////////////////////

H323GatekeeperServer::H323GatekeeperServer(H323EndPoint & ep)
  : ownerEndPoint(ep)
{
  totalBandwidth = UINT_MAX;      // Unlimited total bandwidth
  usedBandwidth = 0;              // None used so far
  defaultBandwidth = 2560;        // Enough for bidirectional G.711 and 64k H.261
  maximumBandwidth = 200000;      // 10baseX LAN bandwidth
  defaultTimeToLive = 3600;       // One hour, zero disables
  defaultInfoResponseRate = 60;   // One minute, zero disables
  overwriteOnSameSignalAddress = TRUE;
  canOnlyCallRegisteredEP = FALSE;
  canOnlyAnswerRegisteredEP = FALSE;
  answerCallPreGrantedARQ = FALSE;
  makeCallPreGrantedARQ = FALSE;
  isGatekeeperRouted = FALSE;
  aliasCanBeHostName = TRUE;
  requireH235 = FALSE;
  disengageOnHearbeatFail = TRUE;

  identifierBase = time(NULL);
  nextIdentifier = 1;

  peakRegistrations = 0;
  totalRegistrations = 0;
  peakCalls = 0;
  totalCalls = 0;

  monitorThread = PThread::Create(PCREATE_NOTIFIER(MonitorMain), 0,
                                  PThread::NoAutoDeleteThread,
                                  PThread::NormalPriority,
                                  "GkSrv Monitor");
}


H323GatekeeperServer::~H323GatekeeperServer()
{
  monitorExit.Signal();
  PAssert(monitorThread->WaitForTermination(10000), "Gatekeeper monitor thread did not terminate!");
  delete monitorThread;
}


BOOL H323GatekeeperServer::AddListeners(const H323TransportAddressArray & ifaces)
{
  if (ifaces.IsEmpty())
    return AddListener("udp$*");

  PINDEX i;

  mutex.Wait();
  for (i = 0; i < listeners.GetSize(); i++) {
    BOOL remove = TRUE;
    for (PINDEX j = 0; j < ifaces.GetSize(); j++) {
      if (listeners[i].GetTransport().GetLocalAddress().IsEquivalent(ifaces[j])) {
        remove = FALSE;
        break;
      }
    }
    if (remove) {
      PTRACE(3, "RAS\tRemoving listener " << listeners[i]);
      listeners.RemoveAt(i--);
    }
  }
  mutex.Signal();

  for (i = 0; i < ifaces.GetSize(); i++) {
    if (!ifaces[i])
      AddListener(ifaces[i]);
  }

  return listeners.GetSize() > 0;
}


BOOL H323GatekeeperServer::AddListener(const H323TransportAddress & interfaceName)
{
  PWaitAndSignal wait(mutex);

  PINDEX i;
  for (i = 0; i < listeners.GetSize(); i++) {
    if (listeners[i].GetTransport().GetLocalAddress().IsEquivalent(interfaceName)) {
      PTRACE(2, "H323\tAlready have listener for " << interfaceName);
      return TRUE;
    }
  }

  PIPSocket::Address addr;
  WORD port = H225_RAS::DefaultRasUdpPort;
  if (!interfaceName.GetIpAndPort(addr, port))
    return AddListener(interfaceName.CreateTransport(ownerEndPoint, H323TransportAddress::FullTSAP));

  if (addr != INADDR_ANY)
    return AddListener(new H323TransportUDP(ownerEndPoint, addr, port));

  PIPSocket::InterfaceTable interfaces;
  if (!PIPSocket::GetInterfaceTable(interfaces)) {
    PTRACE(1, "RAS\tNo interfaces on system!");
    if (!PIPSocket::GetHostAddress(addr))
      return FALSE;
    return AddListener(new H323TransportUDP(ownerEndPoint, addr, port));
  }

  PTRACE(4, "RAS\tAdding interfaces:\n" << setfill('\n') << interfaces << setfill(' '));

  BOOL atLeastOne = FALSE;

  for (i = 0; i < interfaces.GetSize(); i++) {
    addr = interfaces[i].GetAddress();
    if (addr != 0) {
      if (AddListener(new H323TransportUDP(ownerEndPoint, addr, port)))
        atLeastOne = TRUE;
    }
  }

  return atLeastOne;
}


BOOL H323GatekeeperServer::AddListener(H323Transport * transport)
{
  if (transport == NULL)
    return FALSE;

  if (!transport->IsOpen()) {
    delete transport;
    return FALSE;
  }

  return AddListener(CreateListener(transport));
}


BOOL H323GatekeeperServer::AddListener(H323GatekeeperListener * listener)
{
  if (listener == NULL)
    return FALSE;

  PTRACE(3, "H323\tStarted listener " << *listener);

  mutex.Wait();
  listeners.Append(listener);
  mutex.Signal();

  listener->StartRasChannel();

  return TRUE;
}


H323GatekeeperListener * H323GatekeeperServer::CreateListener(H323Transport * transport)
{
  return new H323GatekeeperListener(ownerEndPoint, *this, gatekeeperIdentifier, transport);
}


BOOL H323GatekeeperServer::RemoveListener(H323GatekeeperListener * listener)
{
  BOOL ok = TRUE;

  mutex.Wait();
  if (listener != NULL) {
    PTRACE(3, "RAS\tRemoving listener " << *listener);
    ok = listeners.Remove(listener);
  }
  else {
    PTRACE(3, "RAS\tRemoving all listeners");
    listeners.RemoveAll();
  }
  mutex.Signal();

  return ok;
}


H323GatekeeperRequest::Response H323GatekeeperServer::OnDiscovery(H323GatekeeperGRQ & info)
{
  PTRACE_BLOCK("H323GatekeeperServer::OnDiscovery");

  H235Authenticators authenticators = ownerEndPoint.CreateAuthenticators();
  for (PINDEX auth = 0; auth < authenticators.GetSize(); auth++) {
    for (PINDEX cap = 0; cap < info.grq.m_authenticationCapability.GetSize(); cap++) {
      for (PINDEX alg = 0; alg < info.grq.m_algorithmOIDs.GetSize(); alg++) {
        if (authenticators[auth].IsCapability(info.grq.m_authenticationCapability[cap],
                                              info.grq.m_algorithmOIDs[alg])) {
          PTRACE(3, "RAS\tGRQ accepted on " << H323TransportAddress(info.gcf.m_rasAddress, "udp")
                 << " using authenticator " << authenticators[auth]);
          info.gcf.IncludeOptionalField(H225_GatekeeperConfirm::e_authenticationMode);
          info.gcf.m_authenticationMode = info.grq.m_authenticationCapability[cap];
          info.gcf.IncludeOptionalField(H225_GatekeeperConfirm::e_algorithmOID);
          info.gcf.m_algorithmOID = info.grq.m_algorithmOIDs[alg];
          return H323GatekeeperRequest::Confirm;
        }
      }
    }
  }

  if (requireH235) {
    info.SetRejectReason(H225_GatekeeperRejectReason::e_securityDenial);
    return H323GatekeeperRequest::Reject;
  }
  else {
    PTRACE(3, "RAS\tGRQ accepted on " << H323TransportAddress(info.gcf.m_rasAddress, "udp"));
    return H323GatekeeperRequest::Confirm;
  }
}


H323GatekeeperRequest::Response H323GatekeeperServer::OnRegistration(H323GatekeeperRRQ & info)
{
  PTRACE_BLOCK("H323GatekeeperServer::OnRegistration");

  PINDEX i;

  // Initialise reply with default stuff
  info.rcf.IncludeOptionalField(H225_RegistrationConfirm::e_preGrantedARQ);
  info.rcf.m_preGrantedARQ.m_answerCall = answerCallPreGrantedARQ;
  info.rcf.m_preGrantedARQ.m_useGKCallSignalAddressToAnswer = answerCallPreGrantedARQ && isGatekeeperRouted;
  info.rcf.m_preGrantedARQ.m_makeCall = makeCallPreGrantedARQ;
  info.rcf.m_preGrantedARQ.m_useGKCallSignalAddressToMakeCall = makeCallPreGrantedARQ && isGatekeeperRouted;
  info.rcf.m_willRespondToIRR = TRUE;

  if (defaultInfoResponseRate > 0 && info.rrq.m_protocolIdentifier[5] > 2) {
    info.rcf.m_preGrantedARQ.IncludeOptionalField(H225_RegistrationConfirm_preGrantedARQ::e_irrFrequencyInCall);
    info.rcf.m_preGrantedARQ.m_irrFrequencyInCall = defaultInfoResponseRate;
  }

  if (info.rrq.m_keepAlive) {
    if (info.endpoint != NULL)
      return info.endpoint->OnRegistration(info);

    info.SetRejectReason(H225_RegistrationRejectReason::e_fullRegistrationRequired);
    PTRACE(2, "RAS\tRRQ keep alive rejected, not registered");
    return H323GatekeeperRequest::Reject;
  }

  for (i = 0; i < info.rrq.m_callSignalAddress.GetSize(); i++) {
    PSafePtr<H323RegisteredEndPoint> ep2 = FindEndPointBySignalAddress(info.rrq.m_callSignalAddress[i]);
    if (ep2 != NULL && ep2 != info.endpoint) {
      if (overwriteOnSameSignalAddress) {
        PTRACE(2, "RAS\tOverwriting existing endpoint " << *ep2);
        RemoveEndPoint(ep2);
      }
      else {
        info.SetRejectReason(H225_RegistrationRejectReason::e_invalidCallSignalAddress);
        PTRACE(2, "RAS\tRRQ rejected, duplicate callSignalAddress");
        return H323GatekeeperRequest::Reject;
      }
    }
  }

  if (info.rrq.HasOptionalField(H225_RegistrationRequest::e_terminalAlias)) {
    H225_ArrayOf_AliasAddress duplicateAliases;
    for (i = 0; i < info.rrq.m_terminalAlias.GetSize(); i++) {
      PSafePtr<H323RegisteredEndPoint> ep2 = FindEndPointByAliasAddress(info.rrq.m_terminalAlias[i]);
      if (ep2 != NULL && ep2 != info.endpoint) {
        PINDEX sz = duplicateAliases.GetSize();
        duplicateAliases.SetSize(sz+1);
	duplicateAliases[sz] = info.rrq.m_terminalAlias[i];
      }
    }
    if (duplicateAliases.GetSize() > 0) {
      info.SetRejectReason(H225_RegistrationRejectReason::e_duplicateAlias);
      H225_ArrayOf_AliasAddress & reasonAliases = info.rrj.m_rejectReason;
      reasonAliases = duplicateAliases;
      PTRACE(2, "RAS\tRRQ rejected, duplicate alias");
      return H323GatekeeperRequest::Reject;
    }
  }

  // Check if the endpoint is trying to register a prefix that can be resolved to another endpoint
  const H225_EndpointType & terminalType = info.rrq.m_terminalType;
  if (terminalType.HasOptionalField(H225_EndpointType::e_gateway) &&
      terminalType.m_gateway.HasOptionalField(H225_GatewayInfo::e_protocol)) {
    const H225_ArrayOf_SupportedProtocols & protocols = terminalType.m_gateway.m_protocol;
    for (i = 0; i < protocols.GetSize(); i++) {

      // Only voice prefixes are supported
      if (protocols[i].GetTag() == H225_SupportedProtocols::e_voice) {
	H225_VoiceCaps & voiceCaps = protocols[i];
	if (voiceCaps.HasOptionalField(H225_VoiceCaps::e_supportedPrefixes)) {
	  H225_ArrayOf_SupportedPrefix & prefixes = voiceCaps.m_supportedPrefixes;
	  for (PINDEX j = 0; j < prefixes.GetSize(); j++) {

	    // Reject if the prefix be matched to a registered alias or prefix
	    PSafePtr<H323RegisteredEndPoint> ep2 = FindEndPointByAliasAddress(prefixes[j].m_prefix);
	    if (ep2 != NULL && ep2 != info.endpoint) {
	      info.SetRejectReason(H225_RegistrationRejectReason::e_duplicateAlias);
              H225_ArrayOf_AliasAddress & aliases = info.rrj.m_rejectReason;
              aliases.SetSize(1);
	      aliases[0] = prefixes[j].m_prefix;
	      PTRACE(2, "RAS\tRRQ rejected, duplicate prefix");
	      return H323GatekeeperRequest::Reject;
	    }
	  }
	}
	break;  // If voice protocol is found, don't look any further
      }
    }
  }

  // Are already registered and have just sent another heavy RRQ
  if (info.endpoint != NULL) {
    H323GatekeeperRequest::Response response = info.endpoint->OnRegistration(info);
    switch (response) {
      case H323GatekeeperRequest::Confirm :
        AddEndPoint(info.endpoint);
        break;
      case H323GatekeeperRequest::Reject :
        RemoveEndPoint(info.endpoint);
        break;
      default :
        break;
    }
    return response;
  }

  // Need to create a new endpoint object
  info.endpoint = CreateRegisteredEndPoint(info);
  if (info.endpoint == (H323RegisteredEndPoint *)NULL) {
    PTRACE(1, "RAS\tRRQ rejected, CreateRegisteredEndPoint() returned NULL");
    return H323GatekeeperRequest::Reject;
  }

  H323GatekeeperRequest::Response response = info.endpoint->OnRegistration(info);
  if (response != H323GatekeeperRequest::Confirm) {
    info.endpoint = (H323RegisteredEndPoint *)NULL;
    delete info.endpoint;
    return response;
  }

  // Have successfully registered, save it
  AddEndPoint(info.endpoint);

  PTRACE(2, "RAS\tRRQ accepted: \"" << *info.endpoint << '"');
  return H323GatekeeperRequest::Confirm;
}


H323GatekeeperRequest::Response H323GatekeeperServer::OnUnregistration(H323GatekeeperURQ & info)
{
  PTRACE_BLOCK("H323GatekeeperServer::OnUnregistration");

  H323GatekeeperRequest::Response response = info.endpoint->OnUnregistration(info);
  if (response != H323GatekeeperRequest::Confirm)
    return response;

  if (info.urq.HasOptionalField(H225_UnregistrationRequest::e_endpointAlias)) {
    PINDEX i;
    // See if all aliases to be removed are on the same endpoint
    for (i = 0; i < info.urq.m_endpointAlias.GetSize(); i++) {
      if (FindEndPointByAliasAddress(info.urq.m_endpointAlias[i]) != info.endpoint) {
        info.SetRejectReason(H225_UnregRejectReason::e_permissionDenied);
        PTRACE(2, "RAS\tURQ rejected, alias " << info.urq.m_endpointAlias[i]
               << " not owned by registration");
        return H323GatekeeperRequest::Reject;
      }
    }
    // Remove all the aliases specified in PDU, if no aliases left, then
    // RemoveAlias() will remove the endpoint
    for (i = 0; i < info.urq.m_endpointAlias.GetSize(); i++) {
      RemoveAlias(H323GetAliasAddressString(info.urq.m_endpointAlias[i]));
    }
  }
  else
    RemoveEndPoint(info.endpoint);

  return H323GatekeeperRequest::Confirm;
}


H323GatekeeperRequest::Response H323GatekeeperServer::OnInfoResponse(H323GatekeeperIRR & info)
{
  PTRACE_BLOCK("H323GatekeeperServer::OnInfoResponse");

  return info.endpoint->OnInfoResponse(info);
}


void H323GatekeeperServer::AddEndPoint(H323RegisteredEndPoint * ep)
{
  PTRACE(3, "RAS\tAdding registered endpoint: " << *ep);

  PINDEX i;

  mutex.Wait();

  if (byIdentifier.FindWithLock(ep->GetIdentifier(), PSafeReference) != ep) {
    byIdentifier.SetAt(ep->GetIdentifier(), ep);

    if (byIdentifier.GetSize() > peakRegistrations)
      peakRegistrations = byIdentifier.GetSize();
    totalRegistrations++;
  }

  for (i = 0; i < ep->GetSignalAddressCount(); i++)
    byAddress.Append(new StringMap(ep->GetSignalAddress(i), ep->GetIdentifier()));

  for (i = 0; i < ep->GetAliasCount(); i++)
    byAlias.Append(new StringMap(ep->GetAlias(i), ep->GetIdentifier()));

  for (i = 0; i < ep->GetPrefixCount(); i++)
    byVoicePrefix.Append(new StringMap(ep->GetPrefix(i), ep->GetIdentifier()));

  mutex.Signal();
}


void H323GatekeeperServer::RemoveEndPoint(H323RegisteredEndPoint * ep)
{
  PTRACE(3, "RAS\tRemoving registered endpoint: " << *ep);

  while (ep->GetCallCount() > 0)
    RemoveCall(&ep->GetCall(0));

  PINDEX i;

  mutex.Wait();

  for (i = 0; i < ep->GetPrefixCount(); i++)
    byVoicePrefix.RemoveAt(byVoicePrefix.GetValuesIndex(ep->GetPrefix(i)));

  for (i = 0; i < ep->GetAliasCount(); i++)
    byAlias.RemoveAt(byAlias.GetValuesIndex(ep->GetAlias(i)));

  for (i = 0; i < ep->GetSignalAddressCount(); i++)
    byAddress.RemoveAt(byAddress.GetValuesIndex(ep->GetSignalAddress(i)));

  byIdentifier.RemoveAt(ep->GetIdentifier());    // ep is deleted by this

  mutex.Signal();
}


BOOL H323GatekeeperServer::RemoveAlias(const PString & alias)
{
  PTRACE(3, "RAS\tRemoving registered endpoint alias: " << alias);

  PString id;

  mutex.Wait();

  PINDEX pos = byAlias.GetValuesIndex(alias);
  if (pos != P_MAX_INDEX) {
    id = ((StringMap &)byAlias[pos]).identifier;
    byAlias.RemoveAt(pos);
  }

  mutex.Signal();

  if (id.IsEmpty()) {
    PTRACE(2, "RAS\tRemoval of unknown alias!");
    return FALSE;
  }

  PSafePtr<H323RegisteredEndPoint> ep = FindEndPointByIdentifier(id, PSafeReadWrite);
  if (ep == NULL) {
    PTRACE(1, "RAS\tRemoval of alias for which there is no endpoint!");
    return FALSE;
  }

  ep->RemoveAlias(alias);
  if (ep->GetAliasCount() != 0)
    return FALSE;

  RemoveEndPoint(ep);
  return TRUE;
}


H323RegisteredEndPoint * H323GatekeeperServer::CreateRegisteredEndPoint(H323GatekeeperRRQ &)
{
  return new H323RegisteredEndPoint(*this, CreateEndPointIdentifier());
}


PString H323GatekeeperServer::CreateEndPointIdentifier()
{
  PWaitAndSignal wait(mutex);
  return psprintf("%x:%u", identifierBase, nextIdentifier++);
}


PSafePtr<H323RegisteredEndPoint> H323GatekeeperServer::FindEndPointByIdentifier(
                                            const PString & identifier, PSafetyMode mode)
{
  return byIdentifier.FindWithLock(identifier, mode);
}


PSafePtr<H323RegisteredEndPoint> H323GatekeeperServer::FindEndPointBySignalAddresses(
                            const H225_ArrayOf_TransportAddress & addresses, PSafetyMode mode)
{
  PWaitAndSignal wait(mutex);

  for (PINDEX i = 0; i < addresses.GetSize(); i++) {
    PINDEX pos = byAddress.GetValuesIndex(H323TransportAddress(addresses[i]));
    if (pos != P_MAX_INDEX)
      return FindEndPointByIdentifier(((StringMap &)byAddress[pos]).identifier, mode);
  }

  return (H323RegisteredEndPoint *)NULL;
}


PSafePtr<H323RegisteredEndPoint> H323GatekeeperServer::FindEndPointBySignalAddress(
                                     const H323TransportAddress & address, PSafetyMode mode)
{
  PWaitAndSignal wait(mutex);

  PINDEX pos = byAddress.GetValuesIndex(address);
  if (pos != P_MAX_INDEX)
    return FindEndPointByIdentifier(((StringMap &)byAddress[pos]).identifier, mode);

  return (H323RegisteredEndPoint *)NULL;
}


PSafePtr<H323RegisteredEndPoint> H323GatekeeperServer::FindEndPointByAliasAddress(
                                         const H225_AliasAddress & alias, PSafetyMode mode)
{
  return FindEndPointByAliasString(H323GetAliasAddressString(alias), mode);
}


PSafePtr<H323RegisteredEndPoint> H323GatekeeperServer::FindEndPointByAliasString(
                                                  const PString & alias, PSafetyMode mode)
{
  PWaitAndSignal wait(mutex);
  PINDEX pos = byAlias.GetValuesIndex(alias);

  if (pos != P_MAX_INDEX)
    return FindEndPointByIdentifier(((StringMap &)byAlias[pos]).identifier, mode);

  if (byVoicePrefix.IsEmpty())
    return (H323RegisteredEndPoint *)NULL;

  // If alias is not registered, check if there is a prefix that matches any substring of the alias.
  // ( Would it be more efficient to check every registered prefix instead ? )
  for (PINDEX len = alias.GetLength(); len > 0; len--) {
    pos = byVoicePrefix.GetValuesIndex(alias.Left(len));
    if (pos != P_MAX_INDEX)
      return FindEndPointByIdentifier(((StringMap &)byVoicePrefix[pos]).identifier, mode);
  }

  return (H323RegisteredEndPoint *)NULL;
}


PSafePtr<H323RegisteredEndPoint> H323GatekeeperServer::FindEndPointByPartialAlias(
                                                  const PString & alias, PSafetyMode mode)
{
  PWaitAndSignal wait(mutex);
  PINDEX pos = byAlias.GetNextStringsIndex(alias);

  if (pos != P_MAX_INDEX)
    return FindEndPointByIdentifier(((StringMap &)byAlias[pos]).identifier, mode);

  return (H323RegisteredEndPoint *)NULL;
}


H323GatekeeperRequest::Response H323GatekeeperServer::OnAdmission(H323GatekeeperARQ & info)
{
  PTRACE_BLOCK("H323GatekeeperServer::OnAdmission");

  OpalGloballyUniqueID id = info.arq.m_callIdentifier.m_guid;
  if (id == NULL) {
    PTRACE(2, "RAS\tNo call identifier provided in ARQ!");
    info.SetRejectReason(H225_AdmissionRejectReason::e_undefinedReason);
    return H323GatekeeperRequest::Reject;
  }

  H323GatekeeperRequest::Response response;

  PSafePtr<H323GatekeeperCall> oldCall = FindCall(id, info.arq.m_answerCall);
  if (oldCall != NULL)
    response = oldCall->OnAdmission(info);
  else {
    // If on restarted in thread, then don't create new call, should already
    // have had one created on the last pass through.
    if (!info.IsFastResponseRequired()) {
      PTRACE(2, "RAS\tCall object disappeared after starting slow PDU handler thread!");
      info.SetRejectReason(H225_AdmissionRejectReason::e_undefinedReason);
      return H323GatekeeperRequest::Reject;
    }

    H323GatekeeperCall * newCall = CreateCall(id,
                            info.arq.m_answerCall ? H323GatekeeperCall::AnsweringCall
                                                  : H323GatekeeperCall::OriginatingCall);
    PTRACE(3, "RAS\tCall created: " << *newCall);

    response = newCall->OnAdmission(info);

    if (response != H323GatekeeperRequest::Reject) {
      mutex.Wait();

      info.endpoint->AddCall(newCall);
      oldCall = activeCalls.Append(newCall);

      if (activeCalls.GetSize() > peakCalls)
        peakCalls = activeCalls.GetSize();
      totalCalls++;

      PTRACE(2, "RAS\tAdded new call (total=" << activeCalls.GetSize() << ") " << *newCall);
      mutex.Signal();
    }
  }

  switch (response) {
    case H323GatekeeperRequest::Confirm :
      if (oldCall->AddCallCreditServiceControl(info.acf.m_serviceControl))
        info.acf.IncludeOptionalField(H225_AdmissionConfirm::e_serviceControl);
      break;

    case H323GatekeeperRequest::Reject :
      if (oldCall->AddCallCreditServiceControl(info.arj.m_serviceControl))
        info.arj.IncludeOptionalField(H225_AdmissionReject::e_serviceControl);
      break;

    default :
      break;
  }

  return response;
}


H323GatekeeperRequest::Response H323GatekeeperServer::OnDisengage(H323GatekeeperDRQ & info)
{
  PTRACE_BLOCK("H323GatekeeperServer::OnDisengage");

  OpalGloballyUniqueID callIdentifier = info.drq.m_callIdentifier.m_guid;
  PSafePtr<H323GatekeeperCall> call = FindCall(callIdentifier, info.drq.m_answeredCall);
  if (call == NULL) {
    info.SetRejectReason(H225_DisengageRejectReason::e_requestToDropOther);
    PTRACE(2, "RAS\tDRQ rejected, no call with ID " << callIdentifier);
    return H323GatekeeperRequest::Reject;
  }

  H323GatekeeperRequest::Response response = call->OnDisengage(info);
  if (response != H323GatekeeperRequest::Confirm)
    return response;

  RemoveCall(call);
  return H323GatekeeperRequest::Confirm;
}


H323GatekeeperRequest::Response H323GatekeeperServer::OnBandwidth(H323GatekeeperBRQ & info)
{
  PTRACE_BLOCK("H323GatekeeperServer::OnBandwidth");

  PSafePtr<H323GatekeeperCall> call = FindCall(info.brq.m_callIdentifier.m_guid, info.brq.m_answeredCall);
  if (call == NULL) {
    info.SetRejectReason(H225_BandRejectReason::e_invalidConferenceID);
    PTRACE(2, "RAS\tBRQ rejected, no call with ID");
    return H323GatekeeperRequest::Reject;
  }

  H323GatekeeperRequest::Response response = call->OnBandwidth(info);
  return response;
}


BOOL H323GatekeeperServer::GetAdmissionRequestAuthentication(H323GatekeeperARQ & /*info*/,
                                                             PString & /*remoteId*/,
                                                             PString & /*localId*/,
                                                             PString & /*password*/)
{
  return FALSE;
}


BOOL H323GatekeeperServer::GetUsersPassword(const PString & alias,
                                            PString & password) const
{
  password = passwords(alias);
  if (!password)
    return TRUE;

  return !requireH235;
}


unsigned H323GatekeeperServer::AllocateBandwidth(unsigned newBandwidth,
                                                 unsigned oldBandwidth)
{
  PWaitAndSignal wait(mutex);

  // If first request for bandwidth, then only give them a maximum of the
  // configured default bandwidth
  if (oldBandwidth == 0 && newBandwidth > defaultBandwidth)
    newBandwidth = defaultBandwidth;

  // If then are asking for more than we have in total, drop it down to whatevers left
  if (newBandwidth > oldBandwidth && (newBandwidth - oldBandwidth) > (totalBandwidth - usedBandwidth))
    newBandwidth = totalBandwidth - usedBandwidth - oldBandwidth;

  // If greater than the absolute maximum configured for any endpoint, clamp it
  if (newBandwidth > maximumBandwidth)
    newBandwidth = maximumBandwidth;

  // Finally have adjusted new bandwidth, allocate it!
  usedBandwidth += (newBandwidth - oldBandwidth);
  return newBandwidth;
}


void H323GatekeeperServer::RemoveCall(H323GatekeeperCall * call)
{
  PAssertNULL(call);

  AllocateBandwidth(0, call->GetBandwidthUsed());
  PAssert(call->GetEndPoint().RemoveCall(call), PLogicError);

  PTRACE(2, "RAS\tRemoved call (total=" << (activeCalls.GetSize()-1) << ") id=" << *call);
  PAssert(activeCalls.Remove(call), PLogicError);
}


H323GatekeeperCall * H323GatekeeperServer::CreateCall(const OpalGloballyUniqueID & id,
                                                      H323GatekeeperCall::Direction dir)
{
  return new H323GatekeeperCall(*this, id, dir);
}


PSafePtr<H323GatekeeperCall> H323GatekeeperServer::FindCall(const PString & desc,
                                                            PSafetyMode mode)
{
  PINDEX pos = desc.Find(AnswerCallStr);
  if (pos == P_MAX_INDEX)
    pos = desc.Find(OriginateCallStr);

  OpalGloballyUniqueID id = desc.Left(pos);

  H323GatekeeperCall::Direction dir = H323GatekeeperCall::UnknownDirection;

  PString dirStr = desc.Mid(pos);
  if (dirStr == AnswerCallStr)
    dir = H323GatekeeperCall::AnsweringCall;
  else if (dirStr == OriginateCallStr)
    dir = H323GatekeeperCall::OriginatingCall;

  return FindCall(id, dir, mode);
}


PSafePtr<H323GatekeeperCall> H323GatekeeperServer::FindCall(const OpalGloballyUniqueID & id,
                                                            BOOL answer,
                                                            PSafetyMode mode)
{
  return FindCall(id, answer ? H323GatekeeperCall::AnsweringCall
                             : H323GatekeeperCall::OriginatingCall, mode);
}


PSafePtr<H323GatekeeperCall> H323GatekeeperServer::FindCall(const OpalGloballyUniqueID & id,
                                                            H323GatekeeperCall::Direction dir,
                                                            PSafetyMode mode)
{
  return activeCalls.FindWithLock(H323GatekeeperCall(*this, id, dir), mode);
}


H323GatekeeperRequest::Response H323GatekeeperServer::OnLocation(H323GatekeeperLRQ & info)
{
  PINDEX i;
  for (i = 0; i < info.lrq.m_destinationInfo.GetSize(); i++) {
    PSafePtr<H323RegisteredEndPoint> ep = FindEndPointByAliasAddress(info.lrq.m_destinationInfo[i], PSafeReadOnly);
    if (ep != NULL) {
      ep->GetSignalAddress(0).SetPDU(info.lcf.m_callSignalAddress);
      ep->GetRASAddress(0).SetPDU(info.lcf.m_rasAddress);
      PTRACE(2, "RAS\tLocation of " << H323GetAliasAddressString(info.lrq.m_destinationInfo[i])
             << " is endpoint " << *ep);
      return H323GatekeeperRequest::Confirm;
    }
  }

  info.SetRejectReason(H225_LocationRejectReason::e_requestDenied);
  PTRACE(2, "RAS\tLRQ rejected, alias not found");
  return H323GatekeeperRequest::Reject;
}


BOOL H323GatekeeperServer::TranslateAliasAddressToSignalAddress(const H225_AliasAddress & alias,
                                                                H323TransportAddress & address)
{
  PWaitAndSignal wait(mutex);

  PString aliasString = H323GetAliasAddressString(alias);

  if (isGatekeeperRouted) {
    const H323ListenerList & listeners = ownerEndPoint.GetListeners();
    address = listeners[0].GetTransportAddress();
    PTRACE(2, "RAS\tTranslating alias " << aliasString << " to " << address << ", gatekeeper routed");
    return TRUE;
  }

  PSafePtr<H323RegisteredEndPoint> ep = FindEndPointByAliasAddress(alias, PSafeReadOnly);
  if (ep != NULL) {
    address = ep->GetSignalAddress(0);
    PTRACE(2, "RAS\tTranslating alias " << aliasString << " to " << address << ", registered endpoint");
    return TRUE;
  }

  if (aliasCanBeHostName) {
    H323TransportAddress aliasAsTransport = aliasString;
    PIPSocket::Address ip;
    WORD port = H323EndPoint::DefaultTcpPort;
    if (aliasAsTransport.GetIpAndPort(ip, port)) {
      address = H323TransportAddress(ip, port);
      PTRACE(2, "RAS\tTranslating alias " << aliasString << " to " << address << ", host name");
    }
    else {
      PTRACE(4, "RAS\tCould not translate " << aliasString << " as host name.");
    }
  }

  return !address;
}


BOOL H323GatekeeperServer::CheckSignalAddressPolicy(const H323RegisteredEndPoint &,
                                                    const H225_AdmissionRequest &,
                                                    const H323TransportAddress &)
{
  return TRUE;
}


BOOL H323GatekeeperServer::CheckAliasAddressPolicy(const H323RegisteredEndPoint &,
                                                   const H225_AdmissionRequest & arq,
                                                   const H225_AliasAddress & alias)
{
  PWaitAndSignal wait(mutex);

  if (arq.m_answerCall ? canOnlyAnswerRegisteredEP : canOnlyCallRegisteredEP) {
    PSafePtr<H323RegisteredEndPoint> ep = FindEndPointByAliasAddress(alias);
    if (ep == NULL)
      return FALSE;
  }

  return TRUE;
}


BOOL H323GatekeeperServer::CheckAliasStringPolicy(const H323RegisteredEndPoint &,
                                                  const H225_AdmissionRequest & arq,
                                                  const PString & alias)
{
  PWaitAndSignal wait(mutex);

  if (arq.m_answerCall ? canOnlyAnswerRegisteredEP : canOnlyCallRegisteredEP) {
    PSafePtr<H323RegisteredEndPoint> ep = FindEndPointByAliasString(alias);
    if (ep == NULL)
      return FALSE;
  }

  return TRUE;
}


void H323GatekeeperServer::SetGatekeeperIdentifier(const PString & id,
                                                   BOOL adjustListeners)
{
  mutex.Wait();

  gatekeeperIdentifier = id;

  if (adjustListeners) {
    for (PINDEX i = 0; i < listeners.GetSize(); i++)
      listeners[i].SetIdentifier(id);
  }

  mutex.Signal();
}


void H323GatekeeperServer::MonitorMain(PThread &, INT)
{
  while (!monitorExit.Wait(1000)) {
    PTRACE(6, "RAS\tAging registered endpoints");

    for (PSafePtr<H323RegisteredEndPoint> ep = GetFirstEndPoint(PSafeReference); ep != NULL; ep++) {
      if (!ep->OnTimeToLive()) {
        PTRACE(2, "RAS\tRemoving expired endpoint " << *ep);
        RemoveEndPoint(ep);
      }
    }

    byIdentifier.DeleteObjectsToBeRemoved();

    for (PSafePtr<H323GatekeeperCall> call = GetFirstCall(PSafeReference); call != NULL; call++) {
      if (!call->OnHeartbeat()) {
        if (disengageOnHearbeatFail)
          call->Disengage();
      }
    }

    activeCalls.DeleteObjectsToBeRemoved();
  }
}


/////////////////////////////////////////////////////////////////////////////
