/*
 * transport.cxx
 *
 * Opal transports handler
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
 * $Log: transports.cxx,v $
 * Revision 1.2002  2001/10/03 05:53:25  robertj
 * Update to new PTLib channel error system.
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "transports.h"
#endif

#include <opal/transports.h>

#include <opal/manager.h>
#include <opal/endpoint.h>


static const char IpPrefix[]   = "ip$";
static const char TcpPrefix[]  = "tcp$";
static const char UdpPrefix[]  = "udp$";


////////////////////////////////////////////////////////////////

class OpalInternalTransport : public PObject
{
    PCLASSINFO(OpalInternalTransport, PObject);
  public:
    virtual PString GetHostName(
      const OpalTransportAddress & address
    ) const;

    virtual BOOL GetIpAndPort(
      const OpalTransportAddress & address,
      PIPSocket::Address & ip,
      WORD & port
    ) const;

    virtual OpalListener * CreateListener(
      const OpalTransportAddress & address,
      OpalEndPoint & endpoint   /// Endpoint object for transport creation.
    ) const  = 0;

    virtual OpalListener * CreateCompatibleListener(
      const OpalTransportAddress & address,
      OpalEndPoint & endpoint   /// Endpoint object for transport creation.
    ) const  = 0;

    virtual OpalTransport * CreateTransport(
      OpalEndPoint & endpoint   /// Endpoint object for transport creation.
    ) const = 0;
};


////////////////////////////////////////////////////////////////

class OpalInternalIPTransport : public OpalInternalTransport
{
    PCLASSINFO(OpalInternalIPTransport, OpalInternalTransport);
  public:
    virtual PString GetHostName(
      const OpalTransportAddress & address
    ) const;
    virtual BOOL GetIpAndPort(
      const OpalTransportAddress & address,
      PIPSocket::Address & ip,
      WORD & port
    ) const;
};


////////////////////////////////////////////////////////////////

static class OpalInternalTCPTransport : public OpalInternalIPTransport
{
    PCLASSINFO(OpalInternalTCPTransport, OpalInternalIPTransport);
  public:
    virtual OpalListener * CreateListener(
      const OpalTransportAddress & address,
      OpalEndPoint & endpoint   /// Endpoint object for transport creation.
    ) const;
    virtual OpalListener * CreateCompatibleListener(
      const OpalTransportAddress & address,
      OpalEndPoint & endpoint   /// Endpoint object for transport creation.
    ) const;
    virtual OpalTransport * CreateTransport(
      OpalEndPoint & endpoint   /// Endpoint object for transport creation.
    ) const;
} internalTCPTransport;


////////////////////////////////////////////////////////////////

static class OpalInternalUDPTransport : public OpalInternalIPTransport
{
    PCLASSINFO(OpalInternalUDPTransport, OpalInternalIPTransport);
  public:
    virtual OpalListener * CreateListener(
      const OpalTransportAddress & address,
      OpalEndPoint & endpoint   /// Endpoint object for transport creation.
    ) const;
    virtual OpalListener * CreateCompatibleListener(
      const OpalTransportAddress & address,
      OpalEndPoint & endpoint   /// Endpoint object for transport creation.
    ) const;
    virtual OpalTransport * CreateTransport(
      OpalEndPoint & endpoint   /// Endpoint object for transport creation.
    ) const;
} internalUDPTransport;


/////////////////////////////////////////////////////////////////

#define new PNEW


/////////////////////////////////////////////////////////////////

OpalTransportAddress::OpalTransportAddress()
{
  transport = NULL;
}


OpalTransportAddress::OpalTransportAddress(const char * cstr,
                                           WORD port,
                                           const char * proto)
  : PString(cstr)
{
  SetInternalTransport(port, proto);
}


OpalTransportAddress::OpalTransportAddress(const PString & str,
                                           WORD port,
                                           const char * proto)
  : PString(str)
{
  SetInternalTransport(port, proto);
}


OpalTransportAddress::OpalTransportAddress(const PIPSocket::Address & addr, WORD port, const char * proto)
  : PString(addr.AsString())
{
  SetInternalTransport(port, proto);
}


PString OpalTransportAddress::GetHostName() const
{
  if (transport == NULL)
    return PString();

  return transport->GetHostName(*this);
}
  

BOOL OpalTransportAddress::GetIpAndPort(PIPSocket::Address & ip, WORD & port) const
{
  if (transport == NULL)
    return FALSE;

  return transport->GetIpAndPort(*this, ip, port);
}


OpalListener * OpalTransportAddress::CreateListener(OpalEndPoint & endpoint) const
{
  if (transport == NULL)
    return NULL;

  return transport->CreateListener(*this, endpoint);
}


OpalListener * OpalTransportAddress::CreateCompatibleListener(OpalEndPoint & endpoint) const
{
  if (transport == NULL)
    return NULL;

  return transport->CreateCompatibleListener(*this, endpoint);
}


OpalTransport * OpalTransportAddress::CreateTransport(OpalEndPoint & endpoint) const
{
  if (transport == NULL)
    return NULL;

  return transport->CreateTransport(endpoint);
}


void OpalTransportAddress::SetInternalTransport(WORD port, const char * proto)
{
  transport = NULL;
  
  if (IsEmpty())
    return;

  PINDEX dollar = Find('$');
  if (dollar == P_MAX_INDEX) {
    PString prefix(proto == NULL ? TcpPrefix : proto);
    if (prefix.Find('$') == P_MAX_INDEX)
      prefix += '$';

    Splice(prefix, 0);
    dollar = prefix.GetLength()-1;
  }

  PCaselessString type = Left(dollar+1);

  // look for known transport types, note that "ip$" == "tcp$"
  if ((type == IpPrefix) || (type == TcpPrefix))
    transport = &internalTCPTransport;
  else if (type == UdpPrefix)
    transport = &internalUDPTransport;
  else
    return;

  if (port != 0 && Find(':', dollar) == P_MAX_INDEX)
    sprintf(":%u", port);
}


/////////////////////////////////////////////////////////////////

PString OpalInternalTransport::GetHostName(const OpalTransportAddress & address) const
{
  // skip transport identifier
  PINDEX pos = address.Find('$');
  if (pos == P_MAX_INDEX)
    return PString();

  return address.Mid(pos+1);
}


BOOL OpalInternalTransport::GetIpAndPort(const OpalTransportAddress &,
                                         PIPSocket::Address &,
                                         WORD &) const
{
  return FALSE;
}


//////////////////////////////////////////////////////////////////////////

PString OpalInternalIPTransport::GetHostName(const OpalTransportAddress & address) const
{
  PString str = OpalInternalTransport::GetHostName(address);

  PString host;
  PINDEX colon = str.Find(':');
  if (colon == P_MAX_INDEX)
    host = str;
  else
    host = str.Left(colon);

  PIPSocket::Address ip;
  if (PIPSocket::GetHostAddress(host, ip))
    return ip.AsString();

  return host;
}

BOOL OpalInternalIPTransport::GetIpAndPort(const OpalTransportAddress & address,
                                           PIPSocket::Address & ip,
                                           WORD & port) const
{
  // skip transport identifier
  PINDEX dollar = address.Find('$');
  if (dollar == P_MAX_INDEX)
    return FALSE;
  
  // parse host and port
  PString host;
  PINDEX colon = address.Find(':', dollar+1);
  if (colon == P_MAX_INDEX)
    host = address.Mid(dollar+1);
  else {
    host = address.Mid(dollar+1, colon-dollar-1);
    PString tid = address.Left(dollar);
    if (tid == IpPrefix)
      port = PIPSocket::GetPortByService("tcp", address.Mid(colon+1));
    else
      port = PIPSocket::GetPortByService(address.Left(dollar-1), address.Mid(colon+1));
  }

  if (host.IsEmpty() || port == 0) {
    PTRACE(2, "Opal\tIllegal IP transport address: \"" << *this << '"');
    return FALSE;
  }

  if (host == "*") {
    ip = INADDR_ANY;
    return TRUE;
  }

  if (PIPSocket::GetHostAddress(host, ip))
    return TRUE;

  PTRACE(1, "Opal\tCould not find host : \"" << host << '"');
  return FALSE;
}


//////////////////////////////////////////////////////////////////////////

OpalListener * OpalInternalTCPTransport::CreateListener(const OpalTransportAddress & address,
                                                        OpalEndPoint & endpoint) const
{
  PIPSocket::Address ip;
  WORD port = endpoint.GetDefaultSignalPort();
  if (address.GetIpAndPort(ip, port))
    return new OpalListenerTCP(endpoint, ip, port, address[address.GetLength()-1] != '+');
  return NULL;
}


OpalListener * OpalInternalTCPTransport::CreateCompatibleListener(
                                            const OpalTransportAddress & address,
                                            OpalEndPoint & endpoint) const
{
  PIPSocket::Address ip;
  WORD dummy;
  if (address.GetIpAndPort(ip, dummy))
    return new OpalListenerTCP(endpoint, ip, 0, address[address.GetLength()-1] != '+');

  return NULL;
}


OpalTransport * OpalInternalTCPTransport::CreateTransport(OpalEndPoint & endpoint) const
{
  return new OpalTransportTCP(endpoint);
}


//////////////////////////////////////////////////////////////////////////

OpalListener * OpalInternalUDPTransport::CreateListener(const OpalTransportAddress &,
                                                        OpalEndPoint &) const
{
  return NULL;
}


OpalListener * OpalInternalUDPTransport::CreateCompatibleListener(
                                              const OpalTransportAddress & address,
                                              OpalEndPoint & endpoint) const
{
  PIPSocket::Address ip;
  WORD dummy;
  if (address.GetIpAndPort(ip, dummy))
    return new OpalListenerTCP(endpoint, ip, 0);

  return NULL;
}


OpalTransport * OpalInternalUDPTransport::CreateTransport(OpalEndPoint & endpoint) const
{
  return new OpalTransportUDP(endpoint);
}


//////////////////////////////////////////////////////////////////////////

OpalListener::OpalListener(OpalEndPoint & ep)
  : endpoint(ep)
{
  thread = NULL;
  singleThread = FALSE;
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
      if (singleThread) {
        transport->AttachThread(&thread);
        acceptHandler(*this, (INT)transport);
        break;
      }

      transport->AttachThread(PThread::Create(acceptHandler,
                                              (INT)transport,
                                              PThread::NoAutoDeleteThread,
                                              PThread::NormalPriority,
                                              "Opal Answer:%x"));
    }
  }
}


BOOL OpalListener::StartThread(const PNotifier & theAcceptHandler, BOOL isSingleThread)
{
  acceptHandler = theAcceptHandler;
  singleThread = isSingleThread;

  thread = PThread::Create(PCREATE_NOTIFIER(ListenForConnections), 0,
                           PThread::NoAutoDeleteThread,
                           PThread::NormalPriority,
                           "Opal Listener:%x");

  return thread != NULL;
}


//////////////////////////////////////////////////////////////////////////

OpalListenerTCP::OpalListenerTCP(OpalEndPoint & ep,
                                 PIPSocket::Address binding,
                                 WORD port,
                                 BOOL exclusive)
  : OpalListener(ep),
    listener(port),
    localAddress(binding)
{
  exclusiveListener = exclusive;
}


OpalListenerTCP::~OpalListenerTCP()
{
  Close();
}


BOOL OpalListenerTCP::Open(const PNotifier & theAcceptHandler, BOOL isSingleThread)
{
  if (listener.Listen(localAddress))
    return StartThread(theAcceptHandler, isSingleThread);

  if (exclusiveListener) {
    PTRACE(1, "Listen\tOpen on " << localAddress << ':' << listener.GetPort()
           << " failed: " << listener.GetErrorText());
    return FALSE;
  }

  if (listener.GetErrorNumber() != EADDRINUSE)
    return FALSE;

  PTRACE(1, "Listen\tSocket for " << localAddress << ':' << listener.GetPort()
         << " already in use, incoming connections may not all be serviced!");

  if (listener.Listen(localAddress, 100, 0, PSocket::CanReuseAddress))
    return StartThread(theAcceptHandler, isSingleThread);

  PTRACE(1, "Listen\tOpen (REUSEADDR) on " << localAddress << ':' << listener.GetPort()
         << " failed: " << listener.GetErrorText());
  return FALSE;
}


BOOL OpalListenerTCP::IsOpen()
{
  return listener.IsOpen();
}


BOOL OpalListenerTCP::Close()
{
  BOOL ok = listener.Close();

  PAssert(PThread::Current() != thread, PLogicError);
  if (thread != NULL)
    PAssert(thread->WaitForTermination(1000), "Listener thread did not terminate");

  return ok;
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
  }
  else if (socket->GetErrorCode() != PChannel::Interrupted) {
    PTRACE(1, "Listen\tAccept error:" << socket->GetErrorText());
    listener.Close();
  }

  delete socket;
  return NULL;
}


OpalTransportAddress OpalListenerTCP::GetLocalAddress(const OpalTransportAddress & preferredAddress) const
{
  PString addr;

  // If specifically bound to interface use that
  if (localAddress != INADDR_ANY)
    addr = localAddress.AsString();
  else {
    // If bound to all, then use '*' unless a preferred address is specified
    addr = "*";

    PIPSocket::Address ip;
    WORD dummy;
    if (preferredAddress.GetIpAndPort(ip, dummy)) {
      // Verify preferred address is actually an interface in this machine!
      PIPSocket::InterfaceTable interfaces;
      if (PIPSocket::GetInterfaceTable(interfaces)) {
        for (PINDEX i = 0; i < interfaces.GetSize(); i++) {
          if (interfaces[i].GetAddress() == ip) {
            addr = ip.AsString();
            break;
          }
        }
      }
    }
  }

  addr.sprintf(":%u", listener.GetPort());

  return TcpPrefix + addr;
}


//////////////////////////////////////////////////////////////////////////

OpalTransport::OpalTransport(OpalEndPoint & end)
  : endpoint(end)
{
  thread = NULL;
}


OpalTransport::~OpalTransport()
{
  PAssert(thread == NULL, PLogicError);
}


BOOL OpalTransport::Close()
{
  /* Do not use PIndirectChannel::Close() as this deletes the sub-channel
     member field crashing the background thread. Just close the base
     sub-channel so breaks the threads I/O block.
   */
  if (IsOpen())
    GetBaseReadChannel()->Close();

  return TRUE;
}


