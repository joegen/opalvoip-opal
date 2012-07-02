/*
 * endpoint.cxx
 *
 * Media channels abstraction
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
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
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "endpoint.h"
#endif

#include <opal/buildopts.h>

#include <opal/endpoint.h>
#include <opal/manager.h>
#include <opal/call.h>
#include <rtp/rtp_session.h>

static const OpalBandwidth DefaultInitialBandwidth = 4000000; // 4Mb/s

#define new PNEW


/////////////////////////////////////////////////////////////////////////////

OpalEndPoint::OpalEndPoint(OpalManager & mgr,
                           const PCaselessString & prefix,
                           unsigned attributes)
  : manager(mgr)
  , prefixName(prefix)
  , attributeBits(attributes)
  , defaultSignalPort(0)
  , m_maxSizeUDP(4096)
  , productInfo(mgr.GetProductInfo())
  , defaultLocalPartyName(manager.GetDefaultUserName())
  , defaultDisplayName(manager.GetDefaultDisplayName())
  , m_initialRxBandwidth(DefaultInitialBandwidth)
  , m_initialTxBandwidth(DefaultInitialBandwidth)
  , defaultSendUserInputMode(OpalConnection::SendUserInputAsProtocolDefault)
{
  manager.AttachEndPoint(this);

  if (defaultLocalPartyName.IsEmpty())
    defaultLocalPartyName = PProcess::Current().GetName() & "User";

  PTRACE(4, "OpalEP\tCreated endpoint: " << prefixName);
}


OpalEndPoint::~OpalEndPoint()
{
  PTRACE(4, "OpalEP\t" << prefixName << " endpoint destroyed.");
}


void OpalEndPoint::ShutDown()
{
  PTRACE(3, "OpalEP\t" << prefixName << " endpoint shutting down.");

  // Shut down the listeners as soon as possible to avoid race conditions
  listeners.RemoveAll();
}


void OpalEndPoint::PrintOn(ostream & strm) const
{
  strm << "EP<" << prefixName << '>';
}


OpalBandwidth OpalEndPoint::GetInitialBandwidth(OpalBandwidth::Direction dir) const
{
  switch (dir) {
    case OpalBandwidth::Rx :
      return m_initialRxBandwidth;
    case OpalBandwidth::Tx :
      return m_initialTxBandwidth;
    default :
      return m_initialRxBandwidth+m_initialTxBandwidth;
  }
}


void OpalEndPoint::SetInitialBandwidth(OpalBandwidth::Direction dir, OpalBandwidth bandwidth)
{
  switch (dir) {
    case OpalBandwidth::Rx :
      m_initialRxBandwidth = bandwidth;
      break;

    case OpalBandwidth::Tx :
      m_initialTxBandwidth = bandwidth;
      break;

    default :
      OpalBandwidth rx = (PUInt64)(unsigned)bandwidth*m_initialRxBandwidth/(m_initialRxBandwidth+m_initialTxBandwidth);
      OpalBandwidth tx = (PUInt64)(unsigned)bandwidth*m_initialTxBandwidth/(m_initialRxBandwidth+m_initialTxBandwidth);
      m_initialRxBandwidth = rx;
      m_initialTxBandwidth = tx;
  }
}


PBoolean OpalEndPoint::GarbageCollection()
{
  for (PSafePtr<OpalConnection> connection(connectionsActive, PSafeReference); connection != NULL; ++connection)
    connection->GarbageCollection();

  return connectionsActive.DeleteObjectsToBeRemoved();
}


PBoolean OpalEndPoint::StartListeners(const PStringArray & listenerAddresses)
{
  PStringArray interfaces = listenerAddresses;
  if (interfaces.IsEmpty()) {
    interfaces = GetDefaultListeners();
    if (interfaces.IsEmpty())
      return PFalse;
  }

  PBoolean startedOne = PFalse;

  for (PINDEX i = 0; i < interfaces.GetSize(); i++) {
    if (interfaces[i].Find('$') != P_MAX_INDEX) {
      if (StartListener(interfaces[i]))
        startedOne = PTrue;
    }
    else {
      PStringArray transports = GetDefaultTransport().Tokenise(',');
      for (PINDEX j = 0; j < transports.GetSize(); j++) {
        PString transport = transports[j];
        WORD port = GetDefaultSignalPort();
        PINDEX colon = transport.Find(':');
        if (colon != P_MAX_INDEX) {
          port = (WORD)transport.Mid(colon+1).AsUnsigned();
          transport.Delete(colon, P_MAX_INDEX);
        }
        OpalTransportAddress iface(interfaces[i], port, transport);
        if (StartListener(iface))
          startedOne = PTrue;
      }
    }
  }

  return startedOne;
}


PBoolean OpalEndPoint::StartListener(const OpalTransportAddress & listenerAddress)
{
  OpalListener * listener;

  OpalTransportAddress iface = listenerAddress;

  if (iface.IsEmpty()) {
    PStringArray interfaces = GetDefaultListeners();
    if (interfaces.IsEmpty())
      return PFalse;
    iface = OpalTransportAddress(interfaces[0], GetDefaultSignalPort());
  }

  listener = iface.CreateListener(*this, OpalTransportAddress::FullTSAP);
  if (listener == NULL) {
    PTRACE(1, "OpalEP\tCould not create listener: " << iface);
    return PFalse;
  }

  if (StartListener(listener))
    return PTrue;

  PTRACE(1, "OpalEP\tCould not start listener: " << iface);
  return PFalse;
}


PBoolean OpalEndPoint::StartListener(OpalListener * listener)
{
  if (listener == NULL)
    return false;

  OpalListenerUDP * udpListener = dynamic_cast<OpalListenerUDP *>(listener);
  if (udpListener != NULL)
    udpListener->SetBufferSize(m_maxSizeUDP);

  // as the listener is not open, this will have the effect of immediately
  // stopping the listener thread. This is good - it means that the 
  // listener Close function will appear to have stopped the thread
  if (!listener->Open(PCREATE_NOTIFIER(ListenerCallback))) {
    delete listener;
    return false;
  }

  listeners.Append(listener);
  return true;
}

PString OpalEndPoint::GetDefaultTransport() const
{
  return OpalTransportAddress::TcpPrefix();
}

PStringArray OpalEndPoint::GetDefaultListeners() const
{
  PStringArray listenerAddresses;
  PStringArray transports = GetDefaultTransport().Tokenise(',');
  for (PINDEX i = 0; i < transports.GetSize(); i++) {
    PCaselessString transProto = transports[i];
    WORD port = GetDefaultSignalPort();
    PINDEX pos = transProto.Find(':');
    if (pos != P_MAX_INDEX) {
      port = (WORD)transProto.Mid(pos+1).AsUnsigned();
      transProto.Delete(pos, P_MAX_INDEX);
    }
    if (port != 0) {
      if (transProto == OpalTransportAddress::UdpPrefix())
        listenerAddresses += transProto + psprintf("*:%u", port);
      else {
        listenerAddresses += OpalTransportAddress(PIPSocket::Address::GetAny(4), port, transProto);
#if OPAL_PTLIB_IPV6
        listenerAddresses += OpalTransportAddress(PIPSocket::Address::GetAny(6), port, transProto);
#endif
      }
    }
  }
  return listenerAddresses;
}


OpalListener * OpalEndPoint::FindListener(const OpalTransportAddress & iface)
{
  for (OpalListenerList::iterator listener = listeners.begin(); listener != listeners.end(); ++listener) {
    if (listener->GetLocalAddress().IsEquivalent(iface, true))
      return &*listener;
  }
  return NULL;
}


PBoolean OpalEndPoint::StopListener(const OpalTransportAddress & iface)
{
  OpalListener * listener = FindListener(iface);
  return listener != NULL && RemoveListener(listener);
}


PBoolean OpalEndPoint::RemoveListener(OpalListener * listener)
{
  if (listener != NULL)
    return listeners.Remove(listener);

  listeners.RemoveAll();
  return PTrue;
}


static void AddTransportAddress(OpalTransportAddressArray & interfaceAddresses,
                                const PIPSocket::Address & natInterfaceIP,
                                const PIPSocket::Address & natExternalIP,
                                const PIPSocket::Address & ip,
                                WORD port,
                                const PString & proto)
{
  if (ip != natExternalIP && (natInterfaceIP.IsAny() || ip == natInterfaceIP))
    AddTransportAddress(interfaceAddresses, natInterfaceIP, natExternalIP, natExternalIP, port, proto);

  OpalTransportAddress addr(ip, port, proto);
  if (interfaceAddresses.GetValuesIndex(addr) == P_MAX_INDEX)
    interfaceAddresses.Append(new OpalTransportAddress(addr));
}


static void AddTransportAddresses(OpalTransportAddressArray & interfaceAddresses,
                                  bool excludeLocalHost,
                                  const PIPSocket::Address & natInterfaceIP,
                                  const PIPSocket::Address & natExternalIP,
                                  const OpalTransportAddress & preferredAddress,
                                  const OpalTransportAddress & localAddress,
                                  bool includeProto)
{
  PCaselessString proto = localAddress.GetProtoPrefix();
  if (includeProto && proto != preferredAddress.GetProto(true))
    return;

  if (!preferredAddress.IsEmpty() && !preferredAddress.IsEquivalent(localAddress, true))
    return;

  PIPSocket::Address localIP;
  WORD port = 0;
  if (!localAddress.GetIpAndPort(localIP, port))
    return;

  PIPSocket::InterfaceTable interfaces;
  if (!localIP.IsAny() || !PIPSocket::GetInterfaceTable(interfaces)) {
    AddTransportAddress(interfaceAddresses, natInterfaceIP, natExternalIP, localIP, port, proto);
    return;
  }


  PIPSocket::Address preferredIP;
  if (preferredAddress.GetIpAddress(preferredIP) && !preferredIP.IsAny()) {
    for (PINDEX i = 0; i < interfaces.GetSize(); i++) {
      PIPSocket::Address ip = interfaces[i].GetAddress();
      if (ip == preferredIP)
        AddTransportAddress(interfaceAddresses, natInterfaceIP, natExternalIP, ip, port, proto);
    }
  }

  for (PINDEX i = 0; i < interfaces.GetSize(); i++) {
    PIPSocket::Address ip = interfaces[i].GetAddress();
    if (ip.GetVersion() == localIP.GetVersion() && !(excludeLocalHost && ip.IsLoopback()))
      AddTransportAddress(interfaceAddresses, natInterfaceIP, natExternalIP, ip, port, proto);
  }
}


OpalTransportAddressArray OpalEndPoint::GetInterfaceAddresses(PBoolean excludeLocalHost,
                                                              const OpalTransport * associatedTransport) const
{
  OpalTransportAddressArray interfaceAddresses;

  OpalTransportAddress associatedLocalAddress, associatedRemoteAddress, associatedWildcard;
  PIPSocket::Address natInterfaceIP = PIPSocket::GetInvalidAddress();
  PIPSocket::Address natExternalIP = PIPSocket::GetInvalidAddress();
  if (associatedTransport != NULL) {
    associatedLocalAddress = associatedTransport->GetLocalAddress();
    associatedRemoteAddress = associatedTransport->GetRemoteAddress();

    PIPSocket::Address remoteIP;
    associatedRemoteAddress.GetIpAddress(remoteIP);

    associatedWildcard = OpalTransportAddress(PIPSocket::Address::GetAny(remoteIP.GetVersion()), 0,
                                              associatedLocalAddress.GetProtoPrefix());

#if P_NAT
    PNatMethod * natMethod = manager.GetNatMethod(remoteIP);
    if (natMethod != NULL) {
      natMethod->GetInterfaceAddress(natInterfaceIP);
      natMethod->GetExternalAddress(natExternalIP);
    }
    else
#endif
    if (manager.HasTranslationAddress()) {
      natInterfaceIP = PIPSocket::GetDefaultIpAny();
      natExternalIP = manager.GetTranslationAddress();
    }
  }

  OpalListenerList::const_iterator listener;

  if (!associatedLocalAddress.IsEmpty()) {
    for (listener = listeners.begin(); listener != listeners.end(); ++listener)
      AddTransportAddresses(interfaceAddresses,
                            excludeLocalHost,
                            natInterfaceIP,
                            natExternalIP,
                            associatedLocalAddress,
                            listener->GetLocalAddress(associatedRemoteAddress),
                            true);
    for (listener = listeners.begin(); listener != listeners.end(); ++listener)
      AddTransportAddresses(interfaceAddresses,
                            excludeLocalHost,
                            natInterfaceIP,
                            natExternalIP,
                            associatedWildcard,
                            listener->GetLocalAddress(associatedRemoteAddress),
                            true);
    for (listener = listeners.begin(); listener != listeners.end(); ++listener)
      AddTransportAddresses(interfaceAddresses,
                            excludeLocalHost,
                            natInterfaceIP,
                            natExternalIP,
                            associatedLocalAddress,
                            listener->GetLocalAddress(associatedRemoteAddress),
                            false);
  }

  for (listener = listeners.begin(); listener != listeners.end(); ++listener)
    AddTransportAddresses(interfaceAddresses,
                          excludeLocalHost,
                          natInterfaceIP,
                          natExternalIP,
                          OpalTransportAddress(),
                          listener->GetLocalAddress(),
                          false);

  PTRACE(4, "OpalMan\tListener interfaces: associated transport="
         << (associatedTransport != NULL ? (const char *)associatedLocalAddress : "None")
         << "\n    " << setfill(',') << interfaceAddresses);
  return interfaceAddresses;
}


void OpalEndPoint::ListenerCallback(PThread &, INT param)
{
  // Error in accept, usually means listener thread stopped, so just exit
  if (param == 0)
    return;

  OpalTransport * transport = (OpalTransport *)param;
  if (NewIncomingConnection(transport))
    delete transport;
}


PBoolean OpalEndPoint::NewIncomingConnection(OpalTransport * /*transport*/)
{
  return PTrue;
}


