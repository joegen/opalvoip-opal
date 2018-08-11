/*
 * transport.cxx
 *
 * Opal transports handler
 *
 * Open Phone Abstraction Library (OPAL)
 * Formally known as the Open H323 project.
 *
 * Copyright (c) 2001 Equivalence Pty. Ltd.
 * Portions Copyright (C) 2006 by Post Increment
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
 * Contributor(s): Post Increment
 *     Portions of this code were written with the assistance of funding from
 *     US Joint Forces Command Joint Concept Development & Experimentation (J9)
 *     http://www.jfcom.mil/about/abt_j9.htm
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "transports.h"
#endif

#include <opal_config.h>

#include <opal/transports.h>
#include <opal/manager.h>
#include <opal/endpoint.h>
#include <opal/call.h>
#include <opal_config.h>

#include <ptclib/pnat.h>
#include <ptclib/http.h>


const PCaselessString & OpalTransportAddress::IpPrefix()  { static PConstCaselessString s("ip$" ); return s; }  // For backward compatibility with OpenH323
PFACTORY_CREATE(PFactory<OpalInternalTransport>, OpalInternalTCPTransport, OpalTransportAddress::IpPrefix(), true);

const PCaselessString & OpalTransportAddress::UdpPrefix() { static PConstCaselessString s("udp$"); return s; }
PFACTORY_CREATE(PFactory<OpalInternalTransport>, OpalInternalUDPTransport, OpalTransportAddress::UdpPrefix(), true);

const PCaselessString & OpalTransportAddress::TcpPrefix() { static PConstCaselessString s("tcp$"); return s; }
PFACTORY_SYNONYM(PFactory<OpalInternalTransport>, OpalInternalTCPTransport, TCP, OpalTransportAddress::TcpPrefix ());

#if OPAL_PTLIB_SSL

#include <ptclib/pssl.h>
const PCaselessString & OpalTransportAddress::TlsPrefix() { static PConstCaselessString s("tls$"); return s; }
PFACTORY_CREATE(PFactory<OpalInternalTransport>, OpalInternalTLSTransport, OpalTransportAddress::TlsPrefix(), true);

#if OPAL_PTLIB_HTTP

const PCaselessString & OpalTransportAddress::WsPrefix()  { static PConstCaselessString s("ws$");  return s; }
PFACTORY_CREATE(PFactory<OpalInternalTransport>, OpalInternalWSTransport, OpalTransportAddress::WsPrefix(), true);

const PCaselessString & OpalTransportAddress::WssPrefix() { static PConstCaselessString s("wss$"); return s; }
PFACTORY_CREATE(PFactory<OpalInternalTransport>, OpalInternalWSSTransport, OpalTransportAddress::WssPrefix(), true);

#endif // OPAL_PTLIB_HTTP

#endif // OPAL_PTLIB_SSL

/////////////////////////////////////////////////////////////////

#define new PNEW
#define PTraceModule() "OpalTrans"

/////////////////////////////////////////////////////////////////

OpalTransportAddress::OpalTransportAddress()
  : m_transport(NULL)
{
}


OpalTransportAddress::OpalTransportAddress(const char * cstr,
                                           WORD port,
                                           const char * proto)
  : PCaselessString(cstr)
{
  SetInternalTransport(port, proto);
}


OpalTransportAddress::OpalTransportAddress(const PString & str,
                                           WORD port,
                                           const char * proto)
  : PCaselessString(str)
{
  SetInternalTransport(port, proto);
}


OpalTransportAddress::OpalTransportAddress(const PIPSocket::Address & addr, WORD port, const char * proto)
  : PCaselessString(addr.IsValid() ? addr.AsString(true) : PString('*'))
{
  SetInternalTransport(port, proto);
}


OpalTransportAddress::OpalTransportAddress(const PIPSocket::AddressAndPort & addr, const char * proto)
  : PCaselessString(addr.IsValid() ? addr.AsString() : PString('*'))
{
  SetInternalTransport(addr.GetPort(), proto);
}


PString OpalTransportAddress::GetHostName(bool includeService) const
{
  if (m_transport == NULL)
    return *this;

  return m_transport->GetHostName(*this, includeService);
}


PBoolean OpalTransportAddress::IsEquivalent(const OpalTransportAddress & address, bool wildcard) const
{
  if (*this == address)
    return true;

  if (IsEmpty() || address.IsEmpty())
    return false;

  PCaselessString myPrefix = GetProtoPrefix();
  PCaselessString theirPrefix = address.GetProtoPrefix();
  PIPSocket::Address ip1, ip2;
  WORD port1 = 65535, port2 = 65535;
  return GetIpAndPort(ip1, port1) &&
         address.GetIpAndPort(ip2, port2) &&
         ((ip1 *= ip2) || (wildcard && (ip1.IsAny() || ip2.IsAny()))) &&
         (port1 == port2 || (wildcard && (port1 == 65535 || port2 == 65535))) &&
         (myPrefix == theirPrefix || (wildcard && (myPrefix == IpPrefix() || theirPrefix == IpPrefix())));
}


PBoolean OpalTransportAddress::IsCompatible(const OpalTransportAddress & address) const
{
  if (IsEmpty() || address.IsEmpty())
    return true;

  PCaselessString myPrefix = GetProtoPrefix();
  if (myPrefix == UdpPrefix() ||
#if OPAL_PTLIB_SSL
      myPrefix == TlsPrefix() ||
#endif
      myPrefix == TcpPrefix())
    myPrefix = IpPrefix();

  PCaselessString theirPrefix = address.GetProtoPrefix();
  if (theirPrefix == UdpPrefix() ||
#if OPAL_PTLIB_SSL
      theirPrefix == TlsPrefix() ||
#endif
      theirPrefix == TcpPrefix())
    theirPrefix = IpPrefix();

  if (myPrefix != theirPrefix)
    return false;

  if (myPrefix != IpPrefix())
    return true;

  if (GetHostName() == "*" || address.GetHostName() == "*")
    return true;

  PIPSocket::Address myIP, theirIP;
  return GetIpAddress(myIP) && address.GetIpAddress(theirIP) && myIP.GetVersion() == theirIP.GetVersion();
}


PBoolean OpalTransportAddress::GetIpAddress(PIPSocket::Address & ip) const
{
  if (m_transport == NULL)
    return false;

  WORD dummy = 65535;
  return m_transport->GetIpAndPort(*this, ip, dummy);
}


PBoolean OpalTransportAddress::GetIpAndPort(PIPSocket::Address & ip, WORD & port) const
{
  if (m_transport == NULL)
    return false;

  return m_transport->GetIpAndPort(*this, ip, port);
}


PBoolean OpalTransportAddress::GetIpAndPort(PIPSocketAddressAndPort & ipPort) const
{
  if (m_transport == NULL)
    return false;

  PIPSocket::Address ip;
  WORD port = 0;
  if (!m_transport->GetIpAndPort(*this, ip, port))
    return false;

  // Use 2 calls here as 2nd arg to SetAddress will not set zero
  ipPort.SetAddress(ip);
  ipPort.SetPort(port);
  return true;
}


OpalListener * OpalTransportAddress::CreateListener(OpalEndPoint & endpoint,
                                                    BindOptions option) const
{
  if (m_transport == NULL)
    return NULL;

  return m_transport->CreateListener(*this, endpoint, option);
}


OpalTransport * OpalTransportAddress::CreateTransport(OpalEndPoint & endpoint,
                                                      BindOptions option) const
{
  if (m_transport == NULL)
    return NULL;

  return m_transport->CreateTransport(*this, endpoint, option);
}


void OpalTransportAddress::SetInternalTransport(WORD port, const char * proto)
{
  m_transport = NULL;
  
  if (IsEmpty())
    return;

  PINDEX dollar = Find('$');
  if (dollar == P_MAX_INDEX) {
    PString prefix(proto);
    if (prefix.IsEmpty())
      prefix = TcpPrefix();
    else if (prefix.Find('$') == P_MAX_INDEX)
      prefix += '$';

    Splice(prefix, 0);
    dollar = prefix.GetLength()-1;
  }

#if OPAL_PTLIB_SSL
  if (NumCompare("tcps$") == EqualTo) // For backward compatibility
    Splice(TlsPrefix(), 0, 5);
#endif

  // use factory to create transport types
  m_transport = PFactory<OpalInternalTransport>::CreateInstance(GetProtoPrefix().ToLower());
  if (m_transport != NULL && m_transport->Parse(*this, port))
    return;

  PTRACE(1, "Could not parse transport address \"" << *this << '"');
  MakeEmpty();
}


/////////////////////////////////////////////////////////////////

bool OpalTransportAddressArray::AppendString(const PString & str)
{
  return AppendStringCollection(str.Tokenise(",;\t\n"));
}


bool OpalTransportAddressArray::AppendAddress(const OpalTransportAddress & addr)
{
  if (addr.IsEmpty())
    return false;

  Append(new OpalTransportAddress(addr));
  return true;
}


bool OpalTransportAddressArray::SetAddressPair(const OpalTransportAddress & addr1, const OpalTransportAddress & addr2)
{
  RemoveAll();

  if (!AppendAddress(addr1))
    return false;

  if (addr1 != addr2)
    AppendAddress(addr2);

  return true;
}


void OpalTransportAddressArray::GetIpAndPort(PINDEX index, PIPAddressAndPort & ap)
{
  if (ap.IsValid())
    return;

  if (!IsEmpty() && (*this)[index % GetSize()].GetIpAndPort(ap))
    return;

  ap.SetAddress(PIPSocket::GetDefaultIpAny());
  ap.SetPort(0);
}


bool OpalTransportAddressArray::AppendStringCollection(const PCollection & coll)
{
  bool atLeastOne = false;
  for (PINDEX idx = 0; idx < coll.GetSize(); idx++) {
    PString * str = dynamic_cast<PString *>(coll.GetAt(idx));
    if (str != NULL) {
      PCaselessString addr = *str;

      if (addr.NumCompare("0.0.0.0") == EqualTo)
        atLeastOne = AddInterfaces(PIPAddress::GetAny(4), PIPAddress::GetAny(4), addr.Mid(7)) || atLeastOne;
      else if (addr.NumCompare("[::]") == EqualTo)
        atLeastOne = AddInterfaces(PIPAddress::GetAny(6), PIPAddress::GetAny(6), addr.Mid(4)) || atLeastOne;
      else if (addr.NumCompare("<<ip4>>") == EqualTo)
        atLeastOne = AddInterfaces(PIPAddress::GetAny(4), PIPAddress::GetAny(4), addr.Mid(7)) || atLeastOne;
      else if (addr.NumCompare("<<ip6>>") == EqualTo)
        atLeastOne = AddInterfaces(PIPAddress::GetAny(6), PIPAddress::GetAny(6), addr.Mid(7)) || atLeastOne;
      else {
        PINDEX star = addr.Find('*');
        if (star == P_MAX_INDEX)
          atLeastOne = AppendAddress(addr) || atLeastOne;
        else if (star == 0) {
          atLeastOne = AddInterfaces(PIPAddress::GetAny(4), PIPAddress::GetAny(4), addr.Mid(1)) || atLeastOne;
          atLeastOne = AddInterfaces(PIPAddress::GetAny(6), PIPAddress::GetAny(6), addr.Mid(1)) || atLeastOne;
        }
        else {
          PStringArray components = addr.Left(star).Tokenise('.');
          if (components.GetSize() < 2) {
            PTRACE(1, "Illegal IP wildcard address used: \"" << addr << '"');
            continue;
          }

          PStringStream network;
          PStringStream mask;
          network << components[0];
          mask << "255";
          PINDEX count = components.GetSize()-1;
          if (components[count].IsEmpty())
            --count;
          PINDEX i = 0;
          while (++i <= count) {
            if (!components[i].IsEmpty()) {
              network << '.' << components[i];
              mask << ".255";
            }
          }
          while (i++ < 4) {
            network << ".0";
            mask << ".0";
          }

          atLeastOne = AddInterfaces(PIPAddress(network), PIPAddress(mask), addr.Mid(star + 1)) || atLeastOne;
        }
      }
    }
  }
  return atLeastOne;
}


bool OpalTransportAddressArray::AddInterfaces(const PIPAddress & network, const PIPAddress & mask, const PString & portStr)
{
  bool atLeastOne = false;
  PINDEX colon = portStr.Find(':');
  WORD port = (WORD)(colon != P_MAX_INDEX ? portStr.Mid(colon + 1).AsUnsigned() : 0);

  PIPSocket::InterfaceTable interfaces;
  PIPSocket::GetInterfaceTable(interfaces);

  for (PINDEX i = 0; i < interfaces.GetSize(); ++i) {
    PIPAddress addr = interfaces[i].GetAddress();
    if (addr.IsSubNet(network, mask) && !addr.IsLoopback() && AppendAddress(OpalTransportAddress(addr, port)))
      atLeastOne = true;
  }
  return atLeastOne;
}


/////////////////////////////////////////////////////////////////

PString OpalInternalTransport::GetHostName(const OpalTransportAddress & address, bool) const
{
  // skip transport identifier
  PINDEX pos = address.Find('$');
  if (pos == P_MAX_INDEX)
    return address;

  return address.Mid(pos+1);
}


PBoolean OpalInternalTransport::GetIpAndPort(const OpalTransportAddress &,
                                         PIPSocket::Address &,
                                         WORD &) const
{
  return false;
}


//////////////////////////////////////////////////////////////////////////

static PBoolean SplitAddress(const PString & addr, PString & host, PString & device, PString & service)
{
  // skip transport identifier
  PINDEX i = addr.Find('$');
  if (i == P_MAX_INDEX) {
    //PTRACE(1, "Address " << addr << " has no dollar");
    return false;
  }

  host.MakeEmpty();
  device.MakeEmpty();
  service.MakeEmpty();

  PINDEX j = ++i;

  // parse interface/host
  bool isInterface = (addr[j] == '%') || (addr[j] == '[' && addr[j+1] == '%');

  if (j == '\0') {
    //PTRACE(1, "Address " << addr << " has empty host");
    return false;
  }
  i = j;
  bool bracketed = addr[j] == '[';
  for (;;) {
    if (addr[j] == '\0')
      break;
    if (!bracketed) {
      if (addr[j] == ':')
        break;
    } else if (addr[j] == ']') {
      ++j;
      break;
    }
    j++;
  }
  if (i == j) {
    //PTRACE(1, "Address " << addr << " has invalid host " << i);
    return false;
  }

  if (!isInterface)
    host = addr(i, j-1);
  else {
    if (addr[i] == '[' && addr[i+1] == '%') {
      device = '%';
      device += addr(i+2, j-2);
    }
    else
      device = addr(i, j-1);
  }
  
  // parse optional service
  if (addr[j] == ':') {
    i = ++j;
    for (;;) {
      if (addr[j] == '\0')
        break;
      j++;
    }
    // cannot have zero length service
    if (i == j) {
      //PTRACE(1, "Address " << addr << " has invalid service " << i << " " << j);
      return false;
    }
    service = addr(i, j-1);
  }

  //PTRACE(1, "Split " << addr << " into host='" << host << "',dev='" << device << "',service='" << service << "'");

  return true;
}


bool OpalInternalIPTransport::Parse(OpalTransportAddress & address, WORD port) const
{
  // Get port, even if string is bracketed, i.e. udp$[2001...]:1720
  PINDEX dollar = address.Find('$');
  PINDEX rbracket = address.Find(']');
  if (rbracket != P_MAX_INDEX) {
    if (address.Find('[') > rbracket)
      return false;
    dollar = rbracket + 1;
  }

  if (address.Find(':', dollar) != P_MAX_INDEX)
    return true;

  if (port == 0)
    return true;

  PINDEX end = address.GetLength();
  if (address[end-1] == '+')
    end--;
  address.Splice(psprintf(":%u", port), end);
  return true;
}


PString OpalInternalIPTransport::GetHostName(const OpalTransportAddress & address, bool includeService) const
{
  PString host, device, service;
  if (!SplitAddress(address, host, device, service))
    return address;

  PString hostname = host+device;

  if (device.IsEmpty()) {
    PIPSocket::Address ip;
    if (ip.FromString(host))
      hostname = ip.AsString(true);
  }

  if (includeService)
    hostname += ':' + service;
  return hostname;
}


PBoolean OpalInternalIPTransport::GetIpAndPort(const OpalTransportAddress & address,
                                           PIPSocket::Address & ip,
                                           WORD & port) const
{
  PString host, device, service;
  if (!SplitAddress(address, host, device, service))
    return PFalse;

  if (host.IsEmpty() && device.IsEmpty()) {
    PTRACE(2, "Illegal IP transport address: \"" << address << '"');
    return PFalse;
  }

  WORD newPort;
  if (service.IsEmpty())
    newPort = port;
  else if (service == "*")
    newPort = 0;
  else {
    PCaselessString proto = address.GetProtoPrefix();
    if (proto == OpalTransportAddress::IpPrefix())
      proto = OpalTransportAddress::TcpPrefix();
    if ((newPort = PIPSocket::GetPortByService(proto, service)) == 0) {
      PTRACE(2, "Illegal IP transport port/service: \"" << address << '"');
      return false;
    }
  }

  if (host[0] == '*')
    ip = PIPSocket::GetDefaultIpAny();
  else if (host == "0.0.0.0")
    ip = PIPSocket::Address::GetAny(4);
  else if (host == "::" || host == "[::]")
    ip = PIPSocket::Address::GetAny(6);
  else if (device.IsEmpty()) {
    if (!PIPSocket::GetHostAddress(host, ip)) {
      PTRACE(1, "Could not find host \"" << host << '"');
      return false;
    }
  }
  else {
    if (!ip.FromString(device)) {
      PTRACE(1, "Could not find device \"" << device << '"');
      return false;
    }
  }

  port = newPort;
  return true;
}


//////////////////////////////////////////////////////////////////////////

PBoolean OpalInternalIPTransport::GetAdjustedIpAndPort(const OpalTransportAddress & address,
                                 OpalEndPoint & endpoint,
                                 OpalTransportAddress::BindOptions option,
                                 PIPSocketAddressAndPort & ap,
                                 PBoolean & reuseAddr)
{
  reuseAddr = address[address.GetLength()-1] == '+';

  ap.SetPort(0);
  PIPAddress ip;
  switch (option) {
    case OpalTransportAddress::NoBinding :
      if (address.GetIpAddress(ip))
        ap.SetAddress(PIPSocket::Address::GetAny(ip.GetVersion()));
      else
        ap.SetAddress(PIPSocket::GetDefaultIpAny());
      return true;

    case OpalTransportAddress::HostOnly :
      if (!address.GetIpAddress(ip))
        return false;
      ap.SetAddress(ip);
      return true;

    case OpalTransportAddress::RouteInterface :
      if (address.GetIpAddress(ip))
        ap.SetAddress(PIPSocket::GetRouteInterfaceAddress(ip));
      else
        ap.SetAddress(PIPSocket::GetDefaultIpAny());
      ap.SetPort(0);
      return TRUE;

    default :
      ap.SetPort(endpoint.GetDefaultSignalPort());
      return address.GetIpAndPort(ap);
  }
}


//////////////////////////////////////////////////////////////////////////

OpalListener::OpalListener(OpalEndPoint & ep)
  : m_endpoint(ep)
  , m_thread(NULL)
  , m_threadMode(SpawnNewThreadMode)
{
}


void OpalListener::PrintOn(ostream & strm) const
{
  strm << GetLocalAddress();
}


void OpalListener::CloseWait()
{
  PTRACE(3, "Stopping listening thread on " << GetLocalAddress());
  Close();

  PThread::WaitAndDelete(m_thread, m_endpoint.GetManager().GetSignalingTimeout()+2000);
}


void OpalListener::ListenForConnections()
{
  PTRACE(3, "Started listening thread on " << GetLocalAddress());
  PAssert(!m_acceptHandler.IsNULL(), PLogicError);

  while (IsOpen()) {
    OpalTransport * transport = Accept(PMaxTimeInterval);
    if (transport == NULL)
      m_acceptHandler(*this, 0);
    else {
      switch (m_threadMode) {
        case SpawnNewThreadMode :
          transport->AttachThread(new PThreadObj1Arg<OpalListener, OpalTransportPtr>(
                        *this, transport, &OpalListener::TransportThreadMain, false, "Opal Answer"));
          break;

        case HandOffThreadMode :
          transport->AttachThread(m_thread);
          m_thread = NULL;
          // Then do next case

        case SingleThreadMode :
          m_acceptHandler(*this, transport);
      }
      // Note: acceptHandler is responsible for deletion of the transport
    }
  }
}


bool OpalListener::Open(const AcceptHandler & theAcceptHandler, ThreadMode mode)
{
  m_acceptHandler = theAcceptHandler;
  m_threadMode = mode;

  m_thread = new PThreadObj<OpalListener>(*this, &OpalListener::ListenForConnections, false, "Opal Listener");

  return m_thread != NULL;
}


void OpalListener::TransportThreadMain(OpalTransportPtr transport)
{
  PTRACE_IF(3, transport != NULL, "Handling accepted connection: " << *transport);
  m_acceptHandler(*this, transport);
}


//////////////////////////////////////////////////////////////////////////

OpalListenerIP::OpalListenerIP(OpalEndPoint & ep,
                               PIPSocket::Address binding,
                               WORD port,
                               PBoolean exclusive)
  : OpalListener(ep)
  , m_binding(binding, port)
  , m_exclusiveListener(exclusive)
{
}


OpalListenerIP::OpalListenerIP(OpalEndPoint & endpoint,
                               const OpalTransportAddress & binding,
                               OpalTransportAddress::BindOptions option)
  : OpalListener(endpoint)
{
  OpalInternalIPTransport::GetAdjustedIpAndPort(binding, endpoint, option, m_binding, m_exclusiveListener);
}


OpalTransportAddress OpalListenerIP::GetLocalAddress(const OpalTransportAddress & remoteAddress,
                                                     const OpalTransportAddress & defaultAddress) const
{
  PIPSocket::Address localIP = m_binding.GetAddress();

  PIPSocket::Address remoteIP;
  if (remoteAddress.GetIpAddress(remoteIP)) {
    if ((!localIP.IsValid() || localIP.IsAny()) && (!defaultAddress.GetIpAddress(localIP) || localIP.IsAny()))
      localIP = PIPSocket::GetRouteInterfaceAddress(remoteIP);
    m_endpoint.GetManager().TranslateIPAddress(localIP, remoteIP);
  }

  return OpalTransportAddress(localIP, m_binding.GetPort(), GetProtoPrefix());
}


bool OpalListenerIP::CanCreateTransport(const OpalTransportAddress & localAddress,
                                        const OpalTransportAddress & remoteAddress) const
{
  if (remoteAddress.GetProtoPrefix() != GetProtoPrefix()) {
    PTRACE(5, "Remote protocol (" << remoteAddress.GetProtoPrefix() << ") different to listener (" << GetProtoPrefix() << ')');
    return false;
  }

  // The following then checks for IPv4/IPv6
  OpalTransportAddress myLocalAddress = GetLocalAddress();
  if (!myLocalAddress.IsCompatible(remoteAddress)) {
    PTRACE(5, "Remote address (" << remoteAddress << ") not compatible to listener (" << myLocalAddress << ')');
    return false;
  }

  if (localAddress.IsEmpty() || myLocalAddress.IsCompatible(localAddress))
    return true;
  
  PTRACE(5, "Local address (" << localAddress << ") not compatible to listener (" << myLocalAddress << ')');
  return false;
}


//////////////////////////////////////////////////////////////////////////

OpalListenerTCP::OpalListenerTCP(OpalEndPoint & ep,
                                 PIPSocket::Address binding,
                                 WORD port,
                                 PBoolean exclusive)
  : OpalListenerIP(ep, binding, port, exclusive)
{
}


OpalListenerTCP::OpalListenerTCP(OpalEndPoint & endpoint,
                                 const OpalTransportAddress & binding,
                                 OpalTransportAddress::BindOptions option)
  : OpalListenerIP(endpoint, binding, option)
{
}


OpalListenerTCP::~OpalListenerTCP()
{
  CloseWait();
}


PBoolean OpalListenerTCP::Open(const AcceptHandler & theAcceptHandler, ThreadMode mode)
{
  if (m_binding.GetPort() == 0) {
    OpalManager & manager = m_endpoint.GetManager();
    if (manager.GetTCPPortRange().Listen(m_listener, m_binding.GetAddress(), 1)) {
      m_binding.SetPort(m_listener.GetPort());
      return OpalListenerIP::Open(theAcceptHandler, mode);
    }
  }
  else {
    if (m_listener.Listen(m_binding.GetAddress(), 10, m_binding.GetPort(), m_exclusiveListener ? PSocket::AddressIsExclusive : PSocket::CanReuseAddress))
      return OpalListenerIP::Open(theAcceptHandler, mode);
  }

  PTRACE(1, "Open (" << (m_exclusiveListener ? "EXCLUSIVE" : "REUSEADDR") << ") on "
         << m_binding << " failed: " << m_listener.GetErrorText());
  return false;
}


bool OpalListenerTCP::IsOpen() const
{
  return m_listener.IsOpen();
}


void OpalListenerTCP::Close()
{
  m_listener.Close();
}


OpalTransport * OpalListenerTCP::Accept(const PTimeInterval & timeout)
{
  if (!m_listener.IsOpen())
    return NULL;

  m_listener.SetReadTimeout(timeout); // Wait for remote connect

  PTRACE(4, "Waiting on socket accept on " << GetLocalAddress());
  PTCPSocket * socket = new PTCPSocket;
  if (socket->Accept(m_listener))
    return OnAccept(socket);

  if (socket->GetErrorCode() != PChannel::Interrupted) {
    PTRACE(1, "Accept error:" << socket->GetErrorText());
    m_listener.Close();
  }

  delete socket;
  return NULL;
}


OpalTransport * OpalListenerTCP::OnAccept(PTCPSocket * socket)
{
  return new OpalTransportTCP(m_endpoint, socket);
}


OpalTransport * OpalListenerTCP::CreateTransport(const OpalTransportAddress & localAddress,
                                                 const OpalTransportAddress & remoteAddress) const
{
  if (!CanCreateTransport(localAddress, remoteAddress))
    return NULL;

  if (localAddress.IsEmpty())
    return new OpalTransportTCP(m_endpoint);

  return localAddress.CreateTransport(m_endpoint, OpalTransportAddress::HostOnly);
}


const PCaselessString & OpalListenerTCP::GetProtoPrefix() const
{
  return OpalTransportAddress::TcpPrefix();
}


//////////////////////////////////////////////////////////////////////////

OpalListenerUDP::OpalListenerUDP(OpalEndPoint & endpoint,
                                 PIPSocket::Address binding,
                                 WORD port,
                                 PBoolean exclusive)
  : OpalListenerIP(endpoint, binding, port, exclusive)
  , m_listenerBundle(PMonitoredSockets::Create(binding.AsString(),
                                               !exclusive
                                               P_NAT_PARAM(&endpoint.GetManager().GetNatMethods())))
{
}


OpalListenerUDP::OpalListenerUDP(OpalEndPoint & endpoint,
                                 const OpalTransportAddress & binding,
                                 OpalTransportAddress::BindOptions option)
  : OpalListenerIP(endpoint, binding, option)
  , m_listenerBundle(PMonitoredSockets::Create(binding.GetHostName(),
                                               !m_exclusiveListener
                                               P_NAT_PARAM(&endpoint.GetManager().GetNatMethods())))
  , m_bufferSize(endpoint.GetMaxSizeUDP())
{
  if (binding.GetHostName() == "*")
    m_binding.SetAddress(PIPSocket::GetInvalidAddress()); // Set invalid to distinguish between "*", "0.0.0.0" and "[::]"
}


OpalListenerUDP::~OpalListenerUDP()
{
  CloseWait();
}


PBoolean OpalListenerUDP::Open(const AcceptHandler & theAcceptHandler, ThreadMode /*mode*/)
{
  if (m_listenerBundle != NULL &&
      m_listenerBundle->Open(m_binding.GetPort()) &&
      OpalListenerIP::Open(theAcceptHandler, SingleThreadMode)) {
    m_binding.SetPort(m_listenerBundle->GetPort());
    /* UDP packets need to be handled. Not so much at high speed, but must not be
       significantly delayed by media threads which are running at HighPriority.
       This, for example, helps make sure that a SIP BYE is received and processed
       to kill a call where codecs etc in the media threads are hogging all the CPU. */
    m_thread->SetPriority(PThread::HighestPriority);
    return true;
  }

  PTRACE(1, "Could not start any UDP listeners for port " << m_binding.GetPort());
  return false;
}