void OpalTransport::CloseWait()
{
  PTRACE(3, "Opal\tTransport clean up on termination");

  Close();

  if (thread != NULL) {
    PAssert(thread->WaitForTermination(10000), "Transport thread did not terminate");
    delete thread;
    thread = NULL;
  }
}


BOOL OpalTransport::IsCompatibleTransport(const OpalTransportAddress &) const
{
  PAssertAlways(PUnimplementedFunction);
  return FALSE;
}


void OpalTransport::SetPromiscuous(BOOL /*promiscuous*/)
{
}


void OpalTransport::AttachThread(PThread * thrd)
{
  PAssert(thread == NULL, PLogicError);
  thread = thrd;
}


/////////////////////////////////////////////////////////////////////////////

OpalTransportIP::OpalTransportIP(OpalEndPoint & end, PIPSocket::Address binding, WORD remPort)
  : OpalTransport(end),
    localAddress(binding),
    remoteAddress(0)
{
  localPort = 0;
  remotePort = remPort;
}


OpalTransportAddress OpalTransportIP::GetLocalAddress() const
{
  return OpalTransportAddress(localAddress, localPort);
}


OpalTransportAddress OpalTransportIP::GetRemoteAddress() const
{
  return OpalTransportAddress(remoteAddress, remotePort);
}


