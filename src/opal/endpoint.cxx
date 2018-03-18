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
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "endpoint.h"
#endif

#include <opal_config.h>

#include <opal/endpoint.h>
#include <opal/manager.h>
#include <opal/call.h>
#include <rtp/rtp_session.h>

static const OpalBandwidth DefaultInitialBandwidth = 4000000; // 4Mb/s

#define new PNEW

#define PTraceModule() "OpalEP"


/////////////////////////////////////////////////////////////////////////////

OpalEndPoint::OpalEndPoint(OpalManager & mgr,
                           const PCaselessString & prefix,
                           Attributes attributes)
  : m_manager(mgr)
  , m_prefixName(prefix)
  , m_attributes(attributes)
  , m_maxSizeUDP(4096)
  , m_productInfo(mgr.GetProductInfo())
  , m_defaultLocalPartyName(mgr.GetDefaultUserName())
  , m_defaultDisplayName(mgr.GetDefaultDisplayName())
  , m_initialRxBandwidth(DefaultInitialBandwidth)
  , m_initialTxBandwidth(DefaultInitialBandwidth)
  , m_defaultSendUserInputMode(OpalConnection::SendUserInputAsProtocolDefault)
{
  m_manager.AttachEndPoint(this);

  if (m_defaultLocalPartyName.IsEmpty())
    m_defaultLocalPartyName = PProcess::Current().GetName() & "User";

  PTRACE(4, "Created endpoint: " << m_prefixName);
}


OpalEndPoint::~OpalEndPoint()
{
  PTRACE(4, m_prefixName << " endpoint destroyed.");
}


void OpalEndPoint::ShutDown()
{
  PTRACE(3, m_prefixName << " endpoint shutting down.");

  // Shut down the listeners as soon as possible to avoid race conditions
  m_listeners.RemoveAll();
}


void OpalEndPoint::PrintOn(ostream & strm) const
{
  strm << "EP<" << m_prefixName << '>';
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


bool OpalEndPoint::SetInitialBandwidth(OpalBandwidth::Direction dir, OpalBandwidth bandwidth)
{
  switch (dir) {
    case OpalBandwidth::Rx :
      if (bandwidth < 64000)
        return false;
      m_initialRxBandwidth = bandwidth;
      break;

    case OpalBandwidth::Tx :
      if (bandwidth < 64000)
        return false;
      m_initialTxBandwidth = bandwidth;
      break;

    default :
      OpalBandwidth rx = (PUInt64)(unsigned)bandwidth*m_initialRxBandwidth/(m_initialRxBandwidth+m_initialTxBandwidth);
      OpalBandwidth tx = (PUInt64)(unsigned)bandwidth*m_initialTxBandwidth/(m_initialRxBandwidth+m_initialTxBandwidth);
      if (rx < 64000 || tx < 64000)
        return false;
      m_initialRxBandwidth = rx;
      m_initialTxBandwidth = tx;
  }

  return true;
}


PBoolean OpalEndPoint::GarbageCollection()
{
  for (ConnectionDict::iterator it = m_connectionsActive.begin(); it != m_connectionsActive.end(); ++it) {
    PTRACE_CONTEXT_ID_PUSH_THREAD(it->second);
    it->second->GarbageCollection();
  }

  return m_connectionsActive.DeleteObjectsToBeRemoved();
}


bool OpalEndPoint::StartListeners(const PStringArray & listenerAddresses, bool add)
{
  OpalTransportAddressArray interfaces;
  if (listenerAddresses.IsEmpty()) {
    interfaces = GetDefaultListeners();
    if (interfaces.IsEmpty()) {
      PTRACE(1, "No default listener interfaces specified for " << GetPrefixName());
      return false;
    }
  }
  else {
    for (PINDEX i = 0; i < listenerAddresses.GetSize(); i++) {
      if (listenerAddresses[i].Find('$') != P_MAX_INDEX)
        interfaces.AppendAddress(listenerAddresses[i]);
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
          interfaces.AppendAddress(OpalTransportAddress(listenerAddresses[i], port, transport));
        }
      }
    }
  }

  bool atLeastOne = false;

  // Stop listeners not in list
  if (!add) {
    for (OpalListenerList::iterator it = m_listeners.begin(); it != m_listeners.end(); ) {
      bool removeListener = true;
      for (PINDEX i = 0; i < interfaces.GetSize(); i++) {
        if (it->GetLocalAddress().IsEquivalent(interfaces[i])) {
          interfaces.RemoveAt(i);
          removeListener = false;
          atLeastOne = true;
          ++it;
          break;
        }
      }
      if (removeListener)
        m_listeners.erase(it++);
    }
  }

  for (PINDEX i = 0; i < interfaces.GetSize(); i++) {
    if (StartListener(interfaces[i]))
      atLeastOne = true;
  }

  return atLeastOne;
}