PSafePtr<OpalConnection> OpalEndPoint::GetConnectionWithLock(const PString & token, PSafetyMode mode) const
{
  if (token.IsEmpty())
    return NULL;

  PSafePtr<OpalConnection> connection = connectionsActive.FindWithLock(token, mode);
  if (connection != NULL)
    return connection;

  PSafePtr<OpalCall> call = manager.FindCallWithLock(token, PSafeReadOnly);
  if (call != NULL) {
    for (PINDEX i = 0; (connection = call->GetConnection(i)) != NULL; ++i) {
      if (&connection->GetEndPoint() == this)
        return connection.SetSafetyMode(mode) ? connection : NULL;
    }
  }

  if (token.NumCompare(GetPrefixName()+':') != EqualTo)
    return NULL;

  PString name = token.Mid(GetPrefixName().GetLength()+1);
  for (connection = PSafePtr<OpalConnection>(connectionsActive, PSafeReference); connection != NULL; ++connection) {
    if (connection->GetLocalPartyName() == name)
      return connection.SetSafetyMode(mode) ? connection : NULL;
  }

  return NULL;
}


PStringList OpalEndPoint::GetAllConnections()
{
  PStringList tokens;

  for (PSafePtr<OpalConnection> connection(connectionsActive, PSafeReadOnly); connection != NULL; ++connection)
    tokens.AppendString(connection->GetToken());

  return tokens;
}