BOOL OpalTransportIP::SetRemoteAddress(const OpalTransportAddress & address)
{
  if (IsCompatibleTransport(address))
    return address.GetIpAndPort(remoteAddress, remotePort);

  return FALSE;
}


/////////////////////////////////////////////////////////////////////////////

OpalTransportTCP::OpalTransportTCP(OpalEndPoint & ep,
                                   PIPSocket::Address binding,
                                   WORD port)
  : OpalTransportIP(ep, binding, port)
{
}


OpalTransportTCP::OpalTransportTCP(OpalEndPoint & ep, PTCPSocket * socket)
  : OpalTransportIP(ep, INADDR_ANY, 0)
{
  Open(socket);
}


OpalTransportTCP::~OpalTransportTCP()
{
  CloseWait();
}


BOOL OpalTransportTCP::IsCompatibleTransport(const OpalTransportAddress & address) const
{
  return address.Left(sizeof(TcpPrefix)-1) == TcpPrefix ||
         address.Left(sizeof(IpPrefix)-1) == IpPrefix;
}


BOOL OpalTransportTCP::Connect()
{
  if (IsOpen())
    return TRUE;

  PTCPSocket * socket = new PTCPSocket(remotePort);
  Open(socket);

  WORD portBase = endpoint.GetManager().GetTCPPortBase();
  WORD portMax  = endpoint.GetManager().GetTCPPortMax();

  if (portBase == 0) {
    PTRACE(4, "OpalTCP\tConnecting to "
           << remoteAddress << ':' << remotePort);
    if (!socket->Connect(remoteAddress)) {
      PTRACE(1, "OpalTCP\tCould not connect to "
             << remoteAddress << ':' << remotePort << ' ' << socket->GetErrorText());
      return SetErrorValues(socket->GetErrorCode(), socket->GetErrorNumber());
    }
  }
  else {
    for (;;) {
      PTRACE(4, "OpalTCP\tConnecting to "
             << remoteAddress << ':' << remotePort
             << " (local port=" << portBase << ')');
      if (socket->Connect(portBase, remoteAddress))
        break;

      int code = socket->GetErrorNumber();
      if (((code != EADDRINUSE) && (code != EADDRNOTAVAIL)) || (portBase > portMax)) {
        PTRACE(1, "OpalTCP\tCould not connect to "
                  << remoteAddress << ':' << remotePort
                  << ' ' << socket->GetErrorText()
                  << "(" << socket->GetErrorNumber() << ")");
        return SetErrorValues(socket->GetErrorCode(), socket->GetErrorNumber());
      }

      portBase++;
    }
  }

  PTRACE(3, "OpalTCP\tStarting connection to "
         << remoteAddress << ':' << remotePort);

  if (!OnOpen())
    return FALSE;

  return TRUE;
}


