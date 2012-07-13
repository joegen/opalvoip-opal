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
 * $Revision$
 * $Author$
 * $Date$
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "transports.h"
#endif

#include <opal/buildopts.h>

#include <opal/transports.h>
#include <opal/manager.h>
#include <opal/endpoint.h>
#include <opal/call.h>
#include <opal/buildopts.h>

#include <ptclib/pnat.h>


const PCaselessString & OpalTransportAddress::IpPrefix()  { static PConstCaselessString s("ip$" ); return s; }  // For backward compatibility with OpenH323
const PCaselessString & OpalTransportAddress::UdpPrefix() { static PConstCaselessString s("udp$"); return s; }
const PCaselessString & OpalTransportAddress::TcpPrefix() { static PConstCaselessString s("tcp$"); return s; }

static PFactory<OpalInternalTransport>::Worker<OpalInternalTCPTransport> opalInternalTCPTransportFactory(OpalTransportAddress::TcpPrefix(), true);
static PFactory<OpalInternalTransport>::Worker<OpalInternalTCPTransport> opalInternalIPTransportFactory (OpalTransportAddress::IpPrefix (), true);
static PFactory<OpalInternalTransport>::Worker<OpalInternalUDPTransport> opalInternalUDPTransportFactory(OpalTransportAddress::UdpPrefix(), true);

#if OPAL_PTLIB_SSL
#include <ptclib/pssl.h>
const PCaselessString & OpalTransportAddress::TlsPrefix() { static PConstCaselessString s("tls$"); return s; }
static PFactory<OpalInternalTransport>::Worker<OpalInternalTLSTransport> opalInternalTLSTransportFactory(OpalTransportAddress::TlsPrefix(), true);
#endif

/////////////////////////////////////////////////////////////////

#define new PNEW

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


PString OpalTransportAddress::GetHostName(bool includeService) const
{
  if (m_transport == NULL)
    return *this;

  return m_transport->GetHostName(*this, includeService);
}


PBoolean OpalTransportAddress::IsEquivalent(const OpalTransportAddress & address, bool wildcard) const
{
  if (*this == address)
    return PTrue;

  if (IsEmpty() || address.IsEmpty())
    return PFalse;

  PIPSocket::Address ip1, ip2;
  WORD port1 = 65535, port2 = 65535;
  return GetIpAndPort(ip1, port1) &&
         address.GetIpAndPort(ip2, port2) &&
         ((ip1 *= ip2) || (wildcard && (ip1.IsAny() || ip2.IsAny()))) &&
         (port1 == port2 || (wildcard && (port1 == 65535 || port2 == 65535)));
}


PBoolean OpalTransportAddress::IsCompatible(const OpalTransportAddress & address) const
{
  if (IsEmpty() || address.IsEmpty())
    return PTrue;

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
    return PFalse;

  WORD dummy = 65535;
  return m_transport->GetIpAndPort(*this, ip, dummy);
}


PBoolean OpalTransportAddress::GetIpAndPort(PIPSocket::Address & ip, WORD & port) const
{
  if (m_transport == NULL)
    return PFalse;

  return m_transport->GetIpAndPort(*this, ip, port);
}


PBoolean OpalTransportAddress::GetIpAndPort(PIPSocketAddressAndPort & ipPort) const
{
  if (m_transport == NULL)
    return PFalse;

  PIPSocket::Address ip;
  WORD port = 0;
  if (!m_transport->GetIpAndPort(*this, ip, port))
    return false;

  ipPort.SetAddress(ip, port);
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

  PTRACE(1, "Opal\tCould not parse transport address \"" << *this << '"');
  MakeEmpty();
}


/////////////////////////////////////////////////////////////////

void OpalTransportAddressArray::AppendString(const char * str)
{
  AppendAddress(OpalTransportAddress(str));
}


void OpalTransportAddressArray::AppendString(const PString & str)
{
  AppendAddress(OpalTransportAddress(str));
}


void OpalTransportAddressArray::AppendAddress(const OpalTransportAddress & addr)
{
  if (!addr)
    Append(new OpalTransportAddress(addr));
}


void OpalTransportAddressArray::AppendStringCollection(const PCollection & coll)
{
  for (PINDEX i = 0; i < coll.GetSize(); i++) {
    PObject * obj = coll.GetAt(i);
    if (obj != NULL && PIsDescendant(obj, PString))
      AppendAddress(OpalTransportAddress(*(PString *)obj));
  }
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
  return PFalse;
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
    PTRACE(2, "Opal\tIllegal IP transport address: \"" << address << '"');
    return PFalse;
  }

  if (service == "*")
    port = 0;
  else if (!service.IsEmpty()) {
    PCaselessString proto = address.GetProtoPrefix();
    if (proto == OpalTransportAddress::IpPrefix())
      proto = OpalTransportAddress::TcpPrefix();

    if ((port = PIPSocket::GetPortByService(proto, service)) == 0) {
      PTRACE(2, "Opal\tIllegal IP transport port/service: \"" << address << '"');
      return PFalse;
    }
  }

  if (host[0] == '*') {
    ip = PIPSocket::GetDefaultIpAny();
    return true;
  }

  if (host == "0.0.0.0") {
    ip = PIPSocket::Address::GetAny(4);
    return true;
  }

  if (host == "::" || host == "[::]") {
    ip = PIPSocket::Address::GetAny(6);
    return true;
  }

  if (device.IsEmpty()) {
    if (PIPSocket::GetHostAddress(host, ip))
      return true;
    PTRACE(1, "Opal\tCould not find host \"" << host << '"');
  }
  else {
    if (ip.FromString(device))
      return true;
    PTRACE(1, "Opal\tCould not find device \"" << device << '"');
  }

  return PFalse;
}