PBoolean OpalEndPoint::HasConnection(const PString & token)
{
  return connectionsActive.Contains(token);
}


OpalConnection * OpalEndPoint::AddConnection(OpalConnection * connection)
{
  if (connection == NULL)
    return NULL;

  connection->SetStringOptions(m_defaultStringOptions, false);

  OnNewConnection(connection->GetCall(), *connection);

  connectionsActive.SetAt(connection->GetToken(), connection);

  return connection;
}


void OpalEndPoint::DestroyConnection(OpalConnection * connection)
{
  delete connection;
}


PBoolean OpalEndPoint::OnSetUpConnection(OpalConnection & PTRACE_PARAM(connection))
{
  PTRACE(3, "OpalEP\tOnSetUpConnection " << connection);
  return PTrue;
}


PBoolean OpalEndPoint::OnIncomingConnection(OpalConnection & connection, unsigned options, OpalConnection::StringOptions * stringOptions)
{
  return manager.OnIncomingConnection(connection, options, stringOptions);
}


void OpalEndPoint::OnProceeding(OpalConnection & connection)
{
  manager.OnProceeding(connection);
}

void OpalEndPoint::OnAlerting(OpalConnection & connection)
{
  manager.OnAlerting(connection);
}

OpalConnection::AnswerCallResponse
       OpalEndPoint::OnAnswerCall(OpalConnection & connection,
                                  const PString & caller)
{
  return manager.OnAnswerCall(connection, caller);
}