bool OpalListenerUDP::IsOpen() const
{
  return m_listenerBundle != NULL && m_listenerBundle->IsOpen();
}


void OpalListenerUDP::Close()
{
  if (m_listenerBundle != NULL)
    m_listenerBundle->Close();
}


OpalTransport * OpalListenerUDP::Accept(const PTimeInterval & timeout)
{
  if (!IsOpen())
    return NULL;

  PBYTEArray pdu;
  PMonitoredSockets::BundleParams param;
  param.m_buffer = pdu.GetPointer(m_bufferSize);
  param.m_length = m_bufferSize;
  param.m_timeout = timeout;
  m_listenerBundle->ReadFromBundle(param);

  if (param.m_errorCode != PChannel::NoError && param.m_errorCode != PChannel::BufferTooSmall)
    return NULL;

  pdu.SetSize(param.m_lastCount);

  OpalTransportUDP * transport = new OpalTransportUDP(m_endpoint, m_listenerBundle, param.m_iface,
                                                      OpalTransportAddress(param.m_addr, param.m_port, OpalTransportAddress::UdpPrefix()));
  transport->m_preReadPacket = pdu;
  transport->m_preReadOK = param.m_errorCode == PChannel::NoError;
  transport->GetChannel()->SetBufferSize(m_bufferSize);
  return transport;
}