BOOL OpalTransportTCP::ReadPDU(PBYTEArray & pdu)
{
  // Make sure is a RFC1006 TPKT
  switch (ReadChar()) {
    case 3 :  // Only support version 3
      break;

    default :  // Unknown version number
      SetErrorValues(ProtocolFailure, 0x80000000);
      // Do case for read error

    case -1 :
      return FALSE;
  }

  // Save timeout
  PTimeInterval oldTimeout = GetReadTimeout();

  // Should get all of PDU in 5 seconds or something is seriously wrong,
  SetReadTimeout(5000);

  // Get TPKT length
  BYTE header[3];
  BOOL ok = ReadBlock(header, sizeof(header));
  if (ok) {
    PINDEX packetLength = ((header[1] << 8)|header[2]) - 4;
    ok = ReadBlock(pdu.GetPointer(packetLength), packetLength);
  }

  SetReadTimeout(oldTimeout);

  return ok;
}


BOOL OpalTransportTCP::WritePDU(const PBYTEArray & pdu)
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


BOOL OpalTransportTCP::OnOpen()
{
  PIPSocket * socket = (PIPSocket *)GetReadChannel();

  // Get name of the remote computer for information purposes
  if (!socket->GetPeerAddress(remoteAddress, remotePort)) {
    PTRACE(1, "OpalTCP\tGetPeerAddress() failed: " << socket->GetErrorText());
    return FALSE;
  }

  // get local address of incoming socket to ensure that multi-homed machines
  // use a NIC address that is guaranteed to be addressable to destination
  if (!socket->GetLocalAddress(localAddress, localPort)) {
    PTRACE(1, "OpalTCP\tGetLocalAddress() failed: " << socket->GetErrorText());
    return FALSE;
  }

#ifndef __BEOS__
  if (!socket->SetOption(TCP_NODELAY, 1, IPPROTO_TCP)) {
    PTRACE(1, "OpalTCP\tSetOption(TCP_NODELAY) failed: " << socket->GetErrorText());
  }

  // make sure do not lose outgoing packets on close
  const linger ling = { 1, 3 };
  if (!socket->SetOption(SO_LINGER, &ling, sizeof(ling))) {
    PTRACE(1, "OpalTCP\tSetOption(SO_LINGER) failed: " << socket->GetErrorText());
    return FALSE;
  }
#endif

  PTRACE(1, "OpalTCP\tStarted connection to "
         << remoteAddress << ':' << remotePort
         << " (if=" << localAddress << ':' << localPort << ')');

  return TRUE;
}