//////////////////////////////////////////////////////////////////////////

PBoolean OpalInternalIPTransport::GetAdjustedIpAndPort(const OpalTransportAddress & address,
                                 OpalEndPoint & endpoint,
                                 OpalTransportAddress::BindOptions option,
                                 PIPSocket::Address & ip,
                                 WORD & port,
                                 PBoolean & reuseAddr)
{
  reuseAddr = address[address.GetLength()-1] == '+';

  switch (option) {
    case OpalTransportAddress::NoBinding :
      if (address.GetIpAddress(ip))
        ip = PIPSocket::Address::GetAny(ip.GetVersion());
      else
        ip = PIPSocket::GetDefaultIpAny();
      port = 0;
      return PTrue;

    case OpalTransportAddress::HostOnly :
      port = 0;
      return address.GetIpAddress(ip);

    case OpalTransportAddress::RouteInterface :
      if (address.GetIpAndPort(ip, port))
        ip = PIPSocket::GetRouteInterfaceAddress(ip);
      else
        ip = PIPSocket::GetDefaultIpAny();
      port = 0;
      return TRUE;

    default :
      port = endpoint.GetDefaultSignalPort();
      return address.GetIpAndPort(ip, port);
  }
}


//////////////////////////////////////////////////////////////////////////

OpalListener::OpalListener(OpalEndPoint & ep)
  : endpoint(ep)
{
  thread = NULL;
  threadMode = SpawnNewThreadMode;
}


void OpalListener::PrintOn(ostream & strm) const
{
  strm << GetLocalAddress();
}


void OpalListener::CloseWait()
{
  PTRACE(3, "Listen\tStopping listening thread on " << GetLocalAddress());
  Close();

  PThread * exitingThread = thread;
  thread = NULL;

  if (exitingThread != NULL) {
    if (exitingThread == PThread::Current())
      exitingThread->SetAutoDelete();
    else {
      PAssert(exitingThread->WaitForTermination(10000), "Listener thread did not terminate");
      delete exitingThread;
    }
  }
}


void OpalListener::ListenForConnections(PThread & thread, INT)
{
  PTRACE(3, "Listen\tStarted listening thread on " << GetLocalAddress());
  PAssert(!acceptHandler.IsNULL(), PLogicError);

  while (IsOpen()) {
    OpalTransport * transport = Accept(PMaxTimeInterval);
    if (transport == NULL)
      acceptHandler(*this, 0);
    else {
      switch (threadMode) {
        case SpawnNewThreadMode :
          transport->AttachThread(PThread::Create(acceptHandler,
                                                  (INT)transport,
                                                  PThread::NoAutoDeleteThread,
                                                  PThread::NormalPriority,
                                                  "Opal Answer"));
          break;

        case HandOffThreadMode :
          transport->AttachThread(&thread);
          this->thread = NULL;
          // Then do next case

        case SingleThreadMode :
          acceptHandler(*this, (INT)transport);
      }
      // Note: acceptHandler is responsible for deletion of the transport
    }
  }
}


PBoolean OpalListener::StartThread(const PNotifier & theAcceptHandler, ThreadMode mode)
{
  acceptHandler = theAcceptHandler;
  threadMode = mode;

  thread = PThread::Create(PCREATE_NOTIFIER(ListenForConnections), "Opal Listener");

  return thread != NULL;
}


//////////////////////////////////////////////////////////////////////////

OpalListenerIP::OpalListenerIP(OpalEndPoint & ep,
                               PIPSocket::Address binding,
                               WORD port,
                               PBoolean exclusive)
  : OpalListener(ep),
    localAddress(binding)
{
  listenerPort = port;
  exclusiveListener = exclusive;
}


OpalListenerIP::OpalListenerIP(OpalEndPoint & endpoint,
                               const OpalTransportAddress & binding,
                               OpalTransportAddress::BindOptions option)
  : OpalListener(endpoint)
{
  OpalInternalIPTransport::GetAdjustedIpAndPort(binding, endpoint, option, localAddress, listenerPort, exclusiveListener);
}


#if P_NAT
OpalTransportAddress OpalListenerIP::GetLocalAddress(const OpalTransportAddress & remoteAddress) const
{
  PIPSocket::Address localIP = localAddress;

  PIPSocket::Address remoteIP;
  if (remoteAddress.GetIpAddress(remoteIP)) {
    OpalManager & manager = endpoint.GetManager();
    PNatMethod * natMethod = manager.GetNatMethod(remoteIP);
    if (natMethod != NULL) {
      if (localIP.IsAny())
        natMethod->GetInterfaceAddress(localIP);
      manager.TranslateIPAddress(localIP, remoteIP);
    }
  }

  return OpalTransportAddress(localIP, listenerPort, GetProtoPrefix());
}
#else // P_NAT
OpalTransportAddress OpalListenerIP::GetLocalAddress(const OpalTransportAddress &) const
{
  return OpalTransportAddress(localAddress, listenerPort, GetProtoPrefix());
}
#endif // P_NAT