OpalTransport * OpalListenerUDP::CreateTransport(const OpalTransportAddress & localAddress,
                                                 const OpalTransportAddress & remoteAddress) const
{
  if (!CanCreateTransport(localAddress, remoteAddress))
    return NULL;

  if (!IsOpen())
    return NULL;

  PString iface;
  if (!localAddress.IsEmpty()) {
    PIPSocket::Address addr;
    if (localAddress.GetIpAddress(addr))
      iface = addr.AsString(true);
  }

  return new OpalTransportUDP(m_endpoint, m_listenerBundle, iface, remoteAddress);
}


const PCaselessString & OpalListenerUDP::GetProtoPrefix() const
{
  return OpalTransportAddress::UdpPrefix();
}


OpalTransportAddress OpalListenerUDP::GetLocalAddress(const OpalTransportAddress & remoteAddress,
                                                      const OpalTransportAddress & defaultAddress) const
{
  if (IsOpen()) {
    PIPSocket::Address remoteIP;
    if (remoteAddress.GetIpAddress(remoteIP)) {
      PIPSocket::Address ip;
      WORD port;
      defaultAddress.GetIpAddress(ip);
      if (m_listenerBundle->GetAddress(ip.AsString(), ip, port, !m_endpoint.GetManager().IsLocalAddress(remoteIP)))
        return OpalTransportAddress(ip, port, GetProtoPrefix());
    }
  }

  return OpalListenerIP::GetLocalAddress(remoteAddress, defaultAddress);
}


