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
 * Revision 1.2002  2001/08/01 05:16:18  robertj
 * Moved default session ID's to OpalMediaFormat class.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
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
#include <h323/h235ras.h>
#include <h323/h323rtp.h>


#define new PNEW


/////////////////////////////////////////////////////////////////////////////

H323Gatekeeper::H323Gatekeeper(H323EndPoint & ep, OpalTransport * trans)
  : H225_RAS(ep, trans)
{
  discoveryComplete = FALSE;
  isRegistered = FALSE;

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


BOOL H323Gatekeeper::DiscoverByAddress(const OpalTransportAddress & address)
{
  gatekeeperIdentifier = PString();
  return StartDiscovery(address);
}


BOOL H323Gatekeeper::DiscoverByNameAndAddress(const PString & identifier,
                                              const OpalTransportAddress & address)
{
  gatekeeperIdentifier = identifier;
  return StartDiscovery(address);
}

BOOL H323Gatekeeper::StartDiscovery(const OpalTransportAddress & address)
{
  PWaitAndSignal mutex(makeRequestMutex);

  H323RasPDU request;
  SetupGatekeeperRequest(request);

  requestTag = request.GetTag();

  unsigned retries = endpoint.GetGatekeeperRequestRetries();
  while (!DiscoverGatekeeper(request, address)) {
    if (--retries == 0)
      return FALSE;
  }

  if (!discoveryComplete)
    return FALSE;

  if (transport->Connect())
    StartRasChannel();

  return TRUE;
}


BOOL H323Gatekeeper::DiscoverGatekeeper(H323RasPDU & request,
                                        const OpalTransportAddress & address)
{
  if (!transport->IsDescendant(OpalTransportUDP::Class())) {
    PTRACE(1, "H225\tOnly UDP supported for Gatekeeper discovery");
    return FALSE;
  }

  PTRACE(3, "H225\tStarted gatekeeper discovery of \"" << address << '"');

  PIPSocket::Address destAddr = INADDR_BROADCAST;
  WORD destPort = DefaultRasUdpPort;
  if (!address) {
    if (!address.GetIpAndPort(destAddr, destPort)) {
      PTRACE(2, "RAS\tError decoding address");
      return FALSE;
    }
  }

  PIPSocket::Address originalLocalAddress;
  WORD originalLocalPort;
  transport->GetLocalAddress().GetIpAndPort(originalLocalAddress, originalLocalPort);

  // Skip over the H323Transport::Close to make sure PUDPSocket is deleted.
  transport->PIndirectChannel::Close();

  PIPSocket::InterfaceTable interfaces;

  // See if prebound to interface, only use that if so
  if (originalLocalAddress != INADDR_ANY) {
    PTRACE(3, "RAS\tGatekeeper discovery on pre-bound interface: " << originalLocalAddress);
    interfaces.Append(new PIPSocket::InterfaceEntry("", originalLocalAddress, PIPSocket::Address(0xffffffff), ""));
  }
  else if (!PIPSocket::GetInterfaceTable(interfaces)) {
    PTRACE(1, "RAS\tNo interfaces on system!");
    interfaces.Append(new PIPSocket::InterfaceEntry("", originalLocalAddress, PIPSocket::Address(0xffffffff), ""));
  }
  else {
    PTRACE(4, "RAS\tSearching interfaces:\n" << setfill('\n') << interfaces << setfill(' '));
  }

  PSocketList sockets;
  PSocket::SelectList selection;
  H225_GatekeeperRequest & grq = request;

  PINDEX i;
  for (i = 0; i < interfaces.GetSize(); i++) {
    PIPSocket::Address interfaceAddress = interfaces[i].GetAddress();
    if (interfaceAddress == 0 || interfaceAddress == PIPSocket::Address())
      continue;

    // Check for already have had that IP address.
    PINDEX j;
    for (j = 0; j < i; j++) {
      if (interfaceAddress == interfaces[j].GetAddress())
        break;
    }
    if (j < i)
      continue;

    PUDPSocket * socket;

    static PIPSocket::Address MulticastRasAddress(224, 0, 1, 41);
    if (destAddr != MulticastRasAddress) {

      // Not explicitly multicast
      socket = new PUDPSocket;
      sockets.Append(socket);

      if (!socket->Listen(interfaceAddress)) {
        PTRACE(2, "RAS\tError on discover request listen: " << socket->GetErrorText());
        return FALSE;
      }

//      localPort = socket->GetPort();

#ifndef __BEOS__
      if (destAddr == INADDR_BROADCAST) {
        if (!socket->SetOption(SO_BROADCAST, 1)) {
          PTRACE(2, "RAS\tError allowing broadcast: " << socket->GetErrorText());
          return FALSE;
        }
      }
#else
      PTRACE(2, "RAS\tBroadcast option under BeOS is not implemented yet");
#endif

      // Adjust the PDU to reflect the interface we are writing to.
      H323TransportAddress newLocalAddress(interfaceAddress, socket->GetPort(), "udp");
      newLocalAddress.SetPDU(grq.m_rasAddress);

      PTRACE(3, "RAS\tGatekeeper discovery on interface: " << newLocalAddress);

      socket->SetSendAddress(destAddr, destPort);
      transport->SetWriteChannel(socket, FALSE);
      if (request.Write(*transport))
        selection.Append(socket);
      else
        PTRACE(2, "RAS\tError writing discovery PDU: " << socket->GetErrorText());

#ifndef __BEOS__
      if (destAddr == INADDR_BROADCAST)
        socket->SetOption(SO_BROADCAST, 0);
#endif
    }


#ifdef IP_ADD_MEMBERSHIP
    // Now do it again for Multicast
    if (destAddr == INADDR_BROADCAST || destAddr == MulticastRasAddress) {
      socket = new PUDPSocket;
      sockets.Append(socket);

      if (!socket->Listen(interfaceAddress)) {
        PTRACE(2, "RAS\tError on discover request listen: " << socket->GetErrorText());
        return FALSE;
      }

      struct ip_mreq mreq;
      mreq.imr_multiaddr = MulticastRasAddress;
      mreq.imr_interface = interfaceAddress;    // ip address of host
      if (socket->SetOption(IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq), IPPROTO_IP)) {
        // Adjust the PDU to reflect the interface we are writing to.
        H323TransportAddress newLocalMulticastAddress(interfaceAddress, socket->GetPort(), "udp");
        newLocalMulticastAddress.SetPDU(grq.m_rasAddress);

        socket->SetOption(SO_BROADCAST, 1);

        socket->SetSendAddress(INADDR_BROADCAST, DefaultRasMulticastPort);
        transport->SetWriteChannel(socket, FALSE);
        if (request.Write(*transport))
          selection.Append(socket);
        else
          PTRACE(2, "RAS\tError writing discovery PDU: " << socket->GetErrorText());

        socket->SetOption(SO_BROADCAST, 0);
      }
      else
        PTRACE(2, "RAS\tError allowing multicast: " << socket->GetErrorText());
    }
#endif

    transport->SetWriteChannel(NULL);
  }

  if (sockets.IsEmpty()) {
    PTRACE(1, "RAS\tNo suitable interfaces for discovery!");
    return FALSE;
  }

  if (PSocket::Select(selection, endpoint.GetGatekeeperRequestTimeout()) != PChannel::NoError) {
    PTRACE(3, "RAS\tError on discover request select");
    return FALSE;
  }

  transport->SetReadTimeout(0);

  for (i = 0; i < selection.GetSize(); i++) {
    transport->SetReadChannel(&selection[i], FALSE);
    transport->SetPromiscuous(TRUE);

    H323RasPDU response;
    if (!response.Read(*transport)) {
      PTRACE(3, "RAS\tError on discover request read: " << transport->GetErrorText());
      break;
    }

    do {
      if (HandleRasPDU(response)) {
        PUDPSocket * socket = (PUDPSocket *)transport->GetReadChannel();
        transport->SetReadChannel(NULL);
        if (transport->Open(socket)) {
          sockets.DisallowDeleteObjects();
          sockets.Remove(socket);
          sockets.AllowDeleteObjects();

          PTRACE(2, "RAS\tGatekeeper discovered at: "
                 << transport->GetRemoteAddress()
                 << " (if=" << transport->GetLocalAddress() << ')');
          return TRUE;
        }
      }
    } while (response.Read(*transport));
  }

  PTRACE(2, "RAS\tGatekeeper discovery failed");
  transport->PIndirectChannel::Close();
  return FALSE;
}