bool OpalEndPoint::StartListener(const PString & listenerAddress)
{
  OpalListener * listener;

  OpalTransportAddress iface = listenerAddress;

  if (iface.IsEmpty()) {
    PStringArray interfaces = GetDefaultListeners();
    if (interfaces.IsEmpty()) {
      PTRACE(1, "No default listener interfaces specified for " << GetPrefixName());
      return false;
    }
    iface = OpalTransportAddress(interfaces[0], GetDefaultSignalPort());
  }

  // Check for already listening
  for (OpalListenerList::iterator it = m_listeners.begin(); it != m_listeners.end(); ++it) {
    if (it->GetLocalAddress().IsEquivalent(iface)) {
      PTRACE(4, "Already listening on " << iface);
      return true;
    }
  }

  listener = iface.CreateListener(*this, OpalTransportAddress::FullTSAP);
  if (listener == NULL) {
    PTRACE(1, "Could not create listener: " << iface);
    return false;
  }

  return StartListener(listener);
}


bool OpalEndPoint::StartListener(OpalListener * listener)
{
  if (PAssertNULL(listener) == NULL)
    return false;

  OpalListenerUDP * udpListener = dynamic_cast<OpalListenerUDP *>(listener);
  if (udpListener != NULL)
    udpListener->SetBufferSize(m_maxSizeUDP);

  // as the listener is not open, this will have the effect of immediately
  // stopping the listener thread. This is good - it means that the 
  // listener Close function will appear to have stopped the thread
  if (!listener->Open(PCREATE_NOTIFIER(NewIncomingConnection))) {
    PTRACE(1, "Could not start listener: " << *listener);
    delete listener;
    return false;
  }

  m_listeners.Append(listener);
  return true;
}


PString OpalEndPoint::GetDefaultTransport() const
{
  return PString::Empty();
}


WORD OpalEndPoint::GetDefaultSignalPort() const
{
  return 0;
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
      listenerAddresses += OpalTransportAddress(PIPSocket::Address::GetAny(4), port, transProto);
#if OPAL_PTLIB_IPV6
      listenerAddresses += OpalTransportAddress(PIPSocket::Address::GetAny(6), port, transProto);
#endif
    }
  }
  return listenerAddresses;
}