//////////////////////////////////////////////////////////////////////////

OpalTransport::OpalTransport(OpalEndPoint & end, PChannel * channel)
  : m_endpoint(end)
  , m_channel(channel)
  , m_thread(NULL)
  , m_idleTimer(m_endpoint.GetManager().GetTransportIdleTime())
  , m_referenceCount(0)
{
  m_keepAliveTimer.SetNotifier(PCREATE_NOTIFIER(KeepAlive), "OpalTransKeepAlive");
  PTRACE(5, "Transport constructed: this=" << this << ", channel=" << m_channel << ", ep=" << m_endpoint);
}


OpalTransport::~OpalTransport()
{
  PTRACE(5, "Transport destroyed: " << m_channel);
  PAssert(m_thread == NULL, PLogicError);
  delete m_channel;
}


void OpalTransport::PrintOn(ostream & strm) const
{
  strm << "rem=" << GetRemoteAddress() << ", ";

  PString iface(GetInterface());
  OpalTransportAddress local(GetLocalAddress());

  if (local.IsEmpty() && iface.IsEmpty())
    strm << "not-bound";
  else if (local.Find(iface.Left(iface.Find('%'))) != P_MAX_INDEX)
    strm << "if=" << local;
  else
    strm << "rfx=" << local << ", if=" << iface;
}


bool OpalTransport::SetInterface(const PString &)
{
  return true;
}


PBoolean OpalTransport::Close()
{
  m_keepAliveTimer.Stop();

  /* Do not use PIndirectChannel::Close() as this deletes the sub-channel
     member field crashing the background thread. Just close the base
     sub-channel so breaks the threads I/O block.
   */
  PTRACE_IF (4, IsOpen(), "Transport close of " << *this << ", this=" << this << ", channel=" << m_channel);
  return m_channel->Close();
}


void OpalTransport::CloseWait()
{
  PTRACE_IF(3, m_thread != NULL, "Transport clean up on termination for " << *this);

  Close();

  if (!LockReadWrite())
    return;
  PThread * exitingThread = m_thread;
  m_thread = NULL;
  UnlockReadWrite();

  m_keepAliveTimer.Stop();

  PThread::WaitAndDelete(exitingThread, m_endpoint.GetManager().GetSignalingTimeout()+2000);
}


PBoolean OpalTransport::IsCompatibleTransport(const OpalTransportAddress &) const
{
  PAssertAlways(PUnimplementedFunction);
  return false;
}


void OpalTransport::SetPromiscuous(PromisciousModes /*promiscuous*/)
{
}


OpalTransportAddress OpalTransport::GetLastReceivedAddress() const
{
  return GetRemoteAddress();
}


PString OpalTransport::GetLastReceivedInterface() const
{
  return GetLocalAddress();
}


PBoolean OpalTransport::WriteConnect(const WriteConnectCallback & function)
{
  bool succeeded = false;
  function(*this, succeeded);
  return succeeded;
}


void OpalTransport::AttachThread(PThread * thrd)
{
  PTRACE_CONTEXT_ID_TO(thrd);

  PThread::WaitAndDelete(m_thread);

  m_thread = thrd;
}