void H323Gatekeeper::SetupGatekeeperRequest(H323RasPDU & request)
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
}


BOOL H323Gatekeeper::OnReceiveGatekeeperConfirm(const H225_GatekeeperConfirm & gcf)
{
  if (!H225_RAS::OnReceiveGatekeeperConfirm(gcf))
    return FALSE;

  locatedAddress = gcf.m_rasAddress;
  PTRACE(2, "RAS\tGatekeeper discovery found " << locatedAddress);

  if (!transport->SetRemoteAddress(locatedAddress)) {
    PTRACE(2, "RAS\tInvalid gatekeeper discovery address: \"" << locatedAddress << '"');
  }

  if (!gatekeeperIdentifier)
    discoveryComplete = (gatekeeperIdentifier *= gcf.m_gatekeeperIdentifier);
  else {
    gatekeeperIdentifier = gcf.m_gatekeeperIdentifier;
    discoveryComplete = TRUE;
  }

  return discoveryComplete;
}


BOOL H323Gatekeeper::RegistrationRequest(BOOL autoReg)
{
  PWaitAndSignal mutex(makeRequestMutex);

  autoReregister = autoReg;

  H323_DECLARE_RAS_PDU(request, endpoint);
  H225_RegistrationRequest & rrq = request.BuildRegistrationRequest(GetNextSequenceNumber());

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

  if (!gatekeeperIdentifier) {
    rrq.IncludeOptionalField(H225_RegistrationRequest::e_gatekeeperIdentifier);
    rrq.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  if (!endpointIdentifier.IsEmpty()) {
    rrq.IncludeOptionalField(H225_RegistrationRequest::e_endpointIdentifier);
    rrq.m_endpointIdentifier.SetValue(endpointIdentifier);
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

  return MakeRequest(request);
}


BOOL H323Gatekeeper::OnReceiveRegistrationConfirm(const H225_RegistrationConfirm & rcf)
{
  if (!H225_RAS::OnReceiveRegistrationConfirm(rcf))
    return FALSE;

  isRegistered = TRUE;

  rcf.m_endpointIdentifier.GetValue(endpointIdentifier);

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
    PINDEX i, j;

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

  if (rejectReason == H225_RegistrationRejectReason::e_discoveryRequired) {
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

    PWaitAndSignal mutex(makeRequestMutex);
    H323RasPDU request;
    SetupGatekeeperRequest(request);
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
  PWaitAndSignal mutex(makeRequestMutex);

  H323_DECLARE_RAS_PDU(request, endpoint);
  H225_UnregistrationRequest & urq = request.BuildUnregistrationRequest(GetNextSequenceNumber());

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
    urq.m_endpointIdentifier.SetValue(endpointIdentifier);
  }

  if (reason >= 0) {
    urq.IncludeOptionalField(H225_UnregistrationRequest::e_reason);
    urq.m_reason = reason;
  }

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


void H323Gatekeeper::OnReceiveUnregistrationRequest(const H225_UnregistrationRequest & urq)
{
  PTRACE(2, "RAS\tUnregistration received");
  endpoint.ClearAllCalls(H323Connection::EndedByGatekeeper, FALSE);

  PTRACE(3, "RAS\tUnregistered, calls cleared");
  isRegistered = FALSE;
  timeToLive.Stop();

  H323_DECLARE_RAS_PDU(response, endpoint);
  response.BuildUnregistrationConfirm(urq.m_requestSeqNum);
  WritePDU(response);

  if (autoReregister) {
    PTRACE(3, "RAS\tReregistering by setting timeToLive");
    timeToLive = 1;
  }
}


BOOL H323Gatekeeper::OnReceiveUnregistrationReject(const H225_UnregistrationReject & urj)
{
  if (!H225_RAS::OnReceiveUnregistrationReject(urj))
    return FALSE;

  if (rejectReason == H225_UnregRejectReason::e_notCurrentlyRegistered) {
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
  PWaitAndSignal mutex(makeRequestMutex);

  H323_DECLARE_RAS_PDU(request, endpoint);
  H225_LocationRequest & lrq = request.BuildLocationRequest(GetNextSequenceNumber());

  H323SetAliasAddresses(aliases, lrq.m_destinationInfo);

  if (!endpointIdentifier.IsEmpty()) {
    lrq.IncludeOptionalField(H225_LocationRequest::e_endpointIdentifier);
    lrq.m_endpointIdentifier.SetValue(endpointIdentifier);
  }

  H323TransportAddress replyAddress = transport->GetLocalAddress();
  replyAddress.SetPDU(lrq.m_replyAddress);

  lrq.IncludeOptionalField(H225_LocationRequest::e_sourceInfo);
  H323SetAliasAddresses(endpoint.GetAliasNames(), lrq.m_sourceInfo);

  if (!gatekeeperIdentifier) {
    lrq.IncludeOptionalField(H225_LocationRequest::e_gatekeeperIdentifier);
    lrq.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  if (!MakeRequest(request))
    return FALSE;

  address = locatedAddress;

  return TRUE;
}


BOOL H323Gatekeeper::AdmissionRequest(H323Connection & connection)
{
  H323TransportAddress address;
  return AdmissionRequest(connection, address);
}


BOOL H323Gatekeeper::AdmissionRequest(H323Connection & connection,
                                      H323TransportAddress & address,
                                      BOOL ignorePreGrantedARQ)
{
  PWaitAndSignal mutex(makeRequestMutex);

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
        address = gkRouteAddress;
        return TRUE;
    }
  }

  H323_DECLARE_RAS_PDU(request, endpoint);
  H225_AdmissionRequest & arq = request.BuildAdmissionRequest(GetNextSequenceNumber());

  arq.m_callType.SetTag(H225_CallType::e_pointToPoint);
  arq.m_endpointIdentifier.SetValue(endpointIdentifier);
  arq.m_answerCall = answeringCall;

  PString destInfo = connection.GetRemotePartyName();
  arq.m_srcInfo.SetSize(1);
  if (answeringCall)
    H323SetAliasAddress(destInfo, arq.m_srcInfo[0]);
  else {
    H323SetAliasAddresses(endpoint.GetAliasNames(), arq.m_srcInfo);
    if (destInfo != address) {
      arq.IncludeOptionalField(H225_AdmissionRequest::e_destinationInfo);
      arq.m_destinationInfo.SetSize(1);
      H323SetAliasAddress(connection.GetRemotePartyName(), arq.m_destinationInfo[0]);
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
    if (!address) {
      arq.IncludeOptionalField(H225_AdmissionRequest::e_destCallSignalAddress);
      address.SetPDU(arq.m_destCallSignalAddress);
    }
  }

  arq.m_bandWidth = connection.GetBandwidthAvailable();
  arq.m_callReferenceValue = connection.GetCallReference();
  arq.m_conferenceID = connection.GetConferenceIdentifier();
  arq.m_callIdentifier.m_guid = connection.GetCallIdentifier();

  locatedAddress = H323TransportAddress();

  if (!MakeRequest(request))
    return FALSE;

  connection.SetBandwidthAvailable(allocatedBandwidth);
  address = locatedAddress;

  return TRUE;
}


BOOL H323Gatekeeper::OnReceiveAdmissionConfirm(const H225_AdmissionConfirm & acf)
{
  if (!H225_RAS::OnReceiveAdmissionConfirm(acf))
    return FALSE;

  locatedAddress = acf.m_destCallSignalAddress;
  allocatedBandwidth = acf.m_bandWidth;

  return TRUE;
}


BOOL H323Gatekeeper::DisengageRequest(const H323Connection & connection, unsigned reason)
{
  PWaitAndSignal mutex(makeRequestMutex);

  H323_DECLARE_RAS_PDU(request, endpoint);
  H225_DisengageRequest & drq = request.BuildDisengageRequest(GetNextSequenceNumber());

  drq.m_endpointIdentifier.SetValue(endpointIdentifier);
  drq.m_conferenceID = connection.GetConferenceIdentifier();
  drq.m_callReferenceValue = connection.GetCallReference();
  drq.m_callIdentifier.m_guid = connection.GetCallIdentifier();
  drq.m_disengageReason.SetTag(reason);
  drq.m_answeredCall = connection.HadAnsweredCall();

  if (!gatekeeperIdentifier) {
    drq.IncludeOptionalField(H225_DisengageRequest::e_gatekeeperIdentifier);
    drq.m_gatekeeperIdentifier = gatekeeperIdentifier;
  }

  return MakeRequest(request);
}


void H323Gatekeeper::OnReceiveDisengageRequest(const H225_DisengageRequest & drq)
{
  OpalGloballyUniqueID id = drq.m_callIdentifier.m_guid;
  endpoint.ClearCall(id.AsString(), H323Connection::EndedByGatekeeper);

  H323_DECLARE_RAS_PDU(response, endpoint);
  response.BuildDisengageConfirm(drq.m_requestSeqNum);
  WritePDU(response);
}


BOOL H323Gatekeeper::BandwidthRequest(H323Connection & connection,
                                      unsigned requestedBandwidth)
{
  PWaitAndSignal mutex(makeRequestMutex);

  H323_DECLARE_RAS_PDU(request, endpoint);
  H225_BandwidthRequest & brq = request.BuildBandwidthRequest(GetNextSequenceNumber());

  brq.m_endpointIdentifier.SetValue(endpointIdentifier);
  brq.m_conferenceID = connection.GetConferenceIdentifier();
  brq.m_callReferenceValue = connection.GetCallReference();
  brq.m_callIdentifier.m_guid = connection.GetCallIdentifier();
  brq.m_bandWidth = requestedBandwidth;

  if (!MakeRequest(request))
    return FALSE;

  connection.SetBandwidthAvailable(allocatedBandwidth);

  return TRUE;
}


BOOL H323Gatekeeper::OnReceiveBandwidthConfirm(const H225_BandwidthConfirm & bcf)
{
  if (!H225_RAS::OnReceiveBandwidthConfirm(bcf))
    return FALSE;

  allocatedBandwidth = bcf.m_bandWidth;

  return TRUE;
}


void H323Gatekeeper::OnReceiveBandwidthRequest(const H225_BandwidthRequest & brq)
{
  OpalGloballyUniqueID id = brq.m_callIdentifier.m_guid;
  H323Connection * connection = endpoint.FindConnectionWithLock(id.AsString());

  H323_DECLARE_RAS_PDU(response, endpoint);
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

  WritePDU(response);
}


void H323Gatekeeper::InfoRequestResponse(const H323Connection * connection,
                                         unsigned seqNum)
{
  H323_DECLARE_RAS_PDU(response, endpoint);
  H225_InfoRequestResponse & irr = response.BuildInfoRequestResponse(seqNum);

  endpoint.SetEndpointTypeInfo(irr.m_endpointType);

  irr.m_endpointIdentifier.SetValue(endpointIdentifier);
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
  }

  WritePDU(response);
}


void H323Gatekeeper::OnReceiveInfoRequest(const H225_InfoRequest & irq)
{
  OpalGloballyUniqueID id = irq.m_callIdentifier.m_guid;
  H323Connection * connection = endpoint.FindConnectionWithLock(id.AsString());

  InfoRequestResponse(connection, irq.m_requestSeqNum);
  if (connection != NULL)
    connection->Unlock();
}


/////////////////////////////////////////////////////////////////////////////