OpalListener * OpalEndPoint::FindListener(const OpalTransportAddress & iface)
{
  for (OpalListenerList::iterator listener = m_listeners.begin(); listener != m_listeners.end(); ++listener) {
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
    return m_listeners.Remove(listener);

  m_listeners.RemoveAll();
  return true;
}


static void AddTransportAddress(OpalTransportAddressArray & interfaceAddresses,
                                const OpalTransportAddress & listenerAddress)
{
  if (interfaceAddresses.GetValuesIndex(listenerAddress) == P_MAX_INDEX)
    interfaceAddresses.Append(new OpalTransportAddress(listenerAddress));
}


static void AddTransportAddresses(OpalTransportAddressArray & interfaceAddresses,
                                  const OpalListenerList & listeners,
                                  const OpalTransportAddress & remoteAddress,
                                  const OpalTransportAddress & interfaceAddress)
{
  for (OpalListenerList::const_iterator listener = listeners.begin(); listener != listeners.end(); ++listener) {
    if (listener->GetLocalAddress().IsEquivalent(interfaceAddress, true))
      AddTransportAddress(interfaceAddresses, listener->GetLocalAddress(remoteAddress, interfaceAddress));
  }
}


OpalTransportAddressArray OpalEndPoint::GetInterfaceAddresses(const OpalTransport * associatedTransport) const
{
  OpalTransportAddressArray interfaceAddresses;

  if (associatedTransport != NULL) {
    OpalTransportAddress remoteAddress = associatedTransport->GetRemoteAddress();
    PIPSocket::Address associatedInterfaceIP(associatedTransport->GetInterface());
    if (!associatedInterfaceIP.IsValid())
      associatedTransport->GetLocalAddress().GetIpAddress(associatedInterfaceIP);
    AddTransportAddresses(interfaceAddresses, m_listeners, remoteAddress,
                          OpalTransportAddress(associatedInterfaceIP, 65535, remoteAddress.GetProtoPrefix()));
    AddTransportAddresses(interfaceAddresses, m_listeners, remoteAddress,
                          OpalTransportAddress(associatedInterfaceIP, 65535, OpalTransportAddress::IpPrefix()));
  }

  PIPSocket::InterfaceTable interfaceTable;
  for (OpalListenerList::const_iterator listener = m_listeners.begin(); listener != m_listeners.end(); ++listener) {
    OpalTransportAddress listenerInterface = listener->GetLocalAddress();
    PIPSocket::Address listenerIP;
    WORD listenerPort = 0;
    if (!listenerInterface.GetIpAndPort(listenerIP, listenerPort) || !listenerIP.IsAny())
      AddTransportAddress(interfaceAddresses, listenerInterface);
    else {
      if (interfaceTable.IsEmpty())
        PIPSocket::GetInterfaceTable(interfaceTable);
      for (PINDEX i = 0; i < interfaceTable.GetSize(); ++i) {
        if (!interfaceTable[i].GetAddress().IsLoopback())
          AddTransportAddress(interfaceAddresses, OpalTransportAddress(interfaceTable[i].GetAddress(), listenerPort));
      }
    }
  }

#if PTRACING
  if (PTrace::CanTrace(4) && !interfaceAddresses.IsEmpty()) {
    ostream & trace = PTRACE_BEGIN(4, "OpalMan");
    trace << "Listener interfaces: ";
    if (associatedTransport == NULL)
      trace << "no ";
    trace << "associated transport";
    if (associatedTransport != NULL)
      trace << ' ' << *associatedTransport;
    trace << "\n    " << setfill(',') << interfaceAddresses
          << PTrace::End;
  }
#endif

  return interfaceAddresses;
}


void OpalEndPoint::NewIncomingConnection(OpalListener & /*listener*/, const OpalTransportPtr & /*transport*/)
{
}


PSafePtr<OpalConnection> OpalEndPoint::GetConnectionWithLock(const PString & token, PSafetyMode mode) const
{
  if (token.IsEmpty() || token == "*")
    return PSafePtr<OpalConnection>(m_connectionsActive, mode);

  PSafePtr<OpalConnection> connection = m_connectionsActive.Find(token, mode);
  if (connection != NULL)
    return connection;

  PSafePtr<OpalCall> call = m_manager.FindCallWithLock(token, PSafeReadOnly);
  if (call != NULL) {
    for (PINDEX i = 0; (connection = call->GetConnection(i)) != NULL; ++i) {
      if (&connection->GetEndPoint() == this)
        return connection.SetSafetyMode(mode) ? connection : NULL;
    }
  }

  if (token.NumCompare(GetPrefixName()+':') != EqualTo)
    return NULL;

  PString name = token.Mid(GetPrefixName().GetLength()+1);
  for (connection = PSafePtr<OpalConnection>(m_connectionsActive, PSafeReference); connection != NULL; ++connection) {
    if (connection->GetLocalPartyName() == name)
      return connection.SetSafetyMode(mode) ? connection : NULL;
  }

  return NULL;
}


PStringList OpalEndPoint::GetAllConnections()
{
  PStringList tokens;

  for (PSafePtr<OpalConnection> connection(m_connectionsActive, PSafeReadOnly); connection != NULL; ++connection)
    tokens.AppendString(connection->GetToken());

  return tokens;
}


PBoolean OpalEndPoint::HasConnection(const PString & token)
{
  return m_connectionsActive.Contains(token);
}


OpalConnection * OpalEndPoint::AddConnection(OpalConnection * connection)
{
  if (connection == NULL)
    return NULL;

  PString token = connection->GetToken();
  if (m_connectionsActive.Contains(token)) {
    PTRACE(2, "Cannot add connection, duplicate token: " << token);
    delete connection;
    return NULL;
  }

  connection->SetStringOptions(m_defaultStringOptions, false);

  m_connectionsActive.SetAt(token, connection);

  OnNewConnection(connection->GetCall(), *connection);

  return connection;
}


void OpalEndPoint::DestroyConnection(OpalConnection * connection)
{
  delete connection;
}


PBoolean OpalEndPoint::OnSetUpConnection(OpalConnection & PTRACE_PARAM(connection))
{
  PTRACE(3, "OnSetUpConnection " << connection);
  return true;
}


PBoolean OpalEndPoint::OnIncomingConnection(OpalConnection & connection, unsigned options, OpalConnection::StringOptions * stringOptions)
{
  return m_manager.OnIncomingConnection(connection, options, stringOptions);
}


void OpalEndPoint::OnProceeding(OpalConnection & connection)
{
  m_manager.OnProceeding(connection);
}


void OpalEndPoint::OnAlerting(OpalConnection & connection, bool withMedia)
{
  m_manager.OnAlerting(connection, withMedia);
}


void OpalEndPoint::OnAlerting(OpalConnection & connection)
{
  m_manager.OnAlerting(connection);
}

OpalConnection::AnswerCallResponse
       OpalEndPoint::OnAnswerCall(OpalConnection & connection,
                                  const PString & caller)
{
  return m_manager.OnAnswerCall(connection, caller);
}

void OpalEndPoint::OnConnected(OpalConnection & connection)
{
  m_manager.OnConnected(connection);
}


void OpalEndPoint::OnEstablished(OpalConnection & connection)
{
  m_manager.OnEstablished(connection);
}


void OpalEndPoint::OnReleased(OpalConnection & connection)
{
  PTRACE(4, "OnReleased " << connection);
  m_connectionsActive.RemoveAt(connection.GetToken());
  m_manager.OnReleased(connection);
}


void OpalEndPoint::OnHold(OpalConnection & connection, bool fromRemote, bool onHold)
{
  connection.GetCall().OnHold(connection, fromRemote, onHold);
}


void OpalEndPoint::OnHold(OpalConnection & connection)
{
  m_manager.OnHold(connection);
}


PBoolean OpalEndPoint::OnForwarded(OpalConnection & connection,
			       const PString & forwardParty)
{
  PTRACE(4, "OnForwarded " << connection);
  return m_manager.OnForwarded(connection, forwardParty);
}


bool OpalEndPoint::OnTransferNotify(OpalConnection & connection, const PStringToString & info, const OpalConnection * transferringConnection)
{
  if (&connection != transferringConnection)
    return false;

  bool stayConnected = false;
  PSafePtr<OpalConnection> otherConnection = connection.GetOtherPartyConnection();
  if (otherConnection != NULL)
    stayConnected = otherConnection->OnTransferNotify(info, transferringConnection);

  return m_manager.OnTransferNotify(connection, info) || stayConnected;
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
  return m_manager.ClearCall(token, reason, sync);
}


PBoolean OpalEndPoint::ClearCallSynchronous(const PString & token,
                                            OpalConnection::CallEndReason reason,
                                            PSyncPoint * sync)
{
  PSyncPoint syncPoint;
  if (sync == NULL)
    sync = &syncPoint;

  if (!ClearCall(token, reason, sync))
    return false;

  PTRACE(5, "Synchronous wait for " << token);
  sync->Wait();
  return true;
}


void OpalEndPoint::ClearAllCalls(OpalConnection::CallEndReason reason, PBoolean wait)
{
  m_manager.ClearAllCalls(reason, wait);
}


void OpalEndPoint::AdjustMediaFormats(bool local,
                                      const OpalConnection & connection,
                                      OpalMediaFormatList & mediaFormats) const
{
  m_manager.AdjustMediaFormats(local, connection, mediaFormats);
}


bool OpalEndPoint::GetMediaTransportAddresses(const OpalConnection & provider,
                                              const OpalConnection & consumer,
                                                          unsigned   sessionId,
                                               const OpalMediaType & mediaType,
                                         OpalTransportAddressArray & transports) const
{
  return m_manager.GetMediaTransportAddresses(provider, consumer, sessionId, mediaType, transports);
}


PBoolean OpalEndPoint::OnOpenMediaStream(OpalConnection & connection,
                                     OpalMediaStream & stream)
{
  return m_manager.OnOpenMediaStream(connection, stream);
}


void OpalEndPoint::OnClosedMediaStream(const OpalMediaStream & stream)
{
  m_manager.OnClosedMediaStream(stream);
}


void OpalEndPoint::OnFailedMediaStream(OpalConnection & connection, bool fromRemote, const PString & reason)
{
  m_manager.OnFailedMediaStream(connection, fromRemote, reason);
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


void OpalEndPoint::OnUserInputString(OpalConnection & connection,
                                     const PString & value)
{
  m_manager.OnUserInputString(connection, value);
}


void OpalEndPoint::OnUserInputTone(OpalConnection & connection,
                                   char tone,
                                   int duration)
{
  m_manager.OnUserInputTone(connection, tone, duration);
}


PString OpalEndPoint::ReadUserInput(OpalConnection & connection,
                                    const char * terminators,
                                    unsigned lastDigitTimeout,
                                    unsigned firstDigitTimeout)
{
  return m_manager.ReadUserInput(connection, terminators, lastDigitTimeout, firstDigitTimeout);
}


void OpalEndPoint::OnNewConnection(OpalCall & call, OpalConnection & connection)
{
  call.OnNewConnection(connection);
}


void OpalEndPoint::OnMWIReceived(const PString & party,
                                 OpalManager::MessageWaitingType type,
                                 const PString & extraInfo)
{
  m_manager.OnMWIReceived(party, type, extraInfo);
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

  const PStringList prefixes = m_manager.GetPrefixNames(this);

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
  for (OpalListenerList::iterator listener = m_listeners.begin(); listener != m_listeners.end(); ++listener) {
    addr = listener->GetLocalAddress();
    if (addr.IsCompatible(compatibleTo))
      return true;
   }
  return false;
}

#if OPAL_PTLIB_SSL
bool OpalEndPoint::ApplySSLCredentials(PSSLContext & context, bool create) const
{
  return m_manager.ApplySSLCredentials(*this, context, create);
}
#endif


#if OPAL_HAS_IM

bool OpalEndPoint::Message(const PString & to, const PString & body)
{
  return m_manager.Message(to, body);
}


PBoolean OpalEndPoint::Message(
  const PURL & to, 
  const PString & type,
  const PString & body,
  PURL & from, 
  PString & conversationId
)
{
  return m_manager.Message(to, type, body, from, conversationId);
}


PBoolean OpalEndPoint::Message(OpalIM & message)
{
  return m_manager.Message(message);
}


void OpalEndPoint::OnMessageReceived(const OpalIM & message)
{
  m_manager.OnMessageReceived(message);
}

#endif // OPAL_HAS_IM


PStringList OpalEndPoint::GetAvailableStringOptions() const
{
  static char const * const StringOpts[] = {
    OPAL_OPT_AUTO_START,
    OPAL_OPT_CALL_IDENTIFIER,
    OPAL_OPT_CALLING_PARTY_URL,
    OPAL_OPT_CALLING_PARTY_NUMBER,
    OPAL_OPT_CALLING_PARTY_NAME,
    OPAL_OPT_CALLING_PARTY_DOMAIN,
    OPAL_OPT_CALLING_DISPLAY_NAME,
    OPAL_OPT_CALLED_PARTY_NAME,
    OPAL_OPT_CALLED_DISPLAY_NAME,
    OPAL_OPT_REDIRECTING_PARTY,
    OPAL_OPT_PRESENTATION_BLOCK,
    OPAL_OPT_INTERFACE,
    OPAL_OPT_USER_INPUT_MODE,
    OPAL_OPT_ENABLE_INBAND_DTMF,
    OPAL_OPT_ENABLE_INBAND_DTMF,
    OPAL_OPT_DETECT_INBAND_DTMF,
    OPAL_OPT_SEND_INBAND_DTMF,
    OPAL_OPT_DTMF_MULT,
    OPAL_OPT_DTMF_DIV,
    OPAL_OPT_DISABLE_JITTER,
    OPAL_OPT_MAX_JITTER,
    OPAL_OPT_MIN_JITTER,
    OPAL_OPT_RECORD_AUDIO,
    OPAL_OPT_ALERTING_TYPE,
    OPAL_OPT_REMOVE_CODEC,
    OPAL_OPT_SILENCE_DETECT_MODE,
    OPAL_OPT_VIDUP_METHODS,
    OPAL_OPT_MEDIA_RX_TIMEOUT,
    OPAL_OPT_MEDIA_TX_TIMEOUT
  };

  return PStringList(PARRAYSIZE(StringOpts), StringOpts, true);
}


const PIPSocket::QoS & OpalEndPoint::GetMediaQoS(const OpalMediaType & type) const
{
  OpalManager::MediaQoSMap::const_iterator it = m_mediaQoS.find(type);
  return it != m_mediaQoS.end() ? it->second : m_manager.GetMediaQoS(type);
}


void OpalEndPoint::SetMediaQoS(const OpalMediaType & type, const PIPSocket::QoS & qos)
{
  m_mediaQoS[type] = qos;
}


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