PBoolean OpalTransport::IsRunning() const
{
  if (m_thread == NULL)
    return false;

  return !m_thread->IsTerminated();
}


bool OpalTransport::IsOpen() const
{
  PSafeLockReadOnly lock(*this);
  return lock.IsLocked() && m_channel != NULL && m_channel->IsOpen();
}


bool OpalTransport::IsGood() const
{
  PSafeLockReadOnly lock(*this);
  return lock.IsLocked() && m_channel != NULL && m_channel->IsOpen() && !m_channel->bad() && !m_channel->eof();
}


PChannel::Errors OpalTransport::GetErrorCode(PChannel::ErrorGroup group) const
{
  PSafeLockReadOnly lock(*this);
  return lock.IsLocked() && m_channel != NULL ? m_channel->GetErrorCode(group) : PChannel::NotOpen;
}


PString OpalTransport::GetErrorText(PChannel::ErrorGroup group) const
{
  PSafeLockReadOnly lock(*this);
  return lock.IsLocked() && m_channel != NULL ? m_channel->GetErrorText(group) : PString::Empty();
}


int OpalTransport::GetErrorNumber(PChannel::ErrorGroup group) const
{
  PSafeLockReadOnly lock(*this);
  return lock.IsLocked() && m_channel != NULL ? m_channel->GetErrorNumber(group) : -1;
}


void OpalTransport::SetReadTimeout(const PTimeInterval & t)
{
  PSafeLockReadOnly lock(*this);
  if (lock.IsLocked() && m_channel != NULL)
    m_channel->SetReadTimeout(t);
}


void OpalTransport::SetKeepAlive(const PTimeInterval & timeout, const PBYTEArray & data)
{
  if (!LockReadWrite())
    return;

  m_keepAliveData = data;
  UnlockReadWrite();

  PSocket * socket = dynamic_cast<PSocket *>(m_channel);
  if (socket != NULL && !socket->SetOption(SO_KEEPALIVE, timeout > 0)) {
    PTRACE(1, "SetOption(SO_KEEPALIVE) failed: " << socket->GetErrorText());
  }

  if (timeout <= 0 || data.IsEmpty()) {
    m_keepAliveTimer.Stop(false);
    PTRACE(4, "Transport keep alive disabled on " << *this);
  }
  else {
    static const PTimeInterval MinKeepAlive(0, 10);
    if (timeout < MinKeepAlive) {
      PTRACE(4, "Transport keep alive (" << data.GetSize() << " bytes) set for minimum " << MinKeepAlive << " seconds on " << *this);
      m_keepAliveTimer.RunContinuous(MinKeepAlive);
    }
    else {
      PTRACE(4, "Transport keep alive (" << data.GetSize() << " bytes) set for " << timeout << " seconds on " << *this);
      m_keepAliveTimer.RunContinuous(timeout);
    }
  }
}


void OpalTransport::KeepAlive(PTimer &, P_INT_PTR)
{
  if (!IsOpen()) {
    PTRACE(4, "Keep alive attempt while not open on " << *this);
    return;
  }

  PTRACE(5, "Sending keep alive (" << m_channel->GetLastWriteCount() << " bytes) on " << *this);
  if (!LockReadOnly())
    return;

  if (!Write(m_keepAliveData, m_keepAliveData.GetSize())) {
    PTRACE(2, "Transport keep alive failed on " << *this << ": " << m_channel->GetErrorText(PChannel::LastWriteError));
  }

  UnlockReadOnly();
}


PBoolean OpalTransport::Write(const void * buf, PINDEX len)
{
  if (!IsOpen())
    return false;

  ResetIdle();
  m_keepAliveTimer.Reset();
  return m_channel->Write(buf, len);
}


void OpalTransport::ResetIdle()
{
  m_idleTimer = m_endpoint.GetManager().GetTransportIdleTime();
}


/////////////////////////////////////////////////////////////////////////////

OpalTransportIP::OpalTransportIP(OpalEndPoint & end,
                                 PChannel * channel,
                                 PIPSocket::Address binding,
                                 WORD port)
  : OpalTransport(end, channel)
  , m_binding(binding)
  , m_localAP(binding, port)
  , m_remoteAP(PIPSocket::Address::GetAny(binding.GetVersion()))
{
}


PString OpalTransportIP::GetInterface() const
{
  PSafeLockReadOnly lock(*this);
  if (lock.IsLocked()) {
    PIPSocket * socket = dynamic_cast<PIPSocket *>(m_channel->GetBaseReadChannel());
    if (socket != NULL && socket->IsOpen()) {
      PIPSocket::Address ip;
      if (socket->PIPSocket::GetLocalAddress(ip))
        return ip.AsString();
    }
  }

  return PString::Empty();
}


OpalTransportAddress OpalTransportIP::GetLocalAddress() const
{
  return OpalTransportAddress(m_localAP, GetProtoPrefix());
}


OpalTransportAddress OpalTransportIP::GetRemoteAddress() const
{
  return OpalTransportAddress(m_remoteAP, GetProtoPrefix());
}


PBoolean OpalTransportIP::SetRemoteAddress(const OpalTransportAddress & address)
{
  if (IsCompatibleTransport(address))
    return address.GetIpAndPort(m_remoteAP);

  PTRACE(2, "Attempt to set incompatible transport " << address);
  return false;
}


/////////////////////////////////////////////////////////////////////////////

OpalTransportTCP::OpalTransportTCP(OpalEndPoint & ep,
                                   PIPSocket::Address binding,
                                   WORD port,
                                   PBoolean)
  : OpalTransportIP(ep, new PTCPSocket(port), binding, port)
  , m_pduLengthFormat(0)
  , m_pduLengthOffset(0)
{
}


OpalTransportTCP::OpalTransportTCP(OpalEndPoint & ep, PChannel * channel)
  : OpalTransportIP(ep, channel, INADDR_ANY, 0)
  , m_pduLengthFormat(0)
  , m_pduLengthOffset(0)
{
  if (channel != NULL)
    OnConnectedSocket(dynamic_cast<PTCPSocket *>(channel->GetBaseReadChannel()));
}


OpalTransportTCP::~OpalTransportTCP()
{
  CloseWait();
  PTRACE(4,"Deleted transport " << *this);
}


PBoolean OpalTransportTCP::IsReliable() const
{
  return true;
}


PBoolean OpalTransportTCP::IsCompatibleTransport(const OpalTransportAddress & address) const
{
  return (address.NumCompare(OpalTransportAddress::TcpPrefix()) == EqualTo) ||
         (address.NumCompare(OpalTransportAddress::IpPrefix())  == EqualTo);
}


PBoolean OpalTransportTCP::Connect()
{
  if (IsOpen())
    return true;

  PSafeLockReadWrite mutex(*this);

  PTCPSocket * socket = dynamic_cast<PTCPSocket *>(m_channel);
  if (!PAssert(socket != NULL, PLogicError))
    return false;

  socket->SetPort(m_remoteAP.GetPort());

  OpalManager & manager = m_endpoint.GetManager();
  socket->SetReadTimeout(manager.GetSignalingTimeout());

  PTRACE(4, "Connecting to " << m_remoteAP << " on interface " << m_binding);
  if (!manager.GetTCPPortRange().Connect(*socket, m_remoteAP.GetAddress(), m_binding)) {
    PTRACE(1, "Could not connect to " << m_remoteAP
              << " (range=" << manager.GetTCPPortRange() << ") - "
              << socket->GetErrorText() << '(' << socket->GetErrorNumber() << ')');
    return false;
  }

  return OnConnectedSocket(socket);
}


PBoolean OpalTransportTCP::ReadPDU(PBYTEArray & pdu)
{
  BYTE header[8];
  if (!m_channel->Read(header, 1))
    return false;

  // Save timeout
  PTimeInterval oldTimeout = m_channel->GetReadTimeout();

  // Should get all of PDU in reasonable time or something is seriously wrong,
  m_channel->SetReadTimeout(m_endpoint.GetManager().GetSignalingTimeout());

  bool ok = false;
  int packetLength = 0;

  if (m_pduLengthFormat != 0) {
    size_t count = std::abs(m_pduLengthFormat);
    if (PAssert(count > 0 && count <= sizeof(header), "Invalid PDU length") && m_channel->Read(header + 1, count - 1)) {
      packetLength = 0;
      while (count-- > 0)
        packetLength |= header[count] << (8 * (m_pduLengthFormat < 0 ? count : (m_pduLengthFormat - count - 1)));
      if (m_pduLengthOffset > 0 || packetLength > -m_pduLengthOffset) {
        packetLength += m_pduLengthOffset;
        ok = true;
      }
      else {
        PTRACE(2, "Dwarf PDU received (length " << packetLength << ")");
      }
    }
  }
  else if (header[0] != 3) // Make sure is a RFC1006 TPKT version 3
    m_channel->SetErrorValues(PChannel::ProtocolFailure, 0x80000000);
  else if (m_channel->ReadBlock(header + 1, 3)) { // Get TPKT header
    packetLength = ((header[2] << 8) | header[3]);
    if (packetLength >= 4) {
      packetLength -= 4;
      ok = true;
    }
    else {
      PTRACE(2, "Dwarf TPKT received (length " << packetLength << ")");
    }
  }

  if (ok && packetLength > 0)
    ok = m_channel->ReadBlock(pdu.GetPointer(packetLength), packetLength);

  m_channel->SetReadTimeout(oldTimeout);

  return ok;
}