void OpalEndPoint::OnConnected(OpalConnection & connection)
{
  manager.OnConnected(connection);
}


void OpalEndPoint::OnEstablished(OpalConnection & connection)
{
  manager.OnEstablished(connection);
}


void OpalEndPoint::OnReleased(OpalConnection & connection)
{
  PTRACE(4, "OpalEP\tOnReleased " << connection);
  connectionsActive.RemoveAt(connection.GetToken());
  manager.OnReleased(connection);
}


void OpalEndPoint::OnHold(OpalConnection & connection, bool fromRemote, bool onHold)
{
  manager.OnHold(connection, fromRemote, onHold);
}


void OpalEndPoint::OnHold(OpalConnection & connection)
{
  manager.OnHold(connection);
}


PBoolean OpalEndPoint::OnForwarded(OpalConnection & connection,
			       const PString & forwardParty)
{
  PTRACE(4, "OpalEP\tOnForwarded " << connection);
  return manager.OnForwarded(connection, forwardParty);
}


bool OpalEndPoint::OnTransferNotify(OpalConnection & connection, const PStringToString & info)
{
  return manager.OnTransferNotify(connection, info);
}


void OpalEndPoint::ConnectionDict::DeleteObject(PObject * object) const
{
  OpalConnection * connection = PDownCast(OpalConnection, object);
  if (connection != NULL)
    connection->GetEndPoint().DestroyConnection(connection);
}


