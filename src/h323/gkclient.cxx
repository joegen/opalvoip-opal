/*
 * gkclient.cxx
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
 * Portions of this code were written with the assisance of funding from
 * iFace In, http://www.iface.com
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: gkclient.cxx,v $
 * Revision 1.2008  2002/01/14 06:35:57  robertj
 * Updated to OpenH323 v1.7.9
 *
 * Revision 2.6  2001/11/18 23:10:42  craigs
 * Added cast to allow compilation under Linux
 * Revision 1.83  2002/01/13 23:58:48  robertj
 * Added ability to set destination extra call info in ARQ
 * Filled in destinationInfo in ARQ when answering call.
 * Allowed application to override srcInfo in ARQ on outgoing call by
 *   changing localPartyName.
 * Added better end call codes for some ARJ reasons.
 * Thanks Ben Madsen of Norwood Systems.
 *
 * Revision 1.82  2001/12/15 10:10:48  robertj
 * GCC compatibility
 *
 * Revision 1.81  2001/12/15 08:36:49  robertj
 * Added previous call times to all the other PDU's it is supposed to be in!
 *
 * Revision 1.80  2001/12/15 08:09:21  robertj
 * Added alerting, connect and end of call times to be sent to RAS server.
 *
 * Revision 1.79  2001/12/14 06:41:36  robertj
 * Added call end reason codes in DisengageRequest for GK server use.
 *
 * Revision 1.78  2001/12/13 11:00:13  robertj
 * Changed search for access token in ACF to be able to look for two OID's.
 *
 * Revision 1.77  2001/12/06 06:44:42  robertj
 * Removed "Win32 SSL xxx" build configurations in favour of system
 *   environment variables to select optional libraries.
 *
 * Revision 1.76  2001/10/12 04:14:31  robertj
 * Changed gk unregister so only way it doe not actually unregister is if
 *   get URJ with reason code callInProgress, thanks Chris Purvis.
 *
 * Revision 2.5  2001/11/09 05:49:47  robertj
 * Abstracted UDP connection algorithm
 *
 * Revision 2.4  2001/10/05 00:22:13  robertj
 * Updated to PWLib 1.2.0 and OpenH323 1.7.0
 *
 * Revision 2.3  2001/08/17 08:30:21  robertj
 * Update from OpenH323
 * Moved call end reasons enum from OpalConnection to global.
 *
 * Revision 2.2  2001/08/13 05:10:39  robertj
 * Updates from OpenH323 v1.6.0 release.
 *
 * Revision 2.1  2001/08/01 05:16:18  robertj
 * Moved default session ID's to OpalMediaFormat class.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 * Revision 1.73  2001/09/26 07:03:08  robertj
 * Added needed mutex for SeparateAuthenticationInARQ mode, thanks Nick Hoath
 *
 * Revision 1.72  2001/09/18 10:36:57  robertj
 * Allowed multiple overlapping requests in RAS channel.
 *
 * Revision 1.71  2001/09/13 03:21:16  robertj
 * Added ability to override authentication credentials for ARQ, thanks Nick Hoath
 *
 * Revision 1.70  2001/09/13 01:15:20  robertj
 * Added flag to H235Authenticator to determine if gkid and epid is to be
 *   automatically set as the crypto token remote id and local id.
 *
 * Revision 1.69  2001/09/13 00:32:24  robertj
 * Added missing gkid in ARQ, thanks Nick Hoath
 *
 * Revision 1.68  2001/09/12 07:48:05  robertj
 * Fixed various problems with tracing.
 *
 * Revision 1.67  2001/09/12 06:58:00  robertj
 * Added support for iNow Access Token from gk, thanks Nick Hoath
 *
 * Revision 1.66  2001/09/12 06:04:38  robertj
 * Added support for sending UUIE's to gk on request, thanks Nick Hoath
 *
 * Revision 1.65  2001/09/05 01:16:32  robertj
 * Added overloaded AdmissionRequest for backward compatibility.
 *
 * Revision 1.64  2001/08/14 04:26:46  robertj
 * Completed the Cisco compatible MD5 authentications, thanks Wolfgang Platzer.
 *
 * Revision 1.63  2001/08/13 01:27:03  robertj
 * Changed GK admission so can return multiple aliases to be used in
 *   setup packet, thanks Nick Hoath.
 *
 * Revision 1.62  2001/08/13 00:22:14  robertj
 * Allowed for received DRQ not having call ID (eg v1 gk), uses conference ID
 *
 * Revision 1.61  2001/08/10 11:03:52  robertj
 * Major changes to H.235 support in RAS to support server.
 *
 * Revision 1.60  2001/08/06 07:44:55  robertj
 * Fixed problems with building without SSL
 *
 * Revision 1.59  2001/08/06 03:18:38  robertj
 * Fission of h323.h to h323ep.h & h323con.h, h323.h now just includes files.
 * Improved access to H.235 secure RAS functionality.
 * Changes to H.323 secure RAS contexts to help use with gk server.
 *
 * Revision 1.58  2001/08/02 04:30:43  robertj
 * Added ability for AdmissionRequest to alter destination alias used in
 *   the outgoing call. Thanks Ben Madsen & Graeme Reid.
 *
 * Revision 1.57  2001/06/22 00:21:10  robertj
 * Fixed bug in H.225 RAS protocol with 16 versus 32 bit sequence numbers.
 *
 * Revision 1.56  2001/06/18 23:35:01  robertj
 * Removed condition that prevented aliases on non-terminal endpoints.
 *
 * Revision 1.55  2001/06/18 06:23:50  robertj
 * Split raw H.225 RAS protocol out of gatekeeper client class.
 *
 * Revision 1.54  2001/05/17 03:29:13  robertj
 * Fixed missing replyAddress in LRQ, thanks Alexander Smirnov.
 * Added some extra optional fields to LRQ.
 *
 * Revision 1.53  2001/04/19 08:03:21  robertj
 * Fixed scale on RIp delay, is milliseconds!
 *
 * Revision 1.52  2001/04/13 07:44:20  robertj
 * Fixed setting isRegistered flag to false when get RRJ
 *
 * Revision 1.51  2001/04/05 03:39:43  robertj
 * Fixed deadlock if tried to do discovery in time to live timeout.
 *
 * Revision 1.50  2001/03/28 07:13:06  robertj
 * Changed RAS thread interlock to allow for what should not happen, the
 *   syncpoint being signalled before receiving any packets.
 *
 * Revision 1.49  2001/03/27 02:19:22  robertj
 * Changed to send gk a GRQ if it gives a discoveryRequired error on RRQ.
 * Fixed BIG  condition in use of sequence numbers.
 *
 * Revision 1.48  2001/03/26 05:06:03  robertj
 * Added code to do full registration if RRJ indicates discovery to be redone.
 *
 * Revision 1.47  2001/03/24 00:51:41  robertj
 * Added retry every minute of time to live registration if fails.
 *
 * Revision 1.46  2001/03/23 01:47:49  robertj
 * Improved debug trace message on RAS packet retry.
 *
 * Revision 1.45  2001/03/23 01:19:25  robertj
 * Fixed usage of secure RAS in GRQ, should not do for that one PDU.
 *
 * Revision 1.44  2001/03/21 04:52:42  robertj
 * Added H.235 security to gatekeepers, thanks Fürbass Franz!
 *
 * Revision 1.43  2001/03/19 23:32:30  robertj
 * Fixed problem with auto-reregister doing so in the RAS receive thread.
 *
 * Revision 1.42  2001/03/19 05:50:52  robertj
 * Fixed trace display of timeout value.
 *
 * Revision 1.41  2001/03/18 22:21:29  robertj
 * Fixed GNU C++ problem.
 *
 * Revision 1.40  2001/03/17 00:05:52  robertj
 * Fixed problems with Gatekeeper RIP handling.
 *
 * Revision 1.39  2001/03/16 06:46:21  robertj
 * Added ability to set endpoints desired time to live on a gatekeeper.
 *
 * Revision 1.38  2001/03/15 00:25:58  robertj
 * Fixed bug in receiving RIP packet, did not restart timeout.
 *
 * Revision 1.37  2001/03/09 02:55:53  robertj
 * Fixed bug in RAS IRR, optional field not being included, thanks Erik Larsson.
 *
 * Revision 1.36  2001/03/02 06:59:59  robertj
 * Enhanced the globally unique identifier class.
 *
 * Revision 1.35  2001/02/28 00:20:16  robertj
 * Added DiscoverByNameAndAddress() function, thanks Chris Purvis.
 *
 * Revision 1.34  2001/02/18 22:33:47  robertj
 * Added better handling of URJ, thanks Chris Purvis.
 *
 * Revision 1.33  2001/02/09 05:13:55  craigs
 * Added pragma implementation to (hopefully) reduce the executable image size
 * under Linux
 *
 * Revision 1.32  2001/01/25 01:44:26  robertj
 * Reversed order of changing alias list to avoid assert if delete all aliases.
 *
 * Revision 1.31  2000/11/01 03:30:27  robertj
 * Changed gatekeeper registration time to live to update in slightly less than the
 *    time to live time. Allows for system/network latency. Thanks Laurent PELLETIER.
 *
 * Revision 1.30  2000/09/25 06:48:11  robertj
 * Removed use of alias if there is no alias present, ie only have transport address.
 *
 * Revision 1.29  2000/09/01 02:12:37  robertj
 * Fixed problem when multiple GK's on LAN, only discovered first one.
 * Added ability to select a gatekeeper on LAN via it's identifier name.
 *
 * Revision 1.28  2000/07/15 09:54:21  robertj
 * Fixed problem with having empty or unusable assigned aliases.
 *
 * Revision 1.27  2000/07/11 19:26:39  robertj
 * Fixed problem with endpoint identifiers from some gatekeepers not being a string, just binary info.
 *
 * Revision 1.26  2000/06/20 03:18:04  robertj
 * Added function to get name of gatekeeper, subtle difference from getting identifier.
 *
 * Revision 1.25  2000/05/09 12:14:32  robertj
 * Added adjustment of endpoints alias list as approved by gatekeeper.
 *
 * Revision 1.24  2000/05/09 08:52:50  robertj
 * Added support for preGrantedARQ fields on registration.
 *
 * Revision 1.23  2000/05/04 10:43:54  robertj
 * Fixed problem with still trying to RRQ if got a GRJ.
 *
 * Revision 1.22  2000/05/02 04:32:26  robertj
 * Fixed copyright notice comment.
 *
 * Revision 1.21  2000/04/27 02:52:58  robertj
 * Added keepAlive field to RRQ if already registered,
 *
 * Revision 1.20  2000/04/12 21:22:16  robertj
 * Fixed warning in No Trace mode.
 *
 * Revision 1.19  2000/04/11 04:00:55  robertj
 * Filled in destCallSignallingAddress if specified by caller, used for gateway permissions.
 *
 * Revision 1.18  2000/04/11 03:11:12  robertj
 * Added ability to reject reason on gatekeeper requests.
 *
 * Revision 1.17  2000/03/29 02:14:43  robertj
 * Changed TerminationReason to CallEndReason to use correct telephony nomenclature.
 * Added CallEndReason for capability exchange failure.
 *
 * Revision 1.16  2000/03/23 02:45:28  robertj
 * Changed ClearAllCalls() so will wait for calls to be closed (usefull in endpoint dtors).
 *
 * Revision 1.15  2000/03/21 23:17:55  robertj
 * Changed GK client so does not fill in destCallSignalAddress on outgoing call.
 *
 * Revision 1.14  2000/01/28 00:56:48  robertj
 * Changed ACF to return destination address irrespective of callModel, thanks Chris Gindel.
 *
 * Revision 1.13  1999/12/23 23:02:35  robertj
 * File reorganision for separating RTP from H.323 and creation of LID for VPB support.
 *
 * Revision 1.12  1999/12/11 02:20:58  robertj
 * Added ability to have multiple aliases on local endpoint.
 *
 * Revision 1.11  1999/12/10 01:43:25  robertj
 * Fixed outgoing call Admissionrequestion addresses.
 *
 * Revision 1.10  1999/12/09 21:49:18  robertj
 * Added reregister on unregister and time to live reregistration
 *
 * Revision 1.9  1999/11/06 05:37:45  robertj
 * Complete rewrite of termination of connection to avoid numerous race conditions.
 *
 * Revision 1.8  1999/10/16 03:47:48  robertj
 * Fixed termination of gatekeeper RAS thread problem
 *
 * Revision 1.7  1999/10/15 05:55:50  robertj
 * Fixed crash in responding to InfoRequest
 *
 * Revision 1.6  1999/09/23 08:48:45  robertj
 * Changed register request so cannot do it of have no listeners.
 *
 * Revision 1.5  1999/09/21 14:09:19  robertj
 * Removed warnings when no tracing enabled.
 *
 * Revision 1.4  1999/09/14 08:19:37  robertj
 * Fixed timeout on retry of gatekeeper discover and added more tracing.
 *
 * Revision 1.3  1999/09/14 06:52:54  robertj
 * Added better support for multi-homed client hosts.
 *
 * Revision 1.2  1999/09/10 02:45:31  robertj
 * Added missing binding of address to transport when a specific gatway is used.
 *
 * Revision 1.1  1999/08/31 12:34:18  robertj
 * Added gatekeeper support.
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "gkclient.h"
#endif

#include <h323/gkclient.h>

#include <h323/h323ep.h>
#include <h323/h323con.h>
#include <h323/h323pdu.h>
#include <h323/h323rtp.h>


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

H323Gatekeeper::H323Gatekeeper(H323EndPoint & ep, OpalTransport * trans)
  : H225_RAS(ep, trans)
{
  discoveryComplete = FALSE;
  isRegistered = FALSE;

  pregrantMakeCall = pregrantAnswerCall = RequireARQ;

  authenticators.Append(new H235AuthSimpleMD5);
#if P_SSL
  authenticators.Append(new H235AuthProcedure1);
#endif

  autoReregister = TRUE;
  timeToLive.SetNotifier(PCREATE_NOTIFIER(RegistrationTimeToLive));
  requiresDiscovery = FALSE;
}


H323Gatekeeper::~H323Gatekeeper()
{
  timeToLive.Stop();
}


void H323Gatekeeper::PrintOn(ostream & strm) const
{
  if (!gatekeeperIdentifier)
    strm << gatekeeperIdentifier << "@";

  OpalTransportAddress addr = transport->GetRemoteAddress();

  PIPSocket::Address ip;
  WORD port;
  if (addr.GetIpAndPort(ip, port)) {
    strm << PIPSocket::GetHostName(ip);
    if (port != DefaultRasUdpPort)
      strm << ':' << port;
  }
  else
    strm << addr;
}


PString H323Gatekeeper::GetName() const
{
  PStringStream s;
  s << *this;
  return s;
}


BOOL H323Gatekeeper::DiscoverAny()
{
  gatekeeperIdentifier = PString();
  return StartDiscovery(OpalTransportAddress());
}


BOOL H323Gatekeeper::DiscoverByName(const PString & identifier)
{
  gatekeeperIdentifier = identifier;
  return StartDiscovery(OpalTransportAddress());
}


BOOL H323Gatekeeper::DiscoverByAddress(const PString & address)
{
  gatekeeperIdentifier = PString();
  OpalTransportAddress fullAddress(address,
                                   H225_RAS::DefaultRasUdpPort,
                                   OpalTransportAddress::UdpPrefix);
  return StartDiscovery(fullAddress);
}


BOOL H323Gatekeeper::DiscoverByNameAndAddress(const PString & identifier,
                                              const PString & address)
{
  gatekeeperIdentifier = identifier;
  return StartDiscovery(address);
}


static BOOL WriteGRQ(OpalTransport & transport, PObject * data)
{
  H323RasPDU & pdu = *(H323RasPDU *)data;
  H225_GatekeeperRequest & grq = pdu;
  H323TransportAddress localAddress = transport.GetLocalAddress();
  localAddress.SetPDU(grq.m_rasAddress);
  return pdu.Write(transport);
}


BOOL H323Gatekeeper::StartDiscovery(const OpalTransportAddress & address)
{
  PAssert(!transport->IsRunning(), "Cannot do discovery on running RAS channel");

  if (!transport->ConnectTo(address))
    return FALSE;

  H323RasPDU pdu(*this);
  Request request(SetupGatekeeperRequest(pdu), pdu);

  requestsMutex.Wait();
  requests.SetAt(request.sequenceNumber, &request);
  requestsMutex.Signal();

  for (unsigned retry = 0; retry < endpoint.GetGatekeeperRequestRetries(); retry++) {
    if (!transport->WriteConnect(WriteGRQ, &pdu)) {
      PTRACE(1, "RAS\tError writing discovery PDU: " << transport->GetErrorText());
      break;
    }

    H323RasPDU response(*this);
    if (response.Read(*transport) && HandleRasPDU(response))
      break;
  }

  transport->EndConnect(transport->GetLocalAddress());

  if (discoveryComplete) {
    PTRACE(2, "RAS\tGatekeeper discovered at: "
           << transport->GetRemoteAddress()
           << " (if=" << transport->GetLocalAddress() << ')');
    StartRasChannel();
  }

  requestsMutex.Wait();
  requests.SetAt(request.sequenceNumber, NULL);
  requestsMutex.Signal();

  return discoveryComplete;
}


unsigned H323Gatekeeper::SetupGatekeeperRequest(H323RasPDU & request)
{
  H225_GatekeeperRequest & grq = request.BuildGatekeeperRequest(GetNextSequenceNumber());

  H323TransportAddress rasAddress = transport->GetLocalAddress();
  rasAddress.SetPDU(grq.m_rasAddress);

  endpoint.SetEndpointTypeInfo(grq.m_endpointType);

  grq.IncludeOptionalField(H225_GatekeeperRequest::e_endpointAlias);
  H323SetAliasAddresses(endpoint.GetAliasNames(), grq.m_endpointAlias);

  if (!gatekeeperIdentifier) {
    grq.IncludeOptionalField(H225_GatekeeperRequest::e_gatekeeperIdentifier);
    grq.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  OnSendGatekeeperRequest(grq);

  discoveryComplete = FALSE;

  return grq.m_requestSeqNum;
}


BOOL H323Gatekeeper::OnReceiveGatekeeperConfirm(const H225_GatekeeperConfirm & gcf)
{
  if (!H225_RAS::OnReceiveGatekeeperConfirm(gcf))
    return FALSE;

  if (gatekeeperIdentifier.IsEmpty()) {
    gatekeeperIdentifier = gcf.m_gatekeeperIdentifier;
    discoveryComplete = TRUE;
  }
  else {
    PCaselessString gkid = (PString)gcf.m_gatekeeperIdentifier;
    if (gkid != gatekeeperIdentifier) {
      PTRACE(2, "RAS\tReceived a GCF from " << gkid
             << " but wanted it from " << gatekeeperIdentifier);
      return FALSE;
    }

    gatekeeperIdentifier = gkid;
    discoveryComplete = TRUE;
  }

  H323TransportAddress locatedAddress(gcf.m_rasAddress, OpalTransportAddress::UdpPrefix);
  PTRACE(2, "RAS\tGatekeeper discovery found " << locatedAddress);

  if (!transport->SetRemoteAddress(locatedAddress)) {
    PTRACE(2, "RAS\tInvalid gatekeeper discovery address: \"" << locatedAddress << '"');
    discoveryComplete = FALSE;
    return FALSE;
  }

  for (PINDEX i = 0; i < authenticators.GetSize(); i++) {
    H235Authenticator & authenticator = authenticators[i];
    if (authenticator.UseGkAndEpIdentifiers())
      authenticator.SetRemoteId(gatekeeperIdentifier);
  }

  return discoveryComplete;
}


BOOL H323Gatekeeper::RegistrationRequest(BOOL autoReg)
{
  autoReregister = autoReg;

  H323RasPDU pdu(*this);
  H225_RegistrationRequest & rrq = pdu.BuildRegistrationRequest(GetNextSequenceNumber());

  // If discoveryComplete flag is FALSE then do lightweight reregister
  rrq.m_discoveryComplete = discoveryComplete;

  H323TransportAddress rasAddress = transport->GetLocalAddress();
  rrq.m_rasAddress.SetSize(1);
  rasAddress.SetPDU(rrq.m_rasAddress[0]);

  const OpalListenerList & listeners = endpoint.GetListeners();
  if (listeners.IsEmpty()) {
    PTRACE(1, "RAS\tCannot register with Gatekeeper without a OpalListener!");
    return FALSE;
  }

  for (PINDEX i = 0; i < listeners.GetSize(); i++) {
    H323TransportAddress signalAddress = listeners[i].GetLocalAddress();
    signalAddress.SetPDU(rrq.m_callSignalAddress, rrq.m_rasAddress[0]);
  }

  endpoint.SetEndpointTypeInfo(rrq.m_terminalType);
  endpoint.SetVendorIdentifierInfo(rrq.m_endpointVendor);

  rrq.IncludeOptionalField(H225_RegistrationRequest::e_terminalAlias);
  H323SetAliasAddresses(endpoint.GetAliasNames(), rrq.m_terminalAlias);

  rrq.m_willSupplyUUIEs = TRUE;
  rrq.IncludeOptionalField(H225_RegistrationRequest::e_usageReportingCapability);
  rrq.m_usageReportingCapability.IncludeOptionalField(H225_RasUsageInfoTypes::e_startTime);
  rrq.m_usageReportingCapability.IncludeOptionalField(H225_RasUsageInfoTypes::e_endTime);
  rrq.m_usageReportingCapability.IncludeOptionalField(H225_RasUsageInfoTypes::e_terminationCause);

  if (!gatekeeperIdentifier) {
    rrq.IncludeOptionalField(H225_RegistrationRequest::e_gatekeeperIdentifier);
    rrq.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  if (!endpointIdentifier.IsEmpty()) {
    rrq.IncludeOptionalField(H225_RegistrationRequest::e_endpointIdentifier);
    rrq.m_endpointIdentifier = endpointIdentifier;
  }

  PTimeInterval ttl = endpoint.GetGatekeeperTimeToLive();
  if (ttl > 0) {
    rrq.IncludeOptionalField(H225_RegistrationRequest::e_timeToLive);
    rrq.m_timeToLive = (int)ttl.GetSeconds();
  }

  if (isRegistered) {
    rrq.IncludeOptionalField(H225_RegistrationRequest::e_keepAlive);
    rrq.m_keepAlive = TRUE;
  }

  // After doing full register, do lightweight reregisters from now on
  discoveryComplete = FALSE;

  Request request(rrq.m_requestSeqNum, pdu);
  return MakeRequest(request);
}


BOOL H323Gatekeeper::OnReceiveRegistrationConfirm(const H225_RegistrationConfirm & rcf)
{
  if (!H225_RAS::OnReceiveRegistrationConfirm(rcf))
    return FALSE;

  isRegistered = TRUE;

  endpointIdentifier = rcf.m_endpointIdentifier;

  PINDEX i;
  for (i = 0; i < authenticators.GetSize(); i++) {
    H235Authenticator & authenticator = authenticators[i];
    if (authenticator.UseGkAndEpIdentifiers())
      authenticator.SetLocalId(rcf.m_endpointIdentifier);
  }

  if (rcf.HasOptionalField(H225_RegistrationConfirm::e_timeToLive)) {
    int seconds = rcf.m_timeToLive;
    seconds -= 10; // Allow for an incredible amount of system/network latency
    if (seconds < 1)
      seconds = 1;
    timeToLive = PTimeInterval(0, seconds);
  }
  else
    timeToLive.Stop();

  // At present only support first call signal address to GK
  if (rcf.m_callSignalAddress.GetSize() > 0)
    gkRouteAddress = rcf.m_callSignalAddress[0];

  pregrantMakeCall = pregrantAnswerCall = RequireARQ;
  if (rcf.HasOptionalField(H225_RegistrationConfirm::e_preGrantedARQ)) {
    if (rcf.m_preGrantedARQ.m_makeCall)
      pregrantMakeCall = rcf.m_preGrantedARQ.m_useGKCallSignalAddressToMakeCall
                                                      ? PreGkRoutedARQ : PregrantARQ;
    if (rcf.m_preGrantedARQ.m_answerCall)
      pregrantAnswerCall = rcf.m_preGrantedARQ.m_useGKCallSignalAddressToAnswer
                                                      ? PreGkRoutedARQ : PregrantARQ;
  }

  // Remove the endpoint aliases that the gatekeeper did not like and add the
  // ones that it really wants us to be.
  if (rcf.HasOptionalField(H225_RegistrationConfirm::e_terminalAlias)) {
    const PStringList & currentAliases = endpoint.GetAliasNames();
    PStringList aliasesToChange;
    PINDEX j;

    for (i = 0; i < rcf.m_terminalAlias.GetSize(); i++) {
      PString alias = H323GetAliasAddressString(rcf.m_terminalAlias[i]);
      if (!alias) {
        for (j = 0; j < currentAliases.GetSize(); j++) {
          if (alias *= currentAliases[j])
            break;
        }
        if (j >= currentAliases.GetSize())
          aliasesToChange.AppendString(alias);
      }
    }
    for (i = 0; i < aliasesToChange.GetSize(); i++) {
      PTRACE(2, "RAS\tGatekeeper add of alias \"" << aliasesToChange[i] << '"');
      endpoint.AddAliasName(aliasesToChange[i]);
    }

    aliasesToChange.RemoveAll();

    for (i = 0; i < currentAliases.GetSize(); i++) {
      for (j = 0; j < rcf.m_terminalAlias.GetSize(); j++) {
        if (currentAliases[i] *= H323GetAliasAddressString(rcf.m_terminalAlias[j]))
          break;
      }
      if (j >= rcf.m_terminalAlias.GetSize())
        aliasesToChange.AppendString(currentAliases[i]);
    }
    for (i = 0; i < aliasesToChange.GetSize(); i++) {
      PTRACE(2, "RAS\tGatekeeper removal of alias \"" << aliasesToChange[i] << '"');
      endpoint.RemoveAliasName(aliasesToChange[i]);
    }
  }

  return TRUE;
}


BOOL H323Gatekeeper::OnReceiveRegistrationReject(const H225_RegistrationReject & rrj)
{
  if (!H225_RAS::OnReceiveRegistrationReject(rrj))
    return FALSE;

  if (lastRequest->rejectReason == H225_RegistrationRejectReason::e_discoveryRequired) {
    // If have been told by GK that we need to discover it again, set flag
    // for next register done by timeToLive handler to do discovery
    requiresDiscovery = TRUE;
    timeToLive = PTimeInterval(1);
  }

  isRegistered = FALSE;
  return TRUE;
}


void H323Gatekeeper::RegistrationTimeToLive(PTimer &, INT)
{
  PTRACE(3, "RAS\tTime To Live reregistration");

  if (requiresDiscovery) {
    PTRACE(2, "RAS\tRepeating discovery on gatekeepers request.");

    H323RasPDU pdu(*this);
    Request request(SetupGatekeeperRequest(pdu), pdu);
    if (!MakeRequest(request) || !discoveryComplete) {
      PTRACE(2, "RAS\tRediscovery failed, retrying in 1 minute.");
      timeToLive = PTimeInterval(0, 0, 1);
      return;
    }

    requiresDiscovery = FALSE;
  }

  if (!RegistrationRequest(autoReregister)) {
    PTRACE(2, "RAS\tTime To Live reregistration failed, retrying in 1 minute");
    timeToLive = PTimeInterval(0, 0, 1);
  }
}


BOOL H323Gatekeeper::UnregistrationRequest(int reason)
{
  H323RasPDU pdu(*this);
  H225_UnregistrationRequest & urq = pdu.BuildUnregistrationRequest(GetNextSequenceNumber());

  H323TransportAddress rasAddress = transport->GetLocalAddress();

  const OpalListenerList & listeners = endpoint.GetListeners();
  for (PINDEX i = 0; i < listeners.GetSize(); i++) {
    H323TransportAddress signalAddress = listeners[i].GetLocalAddress(rasAddress);
    signalAddress.SetPDU(urq.m_callSignalAddress, rasAddress);
  }

  urq.IncludeOptionalField(H225_UnregistrationRequest::e_endpointAlias);
  H323SetAliasAddresses(endpoint.GetAliasNames(), urq.m_endpointAlias);

  if (!gatekeeperIdentifier) {
    urq.IncludeOptionalField(H225_UnregistrationRequest::e_gatekeeperIdentifier);
    urq.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  if (!endpointIdentifier.IsEmpty()) {
    urq.IncludeOptionalField(H225_UnregistrationRequest::e_endpointIdentifier);
    urq.m_endpointIdentifier = endpointIdentifier;
  }

  if (reason >= 0) {
    urq.IncludeOptionalField(H225_UnregistrationRequest::e_reason);
    urq.m_reason = reason;
  }

  Request request(urq.m_requestSeqNum, pdu);
  return MakeRequest(request);
}


BOOL H323Gatekeeper::OnReceiveUnregistrationConfirm(const H225_UnregistrationConfirm & ucf)
{
  if (!H225_RAS::OnReceiveUnregistrationConfirm(ucf))
    return FALSE;

  isRegistered = FALSE;
  timeToLive.Stop();

  return TRUE;
}


BOOL H323Gatekeeper::OnReceiveUnregistrationRequest(const H225_UnregistrationRequest & urq)
{
  if (!H225_RAS::OnReceiveUnregistrationRequest(urq))
    return FALSE;

  PTRACE(2, "RAS\tUnregistration received");
  endpoint.ClearAllCalls(EndedByGatekeeper, FALSE);

  PTRACE(3, "RAS\tUnregistered, calls cleared");
  isRegistered = FALSE;
  timeToLive.Stop();

  H323RasPDU response(*this);
  response.BuildUnregistrationConfirm(urq.m_requestSeqNum);
  BOOL ok = WritePDU(response);

  if (autoReregister) {
    PTRACE(3, "RAS\tReregistering by setting timeToLive");
    timeToLive = 1;
  }

  return ok;
}


BOOL H323Gatekeeper::OnReceiveUnregistrationReject(const H225_UnregistrationReject & urj)
{
  if (!H225_RAS::OnReceiveUnregistrationReject(urj))
    return FALSE;

  if (lastRequest->rejectReason == H225_UnregRejectReason::e_notCurrentlyRegistered) {
    isRegistered = FALSE;
    timeToLive.Stop();
  }

  return TRUE;
}


BOOL H323Gatekeeper::LocationRequest(const PString & alias,
                                     OpalTransportAddress & address)
{
  PStringList aliases;
  aliases.AppendString(alias);
  return LocationRequest(aliases, address);
}


BOOL H323Gatekeeper::LocationRequest(const PStringList & aliases,
                                     OpalTransportAddress & address)
{
  H323RasPDU pdu(*this);
  H225_LocationRequest & lrq = pdu.BuildLocationRequest(GetNextSequenceNumber());

  H323SetAliasAddresses(aliases, lrq.m_destinationInfo);

  if (!endpointIdentifier.IsEmpty()) {
    lrq.IncludeOptionalField(H225_LocationRequest::e_endpointIdentifier);
    lrq.m_endpointIdentifier = endpointIdentifier;
  }

  H323TransportAddress replyAddress = transport->GetLocalAddress();
  replyAddress.SetPDU(lrq.m_replyAddress);

  lrq.IncludeOptionalField(H225_LocationRequest::e_sourceInfo);
  H323SetAliasAddresses(endpoint.GetAliasNames(), lrq.m_sourceInfo);

  if (!gatekeeperIdentifier) {
    lrq.IncludeOptionalField(H225_LocationRequest::e_gatekeeperIdentifier);
    lrq.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  Request request(lrq.m_requestSeqNum, pdu);
  request.responseInfo = &address;
  return MakeRequest(request);
}


H323Gatekeeper::AdmissionResponse::AdmissionResponse()
{
  rejectReason = UINT_MAX;
  transportAddress = NULL;
  aliasAddresses = NULL;
  accessTokenData = NULL;
}


struct AdmissionRequestResponseInfo {
  unsigned allocatedBandwidth;
  unsigned uuiesRequested;
  H323TransportAddress * transportAddress;
  H225_ArrayOf_AliasAddress * aliasAddresses;
  H225_ArrayOf_AliasAddress   destExtraCallInfo;
  PString      accessTokenOID1;
  PString      accessTokenOID2;
  PBYTEArray * accessTokenData;
};


BOOL H323Gatekeeper::AdmissionRequest(H323Connection & connection,
                                      AdmissionResponse & response,
                                      BOOL ignorePreGrantedARQ)
{
  BOOL answeringCall = connection.HadAnsweredCall();

  if (!ignorePreGrantedARQ) {
    switch (answeringCall ? pregrantAnswerCall : pregrantMakeCall) {
      case RequireARQ :
        break;
      case PregrantARQ :
        return TRUE;
      case PreGkRoutedARQ :
        if (gkRouteAddress.IsEmpty())
          return FALSE;
        if (response.transportAddress != NULL)
          *response.transportAddress = gkRouteAddress;
        return TRUE;
    }
  }

  H323RasPDU pdu(*this);
  H225_AdmissionRequest & arq = pdu.BuildAdmissionRequest(GetNextSequenceNumber());

  arq.m_callType.SetTag(H225_CallType::e_pointToPoint);
  arq.m_endpointIdentifier = endpointIdentifier;
  arq.m_answerCall = answeringCall;
  arq.m_canMapAlias = TRUE; // Stack supports receiving a different number in the ACF 
                            // to the one sent in the ARQ
  arq.m_willSupplyUUIEs = TRUE;

  if (!gatekeeperIdentifier) {
    arq.IncludeOptionalField(H225_AdmissionRequest::e_gatekeeperIdentifier);
    arq.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  PString localInfo = connection.GetLocalPartyName();
  PString destInfo = connection.GetRemotePartyName();
  arq.m_srcInfo.SetSize(1);
  if (answeringCall) {
    H323SetAliasAddress(destInfo, arq.m_srcInfo[0]);

    if (!localInfo) {
      arq.IncludeOptionalField(H225_AdmissionRequest::e_destinationInfo);
      arq.m_destinationInfo.SetSize(1);
      H323SetAliasAddress(localInfo, arq.m_destinationInfo[0]);
    }
  }
  else {
    if (localInfo == endpoint.GetLocalUserName())
      H323SetAliasAddresses(endpoint.GetAliasNames(), arq.m_srcInfo);
    else {
      arq.m_srcInfo.SetSize(1);
      H323SetAliasAddress(localInfo, arq.m_srcInfo[0]);
    }
    if (response.transportAddress == NULL || destInfo != *response.transportAddress) {
      arq.IncludeOptionalField(H225_AdmissionRequest::e_destinationInfo);
      arq.m_destinationInfo.SetSize(1);
      H323SetAliasAddress(destInfo, arq.m_destinationInfo[0]);
    }
  }

  const OpalTransport * signallingChannel = connection.GetSignallingChannel();
  arq.IncludeOptionalField(H225_AdmissionRequest::e_srcCallSignalAddress);
  if (answeringCall) {
    H323TransportAddress signalAddress = signallingChannel->GetRemoteAddress();
    signalAddress.SetPDU(arq.m_srcCallSignalAddress);
    signalAddress = signallingChannel->GetLocalAddress();
    arq.IncludeOptionalField(H225_AdmissionRequest::e_destCallSignalAddress);
    signalAddress.SetPDU(arq.m_destCallSignalAddress);
  }
  else {
    H323TransportAddress signalAddress = signallingChannel->GetLocalAddress();
    signalAddress.SetPDU(arq.m_srcCallSignalAddress);
    if (response.transportAddress != NULL && !response.transportAddress->IsEmpty()) {
      arq.IncludeOptionalField(H225_AdmissionRequest::e_destCallSignalAddress);
      response.transportAddress->SetPDU(arq.m_destCallSignalAddress);
    }
  }

  arq.m_bandWidth = connection.GetBandwidthAvailable();
  arq.m_callReferenceValue = connection.GetCallReference();
  arq.m_conferenceID = connection.GetConferenceIdentifier();
  arq.m_callIdentifier.m_guid = connection.GetCallIdentifier();

  AdmissionRequestResponseInfo info;
  info.transportAddress = response.transportAddress;
  info.aliasAddresses   = response.aliasAddresses;
  info.accessTokenData  = response.accessTokenData;
  info.accessTokenOID1  = connection.GetGkAccessTokenOID();
  PINDEX comma = info.accessTokenOID1.Find(',');
  if (comma == P_MAX_INDEX)
    info.accessTokenOID2 = info.accessTokenOID1;
  else {
    info.accessTokenOID2 = info.accessTokenOID1.Mid(comma+1);
    info.accessTokenOID1.Delete(comma, P_MAX_INDEX);
  }

  Request request(arq.m_requestSeqNum, pdu);
  request.responseInfo = &info;

  if (!MakeRequest(request)) {
    response.rejectReason = request.rejectReason;
    return FALSE;
  }

  connection.SetBandwidthAvailable(info.allocatedBandwidth);
  connection.SetUUIEsRequested(info.uuiesRequested);

  if (info.destExtraCallInfo.GetSize() > 0) {
    // Just the first element for the time being
    PString destExtraCallInfo = H323GetAliasAddressString(info.destExtraCallInfo[0]);
    connection.SetDestExtraCallInfo(destExtraCallInfo);
  }

  return TRUE;
}


void H323Gatekeeper::OnSendAdmissionRequest(H225_AdmissionRequest & arq)
{
  if (authenticators.IsEmpty() || !GetSeparateAuthenticationInARQ()) {
    H225_RAS::OnSendAdmissionRequest(arq);
    return;
  }

  PString remoteId;
  PString localId;
  PString password;
  if (!GetAdmissionReqestAuthentication(arq, remoteId, localId, password)) {
    PTRACE(2, "RAS\tNo alias to replace authenticators credentials during ARQ");
    H225_RAS::OnSendAdmissionRequest(arq);
    return;
  }

  PTRACE(3, "RAS\tAuthenticators credentials replaced with "
         << remoteId << " during ARQ");

  mutexForSeparateAuthenticationInARQ.Wait();

  PStringList remoteIds, localIds, passwords;
  PINDEX i;
  for (i = 0; i < authenticators.GetSize(); i++) {
    H235Authenticator & authenticator = authenticators[i];

    remoteIds.AppendString(authenticator.GetRemoteId());
    authenticator.SetRemoteId(remoteId);

    localIds.AppendString(authenticator.GetLocalId());
    authenticator.SetLocalId(localId);

    passwords.AppendString(authenticator.GetPassword());
    authenticator.SetPassword(password);
  }

  // Call ancestor to set the crypto tokens
  H225_RAS::OnSendAdmissionRequest(arq);

  // Put all the credentials back again
  for (i = 0; i < authenticators.GetSize(); i++) {
    H235Authenticator & authenticator = authenticators[i];
    authenticator.SetRemoteId(remoteIds[i]);
    authenticator.SetLocalId(localIds[i]);
    authenticator.SetPassword(passwords[i]);
  }

  mutexForSeparateAuthenticationInARQ.Signal();
}


BOOL H323Gatekeeper::GetAdmissionReqestAuthentication(const H225_AdmissionRequest & arq,
                                                      PString & remoteId,
                                                      PString & localId,
                                                      PString & password)
{
  if (arq.m_answerCall)
    remoteId = H323GetAliasAddressString(arq.m_srcInfo[0]);
  else {
    if (arq.HasOptionalField(H225_AdmissionRequest::e_destinationInfo) &&
        arq.m_destinationInfo.GetSize() > 0)
      remoteId = H323GetAliasAddressString(arq.m_destinationInfo[0]);
    else
      return FALSE;
  }

  localId = PString();
  password = remoteId;
  return TRUE;
}


static unsigned GetUUIEsRequested(const H225_UUIEsRequested & pdu)
{
  unsigned uuiesRequested = 0;

  if ((BOOL)pdu.m_setup)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_setup);
  if ((BOOL)pdu.m_callProceeding)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_callProceeding);
  if ((BOOL)pdu.m_connect)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_connect);
  if ((BOOL)pdu.m_alerting)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_alerting);
  if ((BOOL)pdu.m_information)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_information);
  if ((BOOL)pdu.m_releaseComplete)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_releaseComplete);
  if ((BOOL)pdu.m_facility)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_facility);
  if ((BOOL)pdu.m_progress)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_progress);
  if ((BOOL)pdu.m_empty)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_empty);

  if (pdu.HasOptionalField(H225_UUIEsRequested::e_status) && (BOOL)pdu.m_status)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_status);
  if (pdu.HasOptionalField(H225_UUIEsRequested::e_statusInquiry) && (BOOL)pdu.m_statusInquiry)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_statusInquiry);
  if (pdu.HasOptionalField(H225_UUIEsRequested::e_setupAcknowledge) && (BOOL)pdu.m_setupAcknowledge)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_setupAcknowledge);
  if (pdu.HasOptionalField(H225_UUIEsRequested::e_notify) && (BOOL)pdu.m_notify)
    uuiesRequested |= (1<<H225_H323_UU_PDU_h323_message_body::e_notify);

  return uuiesRequested;
}



BOOL H323Gatekeeper::OnReceiveAdmissionConfirm(const H225_AdmissionConfirm & acf)
{
  if (!H225_RAS::OnReceiveAdmissionConfirm(acf))
    return FALSE;

  AdmissionRequestResponseInfo & info = *(AdmissionRequestResponseInfo *)lastRequest->responseInfo;
  info.allocatedBandwidth = acf.m_bandWidth;
  if (info.transportAddress != NULL)
    *info.transportAddress = acf.m_destCallSignalAddress;

  // Remove the endpoint aliases that the gatekeeper did not like and add the
  // ones that it really wants us to be.
  if (info.aliasAddresses != NULL &&
      acf.HasOptionalField(H225_AdmissionConfirm::e_destinationInfo)) {
    PTRACE(3, "RAS\tGatekeeper specified " << acf.m_destinationInfo.GetSize() << " aliases in ACF");
    *info.aliasAddresses = acf.m_destinationInfo;
  }

  if (acf.HasOptionalField(H225_AdmissionConfirm::e_uuiesRequested))
    info.uuiesRequested = GetUUIEsRequested(acf.m_uuiesRequested);

  if (acf.HasOptionalField(H225_AdmissionConfirm::e_destExtraCallInfo))
    info.destExtraCallInfo = acf.m_destExtraCallInfo;

  if (info.accessTokenData != NULL && !info.accessTokenOID1 &&
      acf.HasOptionalField(H225_AdmissionConfirm::e_tokens)) {
    PTRACE(4, "Looking for OID " << info.accessTokenOID1 << " in ACF to copy.");
    for (PINDEX i = 0; i < acf.m_tokens.GetSize(); i++) {
      if (acf.m_tokens[i].m_tokenOID == info.accessTokenOID1) {
        PTRACE(4, "Looking for OID " << info.accessTokenOID2 << " in token to copy.");
        if (acf.m_tokens[i].HasOptionalField(H235_ClearToken::e_nonStandard) &&
            acf.m_tokens[i].m_nonStandard.m_nonStandardIdentifier == info.accessTokenOID2) {
          PTRACE(4, "Copying ACF nonStandard OctetString.");
          *info.accessTokenData = acf.m_tokens[i].m_nonStandard.m_data;
          break;
        }
      }
    }
  }

  return TRUE;
}


static void SetRasUsageInformation(const H323Connection & connection, 
                                   H225_RasUsageInformation & usage)
{
  unsigned time = connection.GetAlertingTime().GetTimeInSeconds();
  if (time != 0) {
    usage.IncludeOptionalField(H225_RasUsageInformation::e_alertingTime);
    usage.m_alertingTime = time;
  }

  time = connection.GetConnectionStartTime().GetTimeInSeconds();
  if (time != 0) {
    usage.IncludeOptionalField(H225_RasUsageInformation::e_connectTime);
    usage.m_connectTime = time;
  }

  time = connection.GetConnectionEndTime().GetTimeInSeconds();
  if (time != 0) {
    usage.IncludeOptionalField(H225_RasUsageInformation::e_endTime);
    usage.m_endTime = time;
  }
}


BOOL H323Gatekeeper::DisengageRequest(const H323Connection & connection, unsigned reason)
{
  H323RasPDU pdu(*this);
  H225_DisengageRequest & drq = pdu.BuildDisengageRequest(GetNextSequenceNumber());

  drq.m_endpointIdentifier = endpointIdentifier;
  drq.m_conferenceID = connection.GetConferenceIdentifier();
  drq.m_callReferenceValue = connection.GetCallReference();
  drq.m_callIdentifier.m_guid = connection.GetCallIdentifier();
  drq.m_disengageReason.SetTag(reason);
  drq.m_answeredCall = connection.HadAnsweredCall();

  drq.IncludeOptionalField(H225_DisengageRequest::e_usageInformation);
  SetRasUsageInformation(connection, drq.m_usageInformation);

  drq.IncludeOptionalField(H225_DisengageRequest::e_terminationCause);
  drq.m_terminationCause.SetTag(H225_CallTerminationCause::e_releaseCompleteReason);
  Q931::CauseValues cause = H323TranslateFromCallEndReason(connection, drq.m_terminationCause);
  if (cause != Q931::ErrorInCauseIE) {
    drq.m_terminationCause.SetTag(H225_CallTerminationCause::e_releaseCompleteCauseIE);
    PASN_OctetString & rcReason = drq.m_terminationCause;
    rcReason.SetSize(2);
    rcReason[0] = 0x80;
    rcReason[1] = (BYTE)(0x80|cause);
  }

  if (!gatekeeperIdentifier) {
    drq.IncludeOptionalField(H225_DisengageRequest::e_gatekeeperIdentifier);
    drq.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  Request request(drq.m_requestSeqNum, pdu);
  return MakeRequest(request);
}


BOOL H323Gatekeeper::OnReceiveDisengageRequest(const H225_DisengageRequest & drq)
{
  if (!H225_RAS::OnReceiveDisengageRequest(drq))
    return FALSE;

  OpalGloballyUniqueID id = NULL;
  if (drq.HasOptionalField(H225_DisengageRequest::e_callIdentifier))
    id = drq.m_callIdentifier.m_guid;
  if (id == NULL)
    id = drq.m_conferenceID;

  endpoint.ClearCall(id.AsString(), EndedByGatekeeper);

  H323RasPDU response(*this);

  H323Connection * connection = endpoint.FindConnectionWithLock(id.AsString());
  if (connection == NULL)
    response.BuildDisengageReject(drq.m_requestSeqNum,
                                  H225_DisengageRejectReason::e_requestToDropOther);
  else {
    H225_DisengageConfirm & dcf = response.BuildDisengageConfirm(drq.m_requestSeqNum);

    dcf.IncludeOptionalField(H225_DisengageConfirm::e_usageInformation);
    SetRasUsageInformation(*connection, dcf.m_usageInformation);

    connection->Release(EndedByGatekeeper);
    connection->Unlock();
  }

  return WritePDU(response);
}


BOOL H323Gatekeeper::BandwidthRequest(H323Connection & connection,
                                      unsigned requestedBandwidth)
{
  H323RasPDU pdu(*this);
  H225_BandwidthRequest & brq = pdu.BuildBandwidthRequest(GetNextSequenceNumber());

  brq.m_endpointIdentifier = endpointIdentifier;
  brq.m_conferenceID = connection.GetConferenceIdentifier();
  brq.m_callReferenceValue = connection.GetCallReference();
  brq.m_callIdentifier.m_guid = connection.GetCallIdentifier();
  brq.m_bandWidth = requestedBandwidth;
  brq.IncludeOptionalField(H225_BandwidthRequest::e_usageInformation);
  SetRasUsageInformation(connection, brq.m_usageInformation);

  Request request(brq.m_requestSeqNum, pdu);
  
  unsigned allocatedBandwidth;
  request.responseInfo = &allocatedBandwidth;

  if (!MakeRequest(request))
    return FALSE;

  connection.SetBandwidthAvailable(allocatedBandwidth);

  return TRUE;
}


BOOL H323Gatekeeper::OnReceiveBandwidthConfirm(const H225_BandwidthConfirm & bcf)
{
  if (!H225_RAS::OnReceiveBandwidthConfirm(bcf))
    return FALSE;

  if (lastRequest->responseInfo != NULL)
    *(unsigned *)lastRequest->responseInfo = bcf.m_bandWidth;

  return TRUE;
}


BOOL H323Gatekeeper::OnReceiveBandwidthRequest(const H225_BandwidthRequest & brq)
{
  if (!H225_RAS::OnReceiveBandwidthRequest(brq))
    return FALSE;

  OpalGloballyUniqueID id = brq.m_callIdentifier.m_guid;
  H323Connection * connection = endpoint.FindConnectionWithLock(id.AsString());

  H323RasPDU response(*this);
  if (connection == NULL)
    response.BuildBandwidthReject(brq.m_requestSeqNum,
                                  H225_BandRejectReason::e_invalidConferenceID);
  else {
    if (connection->SetBandwidthAvailable(brq.m_bandWidth))
      response.BuildBandwidthConfirm(brq.m_requestSeqNum, brq.m_bandWidth);
    else
      response.BuildBandwidthReject(brq.m_requestSeqNum,
                                    H225_BandRejectReason::e_insufficientResources);
    connection->Unlock();
  }

  return WritePDU(response);
}


void H323Gatekeeper::InfoRequestResponse(const H323Connection * connection,
                                         unsigned seqNum)
{
  H323RasPDU response(*this);
  H225_InfoRequestResponse & irr = response.BuildInfoRequestResponse(seqNum);

  BuildInfoRequestResponse(irr, connection);

  WritePDU(response);
}


void H323Gatekeeper::InfoRequestResponse(const H323Connection & connection,
                                         const H225_H323_UU_PDU & pdu,
                                         BOOL sent)
{
  // Are unknown Q.931 PDU
  if (pdu.m_h323_message_body.GetTag() == P_MAX_INDEX)
    return;

  // Check mask of things to report on
  if ((connection.GetUUIEsRequested() & (1<<pdu.m_h323_message_body.GetTag())) == 0)
    return;

  // Report the PDU
  H323RasPDU response(*this);

  H225_InfoRequestResponse & irr = response.BuildInfoRequestResponse(GetNextSequenceNumber());
  BuildInfoRequestResponse(irr, &connection);

  PINDEX index = irr.m_perCallInfo.GetSize();
  irr.m_perCallInfo.SetSize(index+1);
  irr.m_perCallInfo[index].IncludeOptionalField(H225_InfoRequestResponse_perCallInfo_subtype::e_pdu);

  PINDEX index2 = irr.m_perCallInfo[index].m_pdu.GetSize();
  irr.m_perCallInfo[index].m_pdu.SetSize(index2+1);
  irr.m_perCallInfo[index].m_pdu[index2].m_sent = sent;
  irr.m_perCallInfo[index].m_pdu[index2].m_h323pdu = pdu;

  WritePDU(response);
}


void H323Gatekeeper::BuildInfoRequestResponse(H225_InfoRequestResponse & irr,
                                              const H323Connection * connection)
{
  endpoint.SetEndpointTypeInfo(irr.m_endpointType);
  irr.m_endpointIdentifier = endpointIdentifier;
  H323TransportAddress rasAddress = transport->GetLocalAddress();
  rasAddress.SetPDU(irr.m_rasAddress);

  const OpalListenerList & listeners = endpoint.GetListeners();
  for (PINDEX i = 0; i < listeners.GetSize(); i++) {
    H323TransportAddress signalAddress = listeners[i].GetLocalAddress(rasAddress);
    signalAddress.SetPDU(irr.m_callSignalAddress, rasAddress);
  }

  irr.IncludeOptionalField(H225_InfoRequestResponse::e_endpointAlias);
  H323SetAliasAddresses(endpoint.GetAliasNames(), irr.m_endpointAlias);

  if (connection != NULL) {
    irr.IncludeOptionalField(H225_InfoRequestResponse::e_perCallInfo);
    irr.m_perCallInfo.SetSize(1);
    H225_InfoRequestResponse_perCallInfo_subtype & info = irr.m_perCallInfo[0];
    info.m_callReferenceValue = connection->GetCallReference();
    info.m_callIdentifier.m_guid = connection->GetCallIdentifier();
    info.m_conferenceID = connection->GetConferenceIdentifier();
    H323_RTP_Session * session = connection->GetSessionCallbacks(OpalMediaFormat::DefaultAudioSessionID);
    if (session != NULL) {
      info.IncludeOptionalField(H225_InfoRequestResponse_perCallInfo_subtype::e_audio);
      info.m_audio.SetSize(1);
      session->OnSendRasInfo(info.m_audio[0]);
    }
    session = connection->GetSessionCallbacks(OpalMediaFormat::DefaultVideoSessionID);
    if (session != NULL) {
      info.IncludeOptionalField(H225_InfoRequestResponse_perCallInfo_subtype::e_video);
      info.m_video.SetSize(1);
      session->OnSendRasInfo(info.m_video[0]);
    }
    const OpalTransport & controlChannel = connection->GetControlChannel();
    H323TransportAddress h245Address = controlChannel.GetLocalAddress();
    h245Address.SetPDU(info.m_h245.m_recvAddress);
    h245Address = controlChannel.GetRemoteAddress();
    h245Address.SetPDU(info.m_h245.m_sendAddress);
    info.m_callType.SetTag(H225_CallType::e_pointToPoint);
    info.m_bandWidth = connection->GetBandwidthUsed();
    info.m_callModel.SetTag(H225_CallModel::e_direct);
    info.IncludeOptionalField(H225_InfoRequestResponse_perCallInfo_subtype::e_usageInformation);
    SetRasUsageInformation(*connection, info.m_usageInformation);
  }
}


BOOL H323Gatekeeper::OnReceiveInfoRequest(const H225_InfoRequest & irq)
{
  if (!H225_RAS::OnReceiveInfoRequest(irq))
    return FALSE;

  OpalGloballyUniqueID id = irq.m_callIdentifier.m_guid;
  H323Connection * connection = endpoint.FindConnectionWithLock(id.AsString());

  if (connection != NULL && irq.HasOptionalField(H225_InfoRequest::e_uuiesRequested))
    connection->SetUUIEsRequested(::GetUUIEsRequested(irq.m_uuiesRequested));

  InfoRequestResponse(connection, irq.m_requestSeqNum);

  if (connection != NULL)
    connection->Unlock();

  return TRUE;
}


H235Authenticators H323Gatekeeper::GetAuthenticators() const
{
  return authenticators;
}


void H323Gatekeeper::SetPassword(const PString & password, 
                                 const PString & username)
{
  PString localId = username;
  if (localId.IsEmpty())
    localId = endpoint.GetLocalUserName();

  for (PINDEX i = 0; i < authenticators.GetSize(); i++) {
    authenticators[i].SetLocalId(localId);
    authenticators[i].SetPassword(password);
  }
}


/////////////////////////////////////////////////////////////////////////////