PBoolean OpalTransportTCP::WritePDU(const PBYTEArray & pdu)
{
  // We copy the data into a new buffer so we can do a single write call. This
  // is necessary as we have disabled the Nagle TCP delay algorithm to improve
  // network performance.

  int packetLength = pdu.GetSize() + 4;

  // Send RFC1006 TPKT length
  PBYTEArray tpkt(packetLength);
  tpkt[0] = 3;
  tpkt[1] = 0;
  tpkt[2] = (BYTE)(packetLength >> 8);
  tpkt[3] = (BYTE)packetLength;
  memcpy(tpkt.GetPointer()+4, (const BYTE *)pdu, pdu.GetSize());

  return Write((const BYTE *)tpkt, packetLength);
}


bool OpalTransportTCP::OnConnectedSocket(PTCPSocket * socket)
{
  if (socket == NULL)
    return false;

  OpalManager & manager = m_endpoint.GetManager();

  // If write take longer than this, something is wrong
  const PTimeInterval timeout = manager.GetSignalingTimeout();
  m_channel->SetWriteTimeout(timeout);

  // Initial read timeout to idle time plus a buffer
  m_channel->SetReadTimeout(manager.GetTransportIdleTime()+10000);

  // Get name of the remote computer for information purposes
  if (!socket->GetPeerAddress(m_remoteAP)) {
    PTRACE(1, "GetPeerAddress() failed: " << socket->GetErrorText());
    return false;
  }

  // get local address of incoming socket to ensure that multi-homed machines
  // use a NIC address that is guaranteed to be addressable to destination
  PIPSocket::Address ip;
  WORD port;
  if (!socket->GetLocalAddress(ip, port)) {
    PTRACE(1, "GetLocalAddress() failed: " << socket->GetErrorText());
    return false;
  }

  manager.TranslateIPAddress(ip, m_remoteAP.GetAddress());
  m_localAP.SetAddress(ip, port);

  if (!socket->SetOption(TCP_NODELAY, 1, IPPROTO_TCP)) {
    PTRACE(1, "SetOption(TCP_NODELAY) failed: " << socket->GetErrorText());
  }

  // make sure do not lose outgoing packets on close
  const linger ling = { 1, (u_short)timeout.GetSeconds() };
  if (!socket->SetOption(SO_LINGER, &ling, sizeof(ling))) {
    PTRACE(1, "SetOption(SO_LINGER) failed: " << socket->GetErrorText());
    return false;
  }

  socket->SetQoS(m_endpoint.GetSignalQoS());

  m_channel->clear();

  PTRACE(3, "Started connection: rem=" << m_remoteAP << " (if=" << m_localAP << ')');
  return true;
}


const PCaselessString & OpalTransportTCP::GetProtoPrefix() const
{
  return OpalTransportAddress::TcpPrefix();
}


/////////////////////////////////////////////////////////////////////////////

OpalTransportUDP::OpalTransportUDP(OpalEndPoint & ep,
                                   PIPSocket::Address binding,
                                   WORD localPort,
                                   bool reuseAddr,
                                   bool preOpen)
  : OpalTransportIP(ep, NULL, binding, localPort)
  , m_manager(ep.GetManager())
  , m_bufferSize(ep.GetMaxSizeUDP())
  , m_preReadOK(false)
{
  PMonitoredSockets * sockets = PMonitoredSockets::Create(binding.AsString(),
                                                          reuseAddr
                                                          P_NAT_PARAM(&m_manager.GetNatMethods()));
  if (preOpen)
    sockets->Open(localPort);
  m_channel = new PMonitoredSocketChannel(sockets, false);
}


OpalTransportUDP::OpalTransportUDP(OpalEndPoint & ep,
                                   const PMonitoredSocketsPtr & listener,
                                   const PString & iface,
                                   const OpalTransportAddress & remoteAddress)
  : OpalTransportIP(ep, new PMonitoredSocketChannel(listener, true), PIPSocket::GetDefaultIpAny(), 0)
  , m_manager(ep.GetManager())
  , m_bufferSize(ep.GetMaxSizeUDP())
  , m_preReadOK(true)
{
  if (!remoteAddress.GetIpAndPort(m_remoteAP)) {
    PTRACE(3, "Could not use remote address for UDP transport");
  }

  PIPAddress remoteIP = m_remoteAP.GetAddress();
  bool remoteNotLocal = !m_manager.IsLocalAddress(remoteIP);

  PString adjustedInterface = iface;
  if (iface.IsEmpty()) {
    PIPAddress ifIP = PIPSocket::GetRouteInterfaceAddress(remoteIP);
    if (ifIP.IsValid()) {
      adjustedInterface = ifIP.AsString();
      PTRACE(4, "Created UDP transport using router: " << adjustedInterface);
    }
    else if (remoteNotLocal) {
      adjustedInterface = '%' + PIPSocket::GetGatewayInterface(remoteIP.GetVersion());
      PTRACE(4, "Created UDP transport using default gateway " << adjustedInterface);
    }
    else {
      PTRACE(4, "Cannot determine interface to use for " << remoteIP);
    }
  }
  else {
    PTRACE(4, "Created UDP transport using interface " << iface);
  }

  PMonitoredSocketChannel * socket = dynamic_cast<PMonitoredSocketChannel *>(m_channel);
  socket->SetInterface(adjustedInterface);
  socket->GetLocal(m_localAP, remoteNotLocal);
  socket->SetRemote(m_remoteAP);
}


OpalTransportUDP::~OpalTransportUDP()
{
  CloseWait();
  PTRACE(4,"Deleted transport " << *this);
}


PBoolean OpalTransportUDP::IsReliable() const
{
  return false;
}


PBoolean OpalTransportUDP::IsCompatibleTransport(const OpalTransportAddress & address) const
{
  return (address.NumCompare(OpalTransportAddress::UdpPrefix()) == EqualTo) ||
         (address.NumCompare(OpalTransportAddress::IpPrefix())  == EqualTo);
}


PBoolean OpalTransportUDP::Connect()
{	
  if (m_remoteAP.GetPort() == 0)
    return false;

  if (m_remoteAP.GetAddress().IsAny())
	  m_remoteAP.SetAddress(PIPSocket::Address::GetBroadcast(m_remoteAP.GetAddress().GetVersion()));

  if (m_remoteAP.GetAddress().IsBroadcast()) {
    PTRACE(3, "Broadcast connect to port " << m_remoteAP.GetPort());
  }
  else {
    PTRACE(3, "Started connect to " << m_remoteAP);
  }

  if (PAssertNULL(m_channel) == NULL)
    return false;

  PMonitoredSocketsPtr bundle = dynamic_cast<PMonitoredSocketChannel *>(m_channel)->GetMonitoredSockets();
  if (bundle->IsOpen()) {
    bundle->SetQoS(m_endpoint.GetSignalQoS());
    return true;
  }

  if (!bundle->Open(m_localAP.GetPort())) {
    PTRACE(1, "Could not bind to port " << m_localAP.GetPort());
    return false;
  }

  bundle->SetQoS(m_endpoint.GetSignalQoS());

  m_localAP.SetPort(bundle->GetPort());
  return true;
}


PString OpalTransportUDP::GetInterface() const
{
  PMonitoredSocketChannel * socket = dynamic_cast<PMonitoredSocketChannel *>(m_channel);
  if (socket != NULL)
    return socket->GetInterface();

  PTRACE(4, "No interface as no bundled socket");
  return OpalTransportIP::GetInterface();
}


bool OpalTransportUDP::SetInterface(const PString & iface)
{
  PMonitoredSocketChannel * socket = dynamic_cast<PMonitoredSocketChannel *>(m_channel);
  if (socket == NULL) {
    PTRACE(2, "Could not set interface to " << iface);
    return false;
  }
    
  PTRACE(4, "Setting interface to " << iface);
  socket->SetInterface(iface);
  return true;
}