PBoolean OpalEndPoint::ClearCall(const PString & token,
                             OpalConnection::CallEndReason reason,
                             PSyncPoint * sync)
{
  return manager.ClearCall(token, reason, sync);
}


PBoolean OpalEndPoint::ClearCallSynchronous(const PString & token,
                                        OpalConnection::CallEndReason reason,
                                        PSyncPoint * sync)
{
  PSyncPoint syncPoint;
  if (sync == NULL)
    sync = &syncPoint;
  return manager.ClearCall(token, reason, sync);
}


void OpalEndPoint::ClearAllCalls(OpalConnection::CallEndReason reason, PBoolean wait)
{
  manager.ClearAllCalls(reason, wait);
}


void OpalEndPoint::AdjustMediaFormats(bool local,
                                      const OpalConnection & connection,
                                      OpalMediaFormatList & mediaFormats) const
{
  manager.AdjustMediaFormats(local, connection, mediaFormats);
}


PBoolean OpalEndPoint::OnOpenMediaStream(OpalConnection & connection,
                                     OpalMediaStream & stream)
{
  return manager.OnOpenMediaStream(connection, stream);
}


void OpalEndPoint::OnClosedMediaStream(const OpalMediaStream & stream)
{
  manager.OnClosedMediaStream(stream);
}


void OpalEndPoint::SetMediaCryptoSuites(const PStringArray & security)
{
  m_mediaCryptoSuites = security;

  PStringArray valid = GetAllMediaCryptoSuites();
  PAssert(!valid.IsEmpty(), PInvalidParameter);

  PINDEX i = 0;
  while (i < m_mediaCryptoSuites.GetSize()) {
    if (valid.GetValuesIndex(m_mediaCryptoSuites[i]) != P_MAX_INDEX)
      ++i;
    else
      m_mediaCryptoSuites.RemoveAt(i);
  }

  if (m_mediaCryptoSuites.IsEmpty())
    m_mediaCryptoSuites.AppendString(valid[0]);
}


PStringArray OpalEndPoint::GetAllMediaCryptoSuites() const
{
  PStringArray cryptoSuites;

  cryptoSuites.AppendString(OpalMediaCryptoSuite::ClearText());

  OpalMediaCryptoSuiteFactory::KeyList_T all = OpalMediaCryptoSuiteFactory::GetKeyList();
  for  (OpalMediaCryptoSuiteFactory::KeyList_T::iterator it = all.begin(); it != all.end(); ++it) {
    if (*it != OpalMediaCryptoSuite::ClearText() && OpalMediaCryptoSuiteFactory::CreateInstance(*it)->Supports(GetPrefixName()))
      cryptoSuites.AppendString(*it);
  }

  return cryptoSuites;
}


#if P_NAT
PNatStrategy & OpalEndPoint::GetNatMethods() const
{
  return manager.GetNatMethods();
}


PNatMethod * OpalEndPoint::GetNatMethod(const PIPSocket::Address & remoteAddress) const
{
  return manager.GetNatMethod(remoteAddress);
}
#endif


#if OPAL_VIDEO
PBoolean OpalEndPoint::CreateVideoInputDevice(const OpalConnection & connection,
                                          const OpalMediaFormat & mediaFormat,
                                          PVideoInputDevice * & device,
                                          PBoolean & autoDelete)
{
  return manager.CreateVideoInputDevice(connection, mediaFormat, device, autoDelete);
}