/////////////////////////////////////////////////////////////////////////////

OpalTransportUDP::OpalTransportUDP(OpalEndPoint & ep,
                                   PIPSocket::Address binding,
                                   WORD port,
                                   BOOL promiscuous)
  : OpalTransportIP(ep, binding, port)
{
  promiscuousReads = promiscuous;

  PUDPSocket * udp = new PUDPSocket;
  udp->Listen(binding, 0, port);

  localPort = udp->GetPort();

  Open(udp);

  PTRACE(3, "OpalUDP\tBinding to interface: " << binding << ':' << localPort);
}


OpalTransportUDP::~OpalTransportUDP()
{
  CloseWait();
}


BOOL OpalTransportUDP::IsCompatibleTransport(const OpalTransportAddress & address) const
{
  return address.Left(sizeof(UdpPrefix)-1) == UdpPrefix ||
         address.Left(sizeof(IpPrefix)-1) == IpPrefix;
}


BOOL OpalTransportUDP::Connect()
{
  if (remoteAddress == 0 || remotePort == 0)
    return FALSE;

  PUDPSocket * socket = (PUDPSocket *)GetReadChannel();
  socket->SetSendAddress(remoteAddress, remotePort);

  return TRUE;
}


void OpalTransportUDP::SetPromiscuous(BOOL promiscuous)
{
  promiscuousReads = promiscuous;
}


BOOL OpalTransportUDP::ReadPDU(PBYTEArray & pdu)
{
  for (;;) {
    if (!Read(pdu.GetPointer(10000), 10000)) {
      pdu.SetSize(0);
      return FALSE;
    }

    pdu.SetSize(GetLastReadCount());

    PUDPSocket * socket = (PUDPSocket *)GetReadChannel();

    PIPSocket::Address address;
    WORD port;

    socket->GetLastReceiveAddress(address, port);
    if (promiscuousReads) {
      remoteAddress = address;
      remotePort = port;
      socket->SetSendAddress(remoteAddress, remotePort);
      return TRUE;
    }

    if (remoteAddress == address && remotePort == port)
      return TRUE;

    PTRACE(1, "UDP\tReceived PDU from incorrect host: " << address << ':' << port);
  }
}


BOOL OpalTransportUDP::WritePDU(const PBYTEArray & pdu)
{
  return Write((const BYTE *)pdu, pdu.GetSize());
}


//////////////////////////////////////////////////////////////////////////