OpalTransportAddress OpalTransportUDP::GetLocalAddress() const
{
  PMonitoredSocketChannel * socket = dynamic_cast<PMonitoredSocketChannel *>(m_channel);
  if (socket != NULL) {
    OpalTransportUDP * thisWritable = const_cast<OpalTransportUDP *>(this);
    if (!socket->GetLocal(thisWritable->m_localAP, !m_manager.IsLocalAddress(m_remoteAP.GetAddress())))
      return OpalTransportAddress();
  }

  return OpalTransportIP::GetLocalAddress();
}


PBoolean OpalTransportUDP::SetRemoteAddress(const OpalTransportAddress & address)
{
  if (!OpalTransportIP::SetRemoteAddress(address))
    return false;

  PMonitoredSocketChannel * socket = dynamic_cast<PMonitoredSocketChannel *>(m_channel);
  if (socket != NULL)
    socket->SetRemote(m_remoteAP);

  return true;
}


void OpalTransportUDP::SetPromiscuous(PromisciousModes promiscuous)
{
  PMonitoredSocketChannel * socket = dynamic_cast<PMonitoredSocketChannel *>(m_channel);
  if (socket != NULL) {
    socket->SetPromiscuous(promiscuous != AcceptFromRemoteOnly);
    if (promiscuous == AcceptFromAnyAutoSet)
      socket->SetRemote(PIPSocket::GetDefaultIpAny(), 0);
  }
}


OpalTransportAddress OpalTransportUDP::GetLastReceivedAddress() const
{
  PMonitoredSocketChannel * socket = dynamic_cast<PMonitoredSocketChannel *>(m_channel);
  if (socket != NULL) {
    PIPSocket::Address addr;
    WORD port;
    socket->GetLastReceived(addr, port);
    if (!addr.IsAny() && port != 0)
      return OpalTransportAddress(addr, port, OpalTransportAddress::UdpPrefix());
  }

  return OpalTransport::GetLastReceivedAddress();
}


PString OpalTransportUDP::GetLastReceivedInterface() const
{
  PString iface;

  PMonitoredSocketChannel * socket = dynamic_cast<PMonitoredSocketChannel *>(m_channel);
  if (socket != NULL)
    iface = socket->GetLastReceivedInterface();

  if (iface.IsEmpty())
    iface = OpalTransport::GetLastReceivedInterface();

  return iface;
}


PBoolean OpalTransportUDP::Read(void * buffer, PINDEX length)
{
  if (m_preReadPacket.IsEmpty())
    return m_channel->Read(buffer, length);

  memcpy(buffer, m_preReadPacket, PMIN(length, m_preReadPacket.GetSize()));
  m_preReadPacket.SetSize(0);

  return m_preReadOK;
}


PBoolean OpalTransportUDP::ReadPDU(PBYTEArray & packet)
{
  if (m_preReadPacket.GetSize() > 0) {
    packet = m_preReadPacket;
    m_preReadPacket.SetSize(0);
    return m_preReadOK;
  }

  if (!m_channel->Read(packet.GetPointer(m_bufferSize), m_bufferSize)) {
    packet.SetSize(0);
    return false;
  }

  packet.SetSize(m_channel->GetLastReadCount());
  return true;
}


PBoolean OpalTransportUDP::WritePDU(const PBYTEArray & packet)
{
  return Write((const BYTE *)packet, packet.GetSize());
}


bool OpalTransportUDP::WriteConnect(const WriteConnectCallback & function)
{
  PMonitoredSocketChannel * socket = dynamic_cast<PMonitoredSocketChannel *>(m_channel);
  if (socket == NULL)
    return false;

  PMonitoredSocketsPtr bundle = socket->GetMonitoredSockets();
  PIPSocket::Address address;
  GetRemoteAddress().GetIpAddress(address);
  PStringArray interfaces = bundle->GetInterfaces(false, address);

  PBoolean ok = false;
  for (PINDEX i = 0; i < interfaces.GetSize(); i++) {
    PIPSocket::Address ifip(interfaces[i]);
    if (ifip.GetVersion() != m_remoteAP.GetAddress().GetVersion())
      PTRACE(4, "Skipping incompatible interface " << i << " - \"" << interfaces[i] << '"');
    else {
      PTRACE(4, "Writing to interface " << i << " - \"" << interfaces[i] << '"');
      socket->SetInterface(interfaces[i]);
      // Make sure is compatible address
      bool succeeded = false;
      function(*this, succeeded);
      if (succeeded)
        ok = true;
    }
  }
  socket->SetInterface(PString::Empty());

  return ok;
}


const PCaselessString & OpalTransportUDP::GetProtoPrefix() const
{
  return OpalTransportAddress::UdpPrefix();
}


//////////////////////////////////////////////////////////////////////////

#if OPAL_PTLIB_SSL

OpalListenerTLS::OpalListenerTLS(OpalEndPoint & ep,
                                 PIPSocket::Address binding,
                                 WORD port,
                                 PBoolean exclusive)
                                 : OpalListenerTCP(ep, binding, port, exclusive)
                                 , m_sslContext(NULL)
{
}


OpalListenerTLS::OpalListenerTLS(OpalEndPoint & ep,
                                 const OpalTransportAddress & binding,
                                 OpalTransportAddress::BindOptions option)
                                 : OpalListenerTCP(ep, binding, option)
                                 , m_sslContext(NULL)
{
}


OpalListenerTLS::~OpalListenerTLS()
{
  delete m_sslContext;
}


PBoolean OpalListenerTLS::Open(const AcceptHandler & acceptHandler, ThreadMode mode)
{
  m_sslContext = new PSSLContext();
  m_sslContext->SetCipherList("ALL");

  if (!m_endpoint.ApplySSLCredentials(*m_sslContext, true))
    return false;

  return OpalListenerTCP::Open(acceptHandler, mode);
}


OpalTransport * OpalListenerTLS::OnAccept(PTCPSocket * socket)
{
  PSSLChannel * ssl = new PSSLChannel(m_sslContext);
  if (ssl->Accept(socket))
    return new OpalTransportTLS(m_endpoint, ssl);

  PTRACE(1, "Accept failed: " << ssl->GetErrorText());
  delete ssl; // Will also delete socket
  return NULL;
}


const PCaselessString & OpalListenerTLS::GetProtoPrefix() const
{
  return OpalTransportAddress::TlsPrefix();
}


OpalTransport * OpalListenerTLS::CreateTransport(const OpalTransportAddress & localAddress,
                                                 const OpalTransportAddress & remoteAddress) const
{
  if (!CanCreateTransport(localAddress, remoteAddress))
    return NULL;

  PIPSocket::Address binding = PIPSocket::Address::GetAny();
  if (!localAddress.IsEmpty())
    localAddress.GetIpAddress(binding);

  return new OpalTransportTLS(m_endpoint, binding);
}


OpalTransportTLS::OpalTransportTLS(OpalEndPoint & ep,
                                   PIPSocket::Address binding,
                                   WORD port,
                                   PBoolean reuseAddr)
  : OpalTransportTCP(ep, binding, port, reuseAddr)
{
}


OpalTransportTLS::OpalTransportTLS(OpalEndPoint & ep, PChannel * ssl)
  : OpalTransportTCP(ep, ssl)
{
}


OpalTransportTLS::~OpalTransportTLS()
{
  CloseWait();
  PTRACE(4, "Deleted transport " << *this);
}


//////////////////////////////////////////////////////////////////////////

PBoolean OpalTransportTLS::IsCompatibleTransport(const OpalTransportAddress & address) const
{
  return OpalTransportTCP::IsCompatibleTransport(address) ||
         address.NumCompare(OpalTransportAddress::TlsPrefix()) == EqualTo;
}


PBoolean OpalTransportTLS::Connect()
{
  if (IsOpen())
    return true;

  PSafeLockReadWrite mutex(*this);

  delete m_channel;
  m_channel = new PTCPSocket(m_remoteAP.GetPort());
  if (!OpalTransportTCP::Connect())
    return false;

  PSSLContext * context = new PSSLContext();
  if (!m_endpoint.ApplySSLCredentials(*context, false)) {
    delete context;
    return false;
  }

  PSSLChannel * sslChannel = new PSSLChannel(context, true);

  bool ok = sslChannel->Connect(m_channel);
  PTRACE_IF(1, !ok, "Connect failed: " << sslChannel->GetErrorText());
  m_channel = sslChannel;

  return ok;
}


const PCaselessString & OpalTransportTLS::GetProtoPrefix() const
{
  return OpalTransportAddress::TlsPrefix();
}