PBoolean OpalEndPoint::CreateVideoOutputDevice(const OpalConnection & connection,
                                           const OpalMediaFormat & mediaFormat,
                                           PBoolean preview,
                                           PVideoOutputDevice * & device,
                                           PBoolean & autoDelete)
{
  return manager.CreateVideoOutputDevice(connection, mediaFormat, preview, device, autoDelete);
}
#endif


void OpalEndPoint::OnUserInputString(OpalConnection & connection,
                                     const PString & value)
{
  manager.OnUserInputString(connection, value);
}


void OpalEndPoint::OnUserInputTone(OpalConnection & connection,
                                   char tone,
                                   int duration)
{
  manager.OnUserInputTone(connection, tone, duration);
}


PString OpalEndPoint::ReadUserInput(OpalConnection & connection,
                                    const char * terminators,
                                    unsigned lastDigitTimeout,
                                    unsigned firstDigitTimeout)
{
  return manager.ReadUserInput(connection, terminators, lastDigitTimeout, firstDigitTimeout);
}


void OpalEndPoint::OnNewConnection(OpalCall & call, OpalConnection & connection)
{
  call.OnNewConnection(connection);
}


void OpalEndPoint::OnMWIReceived(const PString & party,
                                 OpalManager::MessageWaitingType type,
                                 const PString & extraInfo)
{
  manager.OnMWIReceived(party, type, extraInfo);
}


bool OpalEndPoint::GetConferenceStates(OpalConferenceStates &, const PString &) const
{
  return false;
}


void OpalEndPoint::OnConferenceStatusChanged(OpalEndPoint &, const PString &, OpalConferenceState::ChangeType)
{
}


PStringList OpalEndPoint::GetNetworkURIs(const PString & name) const
{
  PStringList list;

  const PStringList prefixes = manager.GetPrefixNames(this);

  OpalTransportAddressArray addresses = GetInterfaceAddresses();
  for (PINDEX i = 0; i < addresses.GetSize(); ++i) {
    PIPSocket::Address ip;
    WORD port = GetDefaultSignalPort();
    if (addresses[i].GetIpAndPort(ip, port)) {
      for (PStringList::const_iterator it = prefixes.begin(); it != prefixes.end(); ++it) {
        PURL uri;
        if (uri.SetScheme(*it)) {
          uri.SetUserName(name);
          uri.SetHostName(ip.AsString());
          if (uri.GetPort() != port)
            uri.SetPort(port);
          list += uri.AsString();
        }
      }
    }
  }

  return list;
}


bool OpalEndPoint::FindListenerForProtocol(const char * protoPrefix, OpalTransportAddress & addr)
{
  OpalTransportAddress compatibleTo("*", 0, protoPrefix);
  for (OpalListenerList::iterator listener = listeners.begin(); listener != listeners.end(); ++listener) {
    addr = listener->GetLocalAddress();
    if (addr.IsCompatible(compatibleTo))
      return true;
   }
  return false;
}

#if OPAL_PTLIB_SSL
bool OpalEndPoint::GetSSLCredentials(PSSLCertificate & ca, PSSLCertificate & cert, PSSLPrivateKey & key) const
{
  return manager.GetSSLCredentials(*this, ca, cert, key);
}
#endif


#if OPAL_HAS_IM

bool OpalEndPoint::Message(const PString & to, const PString & body)
{
  return manager.Message(to, body);
}


PBoolean OpalEndPoint::Message(
  const PURL & to, 
  const PString & type,
  const PString & body,
  PURL & from, 
  PString & conversationId
)
{
  return manager.Message(to, type, body, from, conversationId);
}


PBoolean OpalEndPoint::Message(OpalIM & message)
{
  return manager.Message(message);
}


void OpalEndPoint::OnMessageReceived(const OpalIM & message)
{
  manager.OnMessageReceived(message);
}

#endif // OPAL_HAS_IM


/////////////////////////////////////////////////////////////////////////////

bool OpalIsE164(const PString & number, bool strict)
{
  if (number.IsEmpty())
    return false;

  PINDEX offset;

  if (strict || number[0] != '+')
    offset = 0;
  else {
    if (number.GetLength() < 2)
      return false;
    offset = 1;
  }

  return number.FindSpan("1234567890*#", offset) == P_MAX_INDEX;
}


/////////////////////////////////////////////////////////////////////////////