bool OpalListenerIP::CanCreateTransport(const OpalTransportAddress & localAddress,
                                        const OpalTransportAddress & remoteAddress) const
{
  if (remoteAddress.GetProtoPrefix() != GetProtoPrefix())
    return false;

  // The following then checks for IPv4/IPv6
  OpalTransportAddress myLocalAddress = GetLocalAddress();
  if (!myLocalAddress.IsCompatible(remoteAddress))
    return false;

  if (localAddress.IsEmpty())
    return true;
  
  return myLocalAddress.IsCompatible(localAddress);
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


PBoolean OpalListenerTCP::Open(const PNotifier & theAcceptHandler, ThreadMode mode)
{
  if (listenerPort == 0) {
    OpalManager & manager = endpoint.GetManager();
    listenerPort = manager.GetNextTCPPort();
    WORD firstPort = listenerPort;
    while (!listener.Listen(localAddress, 1, listenerPort)) {
      listenerPort = manager.GetNextTCPPort();
      if (listenerPort == firstPort) {
        PTRACE(1, "Listen\tOpen on " << localAddress << " failed: " << listener.GetErrorText());
        break;
      }
    }
    listenerPort = listener.GetPort();
    return StartThread(theAcceptHandler, mode);
  }

  if (listener.Listen(localAddress, 10, listenerPort, exclusiveListener ? PSocket::AddressIsExclusive : PSocket::CanReuseAddress))
    return StartThread(theAcceptHandler, mode);

  PTRACE(1, "Listen\tOpen (" << (exclusiveListener ? "EXCLUSIVE" : "REUSEADDR") << ") on "
         << localAddress.AsString(true) << ':' << listener.GetPort()
         << " failed: " << listener.GetErrorText());
  return false;
}


PBoolean OpalListenerTCP::IsOpen()
{
  return listener.IsOpen();
}


void OpalListenerTCP::Close()
{
  listener.Close();
}


OpalTransport * OpalListenerTCP::Accept(const PTimeInterval & timeout)
{
  if (!listener.IsOpen())
    return NULL;

  listener.SetReadTimeout(timeout); // Wait for remote connect

  PTRACE(4, "Listen\tWaiting on socket accept on " << GetLocalAddress());
  PTCPSocket * socket = new PTCPSocket;
  if (socket->Accept(listener)) {
    OpalTransportTCP * transport = new OpalTransportTCP(endpoint);
    if (transport->Open(socket))
      return transport;

    PTRACE(1, "Listen\tFailed to open transport, connection not started.");
    delete transport;
    return NULL;
  }
  else if (socket->GetErrorCode() != PChannel::Interrupted) {
    PTRACE(1, "Listen\tAccept error:" << socket->GetErrorText());
    listener.Close();
  }

  delete socket;
  return NULL;
}


OpalTransport * OpalListenerTCP::CreateTransport(const OpalTransportAddress & localAddress,
                                                 const OpalTransportAddress & remoteAddress) const
{
  if (!CanCreateTransport(localAddress, remoteAddress))
    return NULL;

  if (localAddress.IsEmpty())
    return new OpalTransportTCP(endpoint);

  return localAddress.CreateTransport(endpoint, OpalTransportAddress::HostOnly);
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
  : OpalListenerIP(endpoint, binding, port, exclusive),
    listenerBundle(PMonitoredSockets::Create(binding.AsString(),
                                             !exclusive
                                             P_NAT_PARAM(endpoint.GetManager().GetNatMethod())))
{
}


OpalListenerUDP::OpalListenerUDP(OpalEndPoint & endpoint,
                                 const OpalTransportAddress & binding,
                                 OpalTransportAddress::BindOptions option)
  : OpalListenerIP(endpoint, binding, option)
  , listenerBundle(PMonitoredSockets::Create(binding.GetHostName(),
                                             !exclusiveListener
                                             P_NAT_PARAM(endpoint.GetManager().GetNatMethod())))
  , m_bufferSize(32768)
{
  if (binding.GetHostName() == "*")
    localAddress.FromString(""); // Set invalid to distinguish between "*", "0.0.0.0" and "[::]"
}


OpalListenerUDP::~OpalListenerUDP()
{
  CloseWait();
}


PBoolean OpalListenerUDP::Open(const PNotifier & theAcceptHandler, ThreadMode /*mode*/)
{
  if (listenerBundle->Open(listenerPort) && StartThread(theAcceptHandler, SingleThreadMode)) {
    /* UDP packets need to be handled. Not so much at high speed, but must not be
       significantly delayed by media threads which are running at HighPriority.
       This, for example, helps make sure that a SIP BYE is received and processed
       to kill a call where codecs etc in the media threads are hogging all the CPU. */
    thread->SetPriority(PThread::HighestPriority);
    return true;
  }

  PTRACE(1, "Listen\tCould not start any UDP listeners");
  return PFalse;
}


PBoolean OpalListenerUDP::IsOpen()
{
  return listenerBundle != NULL && listenerBundle->IsOpen();
}


void OpalListenerUDP::Close()
{
  if (listenerBundle != NULL)
    listenerBundle->Close();
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
  listenerBundle->ReadFromBundle(param);

  switch (param.m_errorCode) {
    case PChannel::NoError :
      pdu.SetSize(param.m_lastCount);
      break;

    case PChannel::BufferTooSmall :
      break;

    case PChannel::Interrupted :
      PTRACE(4, "Listen\tInterfaces changed");
      return NULL;

    default :
      PTRACE(1, "Listen\tUDP read error: " << PChannel::GetErrorText(param.m_errorCode, param.m_errorNumber));
      return NULL;
  }

  OpalTransportUDP * transport = new OpalTransportUDP(endpoint, listenerBundle, param.m_iface);
  transport->m_preReadPacket = pdu;
  transport->m_preReadOK = param.m_errorCode == PChannel::NoError;
  transport->SetRemoteAddress(OpalTransportAddress(param.m_addr, param.m_port, OpalTransportAddress::UdpPrefix()));
  return transport;
}


OpalTransport * OpalListenerUDP::CreateTransport(const OpalTransportAddress & localAddress,
                                                 const OpalTransportAddress & remoteAddress) const
{
  if (!CanCreateTransport(localAddress, remoteAddress))
    return NULL;

  PString iface;
  if (!localAddress.IsEmpty()) {
    PIPSocket::Address addr;
    if (localAddress.GetIpAddress(addr))
      iface = addr.AsString(true);
  }

  return new OpalTransportUDP(endpoint, listenerBundle, iface);
}


const PCaselessString & OpalListenerUDP::GetProtoPrefix() const
{
  return OpalTransportAddress::UdpPrefix();
}


#if P_NAT
OpalTransportAddress OpalListenerUDP::GetLocalAddress(const OpalTransportAddress & remoteAddress) const
{
  PIPSocket::Address localIP(""); // Make invalid
  WORD port = listenerPort;

  PIPSocket::Address remoteIP;
  if (remoteAddress.GetIpAddress(remoteIP)) {
    PNatMethod * natMethod = endpoint.GetManager().GetNatMethod(remoteIP);
    if (natMethod != NULL) {
      natMethod->GetInterfaceAddress(localIP);
      listenerBundle->GetAddress(localIP.AsString(), localIP, port, true);
    }
  }

  if (!localIP.IsValid()) {
    listenerBundle->GetAddress(PString::Empty(), localIP, port, false);
    if (!localIP.IsValid() || localIP.IsAny())
      localIP = localAddress;
  }

  return OpalTransportAddress(localIP, port, GetProtoPrefix());
}
#else // P_NAT
OpalTransportAddress OpalListenerUDP::GetLocalAddress(const OpalTransportAddress &) const
{
  return OpalTransportAddress(localAddress, listenerPort, GetProtoPrefix());
}
#endif // P_NAT


//////////////////////////////////////////////////////////////////////////

OpalTransport::OpalTransport(OpalEndPoint & end)
  : endpoint(end)
{
  thread = NULL;
  m_keepAliveTimer.SetNotifier(PCREATE_NOTIFIER(KeepAlive));
}


OpalTransport::~OpalTransport()
{
  PAssert(thread == NULL, PLogicError);
}


void OpalTransport::PrintOn(ostream & strm) const
{
  strm << GetRemoteAddress() << "<if=" << GetLocalAddress() << '>';
}


PString OpalTransport::GetInterface() const
{
  return GetLocalAddress().GetHostName();
}


bool OpalTransport::SetInterface(const PString &)
{
  return true;
}


PBoolean OpalTransport::Close()
{
  PTRACE(4, "Opal\tTransport Close");

  /* Do not use PIndirectChannel::Close() as this deletes the sub-channel
     member field crashing the background thread. Just close the base
     sub-channel so breaks the threads I/O block.
   */
  if (IsOpen())
    return GetBaseWriteChannel()->Close();

  return PTrue;
}


void OpalTransport::CloseWait()
{
  PTRACE(3, "Opal\tTransport clean up on termination");

  Close();

  channelPointerMutex.StartWrite();
  PThread * exitingThread = thread;
  thread = NULL;
  channelPointerMutex.EndWrite();

  m_keepAliveTimer.Stop();

  if (exitingThread != NULL) {
    if (exitingThread == PThread::Current())
      exitingThread->SetAutoDelete();
    else {
      PAssert(exitingThread->WaitForTermination(10000), "Transport thread did not terminate");
      delete exitingThread;
    }
  }
}


PBoolean OpalTransport::IsCompatibleTransport(const OpalTransportAddress &) const
{
  PAssertAlways(PUnimplementedFunction);
  return PFalse;
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


PBoolean OpalTransport::WriteConnect(WriteConnectCallback function, void * userData)
{
  return function(*this, userData);
}


void OpalTransport::AttachThread(PThread * thrd)
{
  if (thread != NULL) {
    PAssert(thread->WaitForTermination(10000), "Transport not terminated when reattaching thread");
    delete thread;
  }

  thread = thrd;
  PTRACE_CONTEXT_ID_TO(thread);
}


PBoolean OpalTransport::IsRunning() const
{
  if (thread == NULL)
    return PFalse;

  return !thread->IsTerminated();
}


void OpalTransport::SetKeepAlive(const PTimeInterval & timeout, const PBYTEArray & data)
{
  m_keepAliveTimer.Stop(false);

  channelPointerMutex.StartWrite();
  m_keepAliveData = data;
  channelPointerMutex.EndWrite();

  if (!data.IsEmpty())
    m_keepAliveTimer = timeout;
}


void OpalTransport::KeepAlive(PTimer &, INT)
{
  channelPointerMutex.StartRead();
#if PTRACING
  bool ok =
#endif
  Write(m_keepAliveData, m_keepAliveData.GetSize());
  channelPointerMutex.EndRead();

  PTRACE(ok ? 5 : 2, "Opal\tTransport keep alive "
         << (ok ? "send to " : "failed on ")
         << *this << ": " << GetLastWriteCount() << " bytes");
}


PBoolean OpalTransport::Write(const void * buf, PINDEX len)
{
  m_keepAliveTimer.Reset();
  return PIndirectChannel::Write(buf, len);
}


/////////////////////////////////////////////////////////////////////////////

OpalTransportIP::OpalTransportIP(OpalEndPoint & end,
                                 PIPSocket::Address binding,
                                 WORD port)
  : OpalTransport(end),
    localAddress(binding),
    remoteAddress(PIPSocket::Address::GetAny(binding.GetVersion()))
{
  localPort = port;
  remotePort = 0;
}


OpalTransportAddress OpalTransportIP::GetLocalAddress(bool /*allowNAT*/) const
{
  return OpalTransportAddress(localAddress, localPort, GetProtoPrefix());
}


PBoolean OpalTransportIP::SetLocalAddress(const OpalTransportAddress & newLocalAddress)
{
  if (!IsCompatibleTransport(newLocalAddress))
    return PFalse;

  if (!IsOpen())
    return newLocalAddress.GetIpAndPort(localAddress, localPort);

  PIPSocket::Address address;
  WORD port = 0;
  if (!newLocalAddress.GetIpAndPort(address, port))
    return PFalse;

  return localAddress == address && localPort == port;
}


OpalTransportAddress OpalTransportIP::GetRemoteAddress() const
{
  return OpalTransportAddress(remoteAddress, remotePort, GetProtoPrefix());
}


PBoolean OpalTransportIP::SetRemoteAddress(const OpalTransportAddress & address)
{
  if (IsCompatibleTransport(address))
    return address.GetIpAndPort(remoteAddress, remotePort);

  PTRACE(2, "OpalIP\tAttempt to set incompatible transport " << address);
  return PFalse;
}


/////////////////////////////////////////////////////////////////////////////

OpalTransportTCP::OpalTransportTCP(OpalEndPoint & ep,
                                   PIPSocket::Address binding,
                                   WORD port,
                                   PBoolean reuseAddr)
  : OpalTransportIP(ep, binding, port)
{
  reuseAddressFlag = reuseAddr;
}


OpalTransportTCP::OpalTransportTCP(OpalEndPoint & ep, PTCPSocket * socket)
  : OpalTransportIP(ep, INADDR_ANY, 0)
{
  Open(socket);
}


OpalTransportTCP::~OpalTransportTCP()
{
  CloseWait();
  PTRACE(4,"Opal\tDeleted transport " << *this);
}


PBoolean OpalTransportTCP::IsReliable() const
{
  return PTrue;
}


PBoolean OpalTransportTCP::IsCompatibleTransport(const OpalTransportAddress & address) const
{
  return (address.NumCompare(OpalTransportAddress::TcpPrefix()) == EqualTo) ||
         (address.NumCompare(OpalTransportAddress::IpPrefix())  == EqualTo);
}


PBoolean OpalTransportTCP::Connect()
{
  if (IsOpen())
    return PTrue;

  PTCPSocket * socket = new PTCPSocket(remotePort);
  Open(socket);

  PReadWaitAndSignal mutex(channelPointerMutex);

  socket->SetReadTimeout(10000);

  OpalManager & manager = endpoint.GetManager();
  localPort = manager.GetNextTCPPort();
  WORD firstPort = localPort;
  for (;;) {
    PTRACE(4, "OpalTCP\tConnecting to "
           << remoteAddress.AsString(true) << ':' << remotePort
           << " (local port=" << localPort << ')');
    if (socket->Connect(localAddress, localPort, remoteAddress))
      break;

    int errnum = socket->GetErrorNumber();
    if (localPort == 0 || (errnum != EADDRINUSE && errnum != EADDRNOTAVAIL)) {
      PTRACE(1, "OpalTCP\tCould not connect to "
                << remoteAddress.AsString(true) << ':' << remotePort
                << " (local port=" << localPort << ") - "
                << socket->GetErrorText() << '(' << errnum << ')');
      return SetErrorValues(socket->GetErrorCode(), errnum);
    }

    localPort = manager.GetNextTCPPort();
    if (localPort == firstPort) {
      PTRACE(1, "OpalTCP\tCould not bind to any port in range " <<
                manager.GetTCPPortBase() << " to " << manager.GetTCPPortMax());
      return SetErrorValues(socket->GetErrorCode(), errnum);
    }
  }

  socket->SetReadTimeout(PMaxTimeInterval);

  return OnOpen();
}


PBoolean OpalTransportTCP::ReadPDU(PBYTEArray & pdu)
{
  // Make sure is a RFC1006 TPKT
  switch (ReadChar()) {
    case 3 :  // Only support version 3
      break;

    default :  // Unknown version number
      SetErrorValues(ProtocolFailure, 0x80000000);
      // Do case for read error

    case -1 :
      return PFalse;
  }

  // Save timeout
  PTimeInterval oldTimeout = GetReadTimeout();

  // Should get all of PDU in 5 seconds or something is seriously wrong,
  SetReadTimeout(5000);

  // Get TPKT length
  BYTE header[3];
  PBoolean ok = ReadBlock(header, sizeof(header));
  if (ok) {
    PINDEX packetLength = ((header[1] << 8)|header[2]);
    if (packetLength < 4) {
      PTRACE(2, "H323TCP\tDwarf PDU received (length " << packetLength << ")");
      ok = PFalse;
    } else {
      packetLength -= 4;
      ok = ReadBlock(pdu.GetPointer(packetLength), packetLength);
    }
  }

  SetReadTimeout(oldTimeout);

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


PBoolean OpalTransportTCP::OnOpen()
{
  PIPSocket * socket = (PIPSocket *)GetReadChannel();

  // Get name of the remote computer for information purposes
  if (!socket->GetPeerAddress(remoteAddress, remotePort)) {
    PTRACE(1, "OpalTCP\tGetPeerAddress() failed: " << socket->GetErrorText());
    return PFalse;
  }

  // get local address of incoming socket to ensure that multi-homed machines
  // use a NIC address that is guaranteed to be addressable to destination
  if (!socket->GetLocalAddress(localAddress, localPort)) {
    PTRACE(1, "OpalTCP\tGetLocalAddress() failed: " << socket->GetErrorText());
    return PFalse;
  }

#ifndef __BEOS__
  if (!socket->SetOption(TCP_NODELAY, 1, IPPROTO_TCP)) {
    PTRACE(1, "OpalTCP\tSetOption(TCP_NODELAY) failed: " << socket->GetErrorText());
  }

  // make sure do not lose outgoing packets on close
  const linger ling = { 1, 3 };
  if (!socket->SetOption(SO_LINGER, &ling, sizeof(ling))) {
    PTRACE(1, "OpalTCP\tSetOption(SO_LINGER) failed: " << socket->GetErrorText());
    return PFalse;
  }
#endif

  PTRACE(3, "OpalTCP\tStarted connection to "
         << remoteAddress.AsString(true) << ':' << remotePort
         << " (if=" << localAddress.AsString(true) << ':' << localPort << ')');

  return PTrue;
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
  : OpalTransportIP(ep, binding, localPort)
  , manager(ep.GetManager())
  , m_bufferSize(8192)
  , m_preReadOK(false)
{
  PMonitoredSockets * sockets = PMonitoredSockets::Create(binding.AsString(),
                                                          reuseAddr
                                                          P_NAT_PARAM(manager.GetNatMethod()));
  if (preOpen)
    sockets->Open(localPort);
  Open(new PMonitoredSocketChannel(sockets, PFalse));
}


OpalTransportUDP::OpalTransportUDP(OpalEndPoint & ep,
                                   const PMonitoredSocketsPtr & listener,
                                   const PString & iface)
  : OpalTransportIP(ep, PIPSocket::GetDefaultIpAny(), 0)
  , manager(ep.GetManager())
  , m_bufferSize(8192)
  , m_preReadOK(true)
{
  PMonitoredSocketChannel * socket = new PMonitoredSocketChannel(listener, PTrue);
  socket->SetInterface(iface);
  socket->GetLocal(localAddress, localPort, !manager.IsLocalAddress(remoteAddress));
  Open(socket);

  PTRACE(3, "OpalUDP\tBinding to interface: " << localAddress.AsString(true) << ':' << localPort);
}


OpalTransportUDP::~OpalTransportUDP()
{
  CloseWait();
  PTRACE(4,"Opal\tDeleted transport " << *this);
}


PBoolean OpalTransportUDP::IsReliable() const
{
  return PFalse;
}


PBoolean OpalTransportUDP::IsCompatibleTransport(const OpalTransportAddress & address) const
{
  return (address.NumCompare(OpalTransportAddress::UdpPrefix()) == EqualTo) ||
         (address.NumCompare(OpalTransportAddress::IpPrefix())  == EqualTo);
}


PBoolean OpalTransportUDP::Connect()
{	
  if (remotePort == 0)
    return PFalse;

  if (remoteAddress.IsAny() || remoteAddress.IsBroadcast()) {
	  remoteAddress = PIPSocket::Address::GetBroadcast(remoteAddress.GetVersion());
    PTRACE(3, "OpalUDP\tBroadcast connect to port " << remotePort);
  }
  else {
    PTRACE(3, "OpalUDP\tStarted connect to " << remoteAddress.AsString(true) << ':' << remotePort);
  }

  if (PAssertNULL(writeChannel) == NULL)
    return PFalse;

  PMonitoredSocketsPtr bundle = ((PMonitoredSocketChannel *)writeChannel)->GetMonitoredSockets();
  if (bundle->IsOpen())
    return PTrue;

  OpalManager & manager = endpoint.GetManager();

  localPort = manager.GetNextUDPPort();
  WORD firstPort = localPort;
  while (!bundle->Open(localPort)) {
    localPort = manager.GetNextUDPPort();
    if (localPort == firstPort) {
      PTRACE(1, "OpalUDP\tCould not bind to any port in range " <<
	      manager.GetUDPPortBase() << " to " << manager.GetUDPPortMax());
      return PFalse;
    }
  }

  return PTrue;
}


PString OpalTransportUDP::GetInterface() const
{
  PMonitoredSocketChannel * socket = (PMonitoredSocketChannel *)readChannel;
  if (socket != NULL)
    return socket->GetInterface();

  return OpalTransportIP::GetInterface();
}


bool OpalTransportUDP::SetInterface(const PString & iface)
{
  PTRACE(3, "OpalUDP\tSetting interface to " << iface);

  PMonitoredSocketChannel * socket = (PMonitoredSocketChannel *)readChannel;
  if (socket == NULL)
    return false;
    
  socket->SetInterface(iface);
  return true;
}


OpalTransportAddress OpalTransportUDP::GetLocalAddress(bool allowNAT) const
{
  PMonitoredSocketChannel * socket = (PMonitoredSocketChannel *)readChannel;
  if (socket != NULL) {
    OpalTransportUDP * thisWritable = const_cast<OpalTransportUDP *>(this);
    if (!socket->GetLocal(thisWritable->localAddress, thisWritable->localPort,
                          allowNAT && !manager.IsLocalAddress(remoteAddress)))
      return OpalTransportAddress();
  }

  return OpalTransportIP::GetLocalAddress(allowNAT);
}


PBoolean OpalTransportUDP::SetLocalAddress(const OpalTransportAddress & newLocalAddress)
{
  if (OpalTransportIP::GetLocalAddress().IsEquivalent(newLocalAddress))
    return PTrue;

  if (!IsCompatibleTransport(newLocalAddress))
    return PFalse;

  if (!newLocalAddress.GetIpAndPort(localAddress, localPort))
    return PFalse;

  PMonitoredSocketChannel * socket = (PMonitoredSocketChannel *)readChannel;
  if (socket != NULL)
    socket->GetMonitoredSockets()->Open(localPort);

  return OpalTransportIP::SetLocalAddress(newLocalAddress);
}


PBoolean OpalTransportUDP::SetRemoteAddress(const OpalTransportAddress & address)
{
  if (!OpalTransportIP::SetRemoteAddress(address))
    return PFalse;

  PMonitoredSocketChannel * socket = (PMonitoredSocketChannel *)readChannel;
  if (socket != NULL)
    socket->SetRemote(remoteAddress, remotePort);

  return PTrue;
}


void OpalTransportUDP::SetPromiscuous(PromisciousModes promiscuous)
{
  PMonitoredSocketChannel * socket = (PMonitoredSocketChannel *)readChannel;
  if (socket != NULL) {
    socket->SetPromiscuous(promiscuous != AcceptFromRemoteOnly);
    if (promiscuous == AcceptFromAnyAutoSet)
      socket->SetRemote(PIPSocket::GetDefaultIpAny(), 0);
  }
}


OpalTransportAddress OpalTransportUDP::GetLastReceivedAddress() const
{
  PMonitoredSocketChannel * socket = (PMonitoredSocketChannel *)readChannel;
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

  PMonitoredSocketChannel * socket = (PMonitoredSocketChannel *)readChannel;
  if (socket != NULL)
    iface = socket->GetLastReceivedInterface();

  if (iface.IsEmpty())
    iface = OpalTransport::GetLastReceivedInterface();

  return iface;
}


PBoolean OpalTransportUDP::Read(void * buffer, PINDEX length)
{
  if (m_preReadPacket.IsEmpty())
    return OpalTransportIP::Read(buffer, length);

  lastReadCount = PMIN(length, m_preReadPacket.GetSize());
  memcpy(buffer, m_preReadPacket, lastReadCount);
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

  if (!Read(packet.GetPointer(m_bufferSize), m_bufferSize)) {
    packet.SetSize(0);
    return false;
  }

  packet.SetSize(GetLastReadCount());
  return PTrue;
}


PBoolean OpalTransportUDP::WritePDU(const PBYTEArray & packet)
{
  return Write((const BYTE *)packet, packet.GetSize());
}


PBoolean OpalTransportUDP::WriteConnect(WriteConnectCallback function, void * userData)
{
  PMonitoredSocketChannel * socket = (PMonitoredSocketChannel *)writeChannel;
  if (socket == NULL)
    return PFalse;

  PMonitoredSocketsPtr bundle = socket->GetMonitoredSockets();
  PIPSocket::Address address;
  GetRemoteAddress().GetIpAddress(address);
  PStringArray interfaces = bundle->GetInterfaces(PFalse, address);

  PBoolean ok = PFalse;
  for (PINDEX i = 0; i < interfaces.GetSize(); i++) {
    PIPSocket::Address ifip(interfaces[i]);
    if (ifip.GetVersion() != remoteAddress.GetVersion())
      PTRACE(4, "OpalUDP\tSkipping incompatible interface " << i << " - \"" << interfaces[i] << '"');
    else {
      PTRACE(4, "OpalUDP\tWriting to interface " << i << " - \"" << interfaces[i] << '"');
      socket->SetInterface(interfaces[i]);
      // Make sure is compatible address
      if (function(*this, userData))
        ok = PTrue;
    }
  }

  return ok;
}


const PCaselessString & OpalTransportUDP::GetProtoPrefix() const
{
  return OpalTransportAddress::UdpPrefix();
}


//////////////////////////////////////////////////////////////////////////

#if OPAL_PTLIB_SSL

OpalTransportTLS::OpalTransportTLS(OpalEndPoint & ep,
                                   PIPSocket::Address binding,
                                   WORD port,
                                   PBoolean reuseAddr)
  : OpalTransportTCP(ep, binding, port, reuseAddr)
{
}


OpalTransportTLS::~OpalTransportTLS()
{
  CloseWait();
  PTRACE(4,"Opal\tDeleted transport " << *this);
}


PBoolean OpalTransportTLS::IsCompatibleTransport(const OpalTransportAddress & address) const
{
  return OpalTransportTCP::IsCompatibleTransport(address) ||
         address.NumCompare(OpalTransportAddress::TlsPrefix()) == EqualTo;
}


PBoolean OpalTransportTLS::Connect()
{
  if (IsOpen())
    return PTrue;

  PTCPSocket * socket = new PTCPSocket(remotePort);

  PReadWaitAndSignal mutex(channelPointerMutex);

  socket->SetReadTimeout(10000);

  OpalManager & manager = endpoint.GetManager();
  localPort = manager.GetNextTCPPort();
  WORD firstPort = localPort;
  for (;;) {
    PTRACE(4, "OpalTLS\tConnecting to "
           << remoteAddress.AsString(true) << ':' << remotePort
           << " (local port=" << localPort << ')');
    if (socket->Connect(localAddress, localPort, remoteAddress))
      break;

    int errnum = socket->GetErrorNumber();
    if (localPort == 0 || (errnum != EADDRINUSE && errnum != EADDRNOTAVAIL)) {
      PTRACE(1, "OpalTLS\tCould not connect to "
                << remoteAddress.AsString(true) << ':' << remotePort
                << " (local port=" << localPort << ") - "
                << socket->GetErrorText() << '(' << errnum << ')');
      return SetErrorValues(socket->GetErrorCode(), errnum);
    }

    localPort = manager.GetNextTCPPort();
    if (localPort == firstPort) {
      PTRACE(1, "OpalTCP\tCould not bind to any port in range " <<
                manager.GetTCPPortBase() << " to " << manager.GetTCPPortMax());
      return SetErrorValues(socket->GetErrorCode(), errnum);
    }
  }

  socket->SetReadTimeout(PMaxTimeInterval);

  PSSLCertificate ca, cert;
  PSSLPrivateKey key;
  if (!endpoint.GetSSLCredentials(ca, cert, key))
    return SetErrorValues(AccessDenied, EINVAL);

  PSSLChannel * sslChannel = new PSSLChannel();
  sslChannel->SetReadTimeout(5000);

  sslChannel->AddCA(ca); // Don't care about error return on this one

  if (!sslChannel->UseCertificate(cert)) {
    PTRACE(1, "OpalTLS\tCould not use certificate");
  }
  else if (!sslChannel->UsePrivateKey(key)) {
    PTRACE(1, "OpalTLS\tCould not use private key");
  }
  else if (!sslChannel->Connect(socket)) {
    PTRACE(1, "OpalTLS\tConnect failed: " << sslChannel->GetErrorText());
  }
  else
    return Open(sslChannel);

  delete sslChannel;
  return SetErrorValues(AccessDenied, EACCES);
}

PBoolean OpalTransportTLS::OnOpen()
{
  PSSLChannel * sslChannel = dynamic_cast<PSSLChannel *>(GetReadChannel());
  if (sslChannel == NULL)
    return PFalse;

  PIPSocket * socket = dynamic_cast<PIPSocket *>(sslChannel->GetReadChannel());

  // Get name of the remote computer for information purposes
  if (!socket->GetPeerAddress(remoteAddress, remotePort)) {
    PTRACE(1, "OpalTLS\tGetPeerAddress() failed: " << socket->GetErrorText());
    return PFalse;
  }

  // get local address of incoming socket to ensure that multi-homed machines
  // use a NIC address that is guaranteed to be addressable to destination
  if (!socket->GetLocalAddress(localAddress, localPort)) {
    PTRACE(1, "OpalTLS\tGetLocalAddress() failed: " << socket->GetErrorText());
    return PFalse;
  }

#ifndef __BEOS__
  if (!socket->SetOption(TCP_NODELAY, 1, IPPROTO_TCP)) {
    PTRACE(1, "OpalTLS\tSetOption(TCP_NODELAY) failed: " << socket->GetErrorText());
  }

  // make sure do not lose outgoing packets on close
  const linger ling = { 1, 3 };
  if (!socket->SetOption(SO_LINGER, &ling, sizeof(ling))) {
    PTRACE(1, "OpalTCP\tSetOption(SO_LINGER) failed: " << socket->GetErrorText());
    return PFalse;
  }
#endif

  PTRACE(3, "OpalTLS\tStarted connection to "
         << remoteAddress.AsString(true) << ':' << remotePort
         << " (if=" << localAddress.AsString(true) << ':' << localPort << ')');

  return PTrue;
}


const PCaselessString & OpalTransportTLS::GetProtoPrefix() const
{
  return OpalTransportAddress::TlsPrefix();
}

//////////////////////////////////////////////////////////////////////////

OpalListenerTLS::OpalListenerTLS(OpalEndPoint & ep,
                                 PIPSocket::Address binding,
                                 WORD port,
                                 PBoolean exclusive)
  : OpalListenerTCP(ep, binding, port, exclusive)
{
  Construct();
}


OpalListenerTLS::OpalListenerTLS(OpalEndPoint & ep,
                                    const OpalTransportAddress & binding,
                                    OpalTransportAddress::BindOptions option)
  : OpalListenerTCP(ep, binding, option)
{
  Construct();
}


OpalListenerTLS::~OpalListenerTLS()
{
  delete sslContext;
}


void OpalListenerTLS::Construct()
{
  sslContext = new PSSLContext();

  PSSLCertificate ca, cert;
  PSSLPrivateKey key;
  if (endpoint.GetSSLCredentials(ca, cert, key)) {
    sslContext->AddCA(ca);
    if (!sslContext->UseCertificate(cert)) {
      PTRACE(1, "OpalTLS\tCould not use certificate");
    }
    else if (!sslContext->UsePrivateKey(key)) {
      PTRACE(1, "OpalTLS\tCould not use private key");
    }
    sslContext->SetCipherList("ALL");
  }
}


OpalTransport * OpalListenerTLS::Accept(const PTimeInterval & timeout)
{
  if (!listener.IsOpen())
    return NULL;

  listener.SetReadTimeout(timeout); // Wait for remote connect

  PTRACE(4, "OpalTLS\tWaiting on socket accept on " << GetLocalAddress());
  PTCPSocket * socket = new PTCPSocket;
  if (!socket->Accept(listener)) {
    if (socket->GetErrorCode() != PChannel::Interrupted) {
      PTRACE(1, "Listen\tAccept error:" << socket->GetErrorText());
      listener.Close();
    }
    delete socket;
    return NULL;
  }

  PSSLChannel * ssl = new PSSLChannel(sslContext);
  if (!ssl->Accept(socket)) {
    PTRACE(1, "OpalTLS\tAccept failed: " << ssl->GetErrorText());
    delete ssl; // Will also delete socket
    return NULL;
  }

  OpalTransportTLS * transport = new OpalTransportTLS(endpoint);
  if (transport->Open(ssl))
    return transport;

  PTRACE(1, "OpalTLS\tFailed to open transport, connection not started.");
  delete transport; // Will also delete ssl and socket
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

  return new OpalTransportTLS(endpoint, binding);
}


#endif // OPAL_PTLIB_SSL


///////////////////////////////////////////////////////////////////////////////