bool OpalTransportTLS::IsAuthenticated(const PString & domain) const
{
  if (PIPSocket::Address(domain).IsValid()) {
    PTRACE(4, "Ignoring certificate AltName/CN as using IP address.");
    return true;
  }

  PSSLChannel * sslChannel = dynamic_cast<PSSLChannel *>(m_channel);
  if (sslChannel == NULL) {
    PTRACE(2, "Certificate validation failed: no SSL channel");
    return false;
  }

  PSSLCertificate cert;
  PString error;
  if (!sslChannel->GetPeerCertificate(cert, &error)) {
    PTRACE(2, "Certificate validation failed: " << error);
    return false;
  }

  PSSLCertificate::X509_Name subject;
  if (!cert.GetSubjectName(subject)) {
    PTRACE(2, "Could not get subject name from certificate!");
    return false;
  }

  PString alt = cert.GetSubjectAltName();
  PTRACE(3, "Peer certificate: alt=\"" << alt << "\", subject=\"" << subject << '"');

  if (domain == (alt.IsEmpty() ? subject.GetCommonName() : alt))
    return true;

  PTRACE(1, "Could not authenticate certificate against " << domain);
  return false;
}


//////////////////////////////////////////////////////////////////////////

#if OPAL_PTLIB_HTTP

OpalListenerWS::OpalListenerWS(OpalEndPoint & endpoint, PIPSocket::Address binding, WORD port, bool exclusive)
: OpalListenerTCP(endpoint, binding, port, exclusive)
{
}


OpalListenerWS::OpalListenerWS(OpalEndPoint & endpoint, const OpalTransportAddress & binding, OpalTransportAddress::BindOptions option)
: OpalListenerTCP(endpoint, binding, option)
{
}


static bool AcceptWS(PChannel & chan, const PString & protocol)
{
  PHTTPServer ws;
  ws.SetWebSocketNotifier(protocol, PHTTPServer::WebSocketNotifier());

  if (!ws.Open(chan))
    return false;

  while (ws.ProcessCommand())
    ;
  ws.Detach();

  return chan.IsOpen();
}


OpalTransport * OpalListenerWS::OnAccept(PTCPSocket * socket)
{
  if (AcceptWS(*socket, m_endpoint.GetPrefixName()))
    return new OpalTransportWS(m_endpoint, socket);

  delete socket;
  return NULL;
}


OpalTransport * OpalListenerWS::CreateTransport(const OpalTransportAddress & localAddress, const OpalTransportAddress & remoteAddress) const
{
  if (!CanCreateTransport(localAddress, remoteAddress))
    return NULL;

  if (localAddress.IsEmpty())
    return new OpalTransportWS(m_endpoint);

  return localAddress.CreateTransport(m_endpoint, OpalTransportAddress::HostOnly);
}


const PCaselessString & OpalListenerWS::GetProtoPrefix() const
{
  return OpalTransportAddress::WsPrefix();
}


//////////////////////////////////////////////////////////////////////////

OpalListenerWSS::OpalListenerWSS(OpalEndPoint & endpoint, PIPSocket::Address binding, WORD port, PBoolean exclusive)
  : OpalListenerTLS(endpoint, binding, port, exclusive)
{
}


OpalListenerWSS::OpalListenerWSS(OpalEndPoint & endpoint, const OpalTransportAddress & binding, OpalTransportAddress::BindOptions option)
  : OpalListenerTLS(endpoint, binding, option)
{
}


OpalTransport * OpalListenerWSS::OnAccept(PTCPSocket * socket)
{
  PSSLChannel * ssl = new PSSLChannel(m_sslContext);
  if (ssl->Accept(socket) && AcceptWS(*ssl, m_endpoint.GetPrefixName()))
    return new OpalTransportWSS(m_endpoint, ssl);

  PTRACE(1, "Accept failed: " << ssl->GetErrorText());
  delete ssl; // Will also delete socket
  return NULL;
}


const PCaselessString & OpalListenerWSS::GetProtoPrefix() const
{
  return OpalTransportAddress::WssPrefix();
}


/////////////////////////////////////////////////////////////////////////////

OpalTransportWS::OpalTransportWS(OpalEndPoint & endpoint, PIPSocket::Address binding, WORD port, bool dummy)
: OpalTransportTCP(endpoint, binding, port, dummy)
{
}


OpalTransportWS::OpalTransportWS(OpalEndPoint & endpoint, PChannel * socket)
: OpalTransportTCP(endpoint, new PWebSocket)
{
  OnConnectedSocket(dynamic_cast<PTCPSocket *>(socket));
  dynamic_cast<PWebSocket *>(m_channel)->Open(socket);

  // Due to the way WebSocket connections often work, we don't use the idle timeout
  m_channel->SetReadTimeout(PMaxTimeInterval);
}


PBoolean OpalTransportWS::IsCompatibleTransport(const OpalTransportAddress & address) const
{
    return OpalTransportTCP::IsCompatibleTransport(address) ||
               address.NumCompare(OpalTransportAddress::WsPrefix()) == EqualTo;
}


PBoolean OpalTransportWS::Connect()
{
  PSafeLockReadWrite mutex(*this);

  if (!OpalTransportTCP::Connect())
    return false;

  PWebSocket * webSocket = new PWebSocket();
  if (!webSocket->Open(m_channel)) {
    delete webSocket;
    return false;
  }
  m_channel = webSocket;
  return webSocket->Connect("ws://" + GetRemoteAddress().GetHostName(), m_endpoint.GetPrefixName());
}


PBoolean OpalTransportWS::ReadPDU(PBYTEArray & pdu)
{
  return dynamic_cast<PWebSocket *>(m_channel)->ReadMessage(pdu);
}


PBoolean OpalTransportWS::WritePDU(const PBYTEArray & pdu)
{
  return Write(pdu, pdu.GetSize());
}


const PCaselessString & OpalTransportWS::GetProtoPrefix() const
{
  return OpalTransportAddress::WsPrefix();
}


//////////////////////////////////////////////////////////////////////////

OpalTransportWSS::OpalTransportWSS(OpalEndPoint & endpoint, PIPSocket::Address binding, WORD port, bool dummy)
  : OpalTransportTLS(endpoint, binding, port, dummy)
{
}


OpalTransportWSS::OpalTransportWSS(OpalEndPoint & endpoint, PChannel * socket)
: OpalTransportTLS(endpoint, new PWebSocket)
{
  dynamic_cast<PWebSocket *>(m_channel)->Open(socket);
}


PBoolean OpalTransportWSS::IsCompatibleTransport(const OpalTransportAddress & address) const
{
    return OpalTransportTLS::IsCompatibleTransport(address) ||
               address.NumCompare(OpalTransportAddress::WssPrefix()) == EqualTo;
}


PBoolean OpalTransportWSS::Connect()
{
  PSafeLockReadWrite mutex(*this);

  if (!OpalTransportTLS::Connect())
    return false;

  PWebSocket * webSocket = new PWebSocket();
  if (!webSocket->Open(m_channel)) {
    delete webSocket;
    return false;
  }
  m_channel = webSocket;
  return webSocket->Connect("wss://" + GetRemoteAddress().GetHostName(), m_endpoint.GetPrefixName());
}


PBoolean OpalTransportWSS::ReadPDU(PBYTEArray & pdu)
{
  return dynamic_cast<PWebSocket *>(m_channel)->ReadMessage(pdu);
}


PBoolean OpalTransportWSS::WritePDU(const PBYTEArray & pdu)
{
  return Write(pdu, pdu.GetSize());
}


const PCaselessString & OpalTransportWSS::GetProtoPrefix() const
{
  return OpalTransportAddress::WssPrefix();
}


///////////////////////////////////////////////////////////////////////////////

OpalHTTPConnector::OpalHTTPConnector(OpalManager & manager, const PURL & url)
  : PHTTPResource(url)
  , m_manager(manager)
{
}


OpalHTTPConnector::OpalHTTPConnector(OpalManager & manager, const PURL & url, const PHTTPAuthority & auth)
  : PHTTPResource(url, auth)
  , m_manager(manager)
{
}


bool OpalHTTPConnector::OnWebSocket(PHTTPServer & server, PHTTPConnectionInfo & connectInfo)
{
  PURL dest(connectInfo.GetURL().GetQueryVars().Get("dest"));
  OpalEndPoint * ep = m_manager.FindEndPoint(dest.GetScheme());
  if (ep == NULL) {
    return false;
  }

  OpalTransport * transport = new OpalTransportWS(*ep, server.GetSocket());
  server.Detach();

  OpalTransportAddress addr;
  if (!ep->FindListenerForProtocol(OpalTransportAddress::WsPrefix(), addr)) {
    return false;
  }

  transport->AttachThread(new PThreadObj1Arg<OpalListener, OpalTransportPtr>(
                *ep->FindListener(addr), transport, &OpalListener::TransportThreadMain, false, "Opal Answer"));
  return false;
}

#endif // OPAL_PTLIB_HTTP
#endif // OPAL_PTLIB_SSL


///////////////////////////////////////////////////////////////////////////////
