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
 * Revision 1.2015  2002/04/10 03:12:35  robertj
 * Fixed SetLocalAddress to return FALSE if did not set the address to a
 *   different address to the current one. Altered UDP version to cope.
 *
 * Revision 2.13  2002/04/09 04:44:36  robertj
 * Fixed bug where crahses if close channel while in UDP connect mode.
 *
 * Revision 2.12  2002/04/09 00:22:16  robertj
 * Added ability to set the local address on a transport, under some circumstances.
 *
 * Revision 2.11  2002/03/27 05:37:39  robertj
 * Fixed removal of writeChannel after wrinting to UDP transport in connect mode.
 *
 * Revision 2.10  2002/03/15 00:20:54  robertj
 * Fixed bug when closing UDP transport when in "connect" mode.
 *
 * Revision 2.9  2002/02/06 06:07:10  robertj
 * Fixed shutting down UDP listener thread
 *
 * Revision 2.8  2002/01/14 00:19:33  craigs
 * Fixed problems with remote address used instead of local address
 *
 * Revision 2.7  2001/12/07 08:55:32  robertj
 * Used UDP port base when creating UDP transport.
 *
 * Revision 2.6  2001/11/14 06:28:20  robertj
 * Added missing break when ending UDP connect phase.
 *
 * Revision 2.5  2001/11/13 04:29:48  robertj
 * Changed OpalTransportAddress CreateTransport and CreateListsner functions
 *   to have extra parameter to control local binding of sockets.
 *
 * Revision 2.4  2001/11/12 05:31:36  robertj
 * Changed CreateTransport() on an OpalTransportAddress to bind to local address.
 * Added OpalTransportAddress::GetIpAddress when don't need port number.
 *
 * Revision 2.3  2001/11/09 05:49:47  robertj
 * Abstracted UDP connection algorithm
 *
 * Revision 2.2  2001/11/06 05:40:13  robertj
 * Added OpalListenerUDP class to do listener semantics on a UDP socket.
 *
 * Revision 2.1  2001/10/03 05:53:25  robertj
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

#include <ptclib/asner.h>


static const char IpPrefix[] = "ip$"; // For backward compatibility with OpenH323

const char * OpalTransportAddress::TcpPrefix = "tcp$";
const char * OpalTransportAddress::UdpPrefix = "udp$";


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
      OpalEndPoint & endpoint,
      OpalTransportAddress::BindOptions options
    ) const  = 0;

    virtual OpalTransport * CreateTransport(
      const OpalTransportAddress & address,
      OpalEndPoint & endpoint,
      OpalTransportAddress::BindOptions options
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
      OpalEndPoint & endpoint,
      OpalTransportAddress::BindOptions options
    ) const;
    virtual OpalTransport * CreateTransport(
      const OpalTransportAddress & address,
      OpalEndPoint & endpoint,
      OpalTransportAddress::BindOptions options
    ) const;
} internalTCPTransport;


////////////////////////////////////////////////////////////////

static class OpalInternalUDPTransport : public OpalInternalIPTransport
{
    PCLASSINFO(OpalInternalUDPTransport, OpalInternalIPTransport);
  public:
    virtual OpalListener * CreateListener(
      const OpalTransportAddress & address,
      OpalEndPoint & endpoint,
      OpalTransportAddress::BindOptions options
    ) const;
    virtual OpalTransport * CreateTransport(
      const OpalTransportAddress & address,
      OpalEndPoint & endpoint,
      OpalTransportAddress::BindOptions options
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
  

BOOL OpalTransportAddress::GetIpAddress(PIPSocket::Address & ip) const
{
  if (transport == NULL)
    return FALSE;

  WORD dummy;
  return transport->GetIpAndPort(*this, ip, dummy);
}


BOOL OpalTransportAddress::GetIpAndPort(PIPSocket::Address & ip, WORD & port) const
{
  if (transport == NULL)
    return FALSE;

  return transport->GetIpAndPort(*this, ip, port);
}


OpalListener * OpalTransportAddress::CreateListener(OpalEndPoint & endpoint,
                                                    BindOptions option) const
{
  if (transport == NULL)
    return NULL;

  return transport->CreateListener(*this, endpoint, option);
}


OpalTransport * OpalTransportAddress::CreateTransport(OpalEndPoint & endpoint,
                                                      BindOptions option) const
{
  if (transport == NULL)
    return NULL;

  return transport->CreateTransport(*this, endpoint, option);
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

  if (host == "*" || host == "0.0.0.0") {
    ip = INADDR_ANY;
    return TRUE;
  }

  if (PIPSocket::GetHostAddress(host, ip))
    return TRUE;

  PTRACE(1, "Opal\tCould not find host \"" << host << '"');
  return FALSE;
}


//////////////////////////////////////////////////////////////////////////

static BOOL GetAdjustedIpAndPort(const OpalTransportAddress & address,
                                 OpalEndPoint & endpoint,
                                 OpalTransportAddress::BindOptions option,
                                 PIPSocket::Address & ip,
                                 WORD & port,
                                 BOOL & reuseAddr)
{
  reuseAddr = address[address.GetLength()-1] != '+';

  switch (option) {
    case OpalTransportAddress::NoBinding :
      ip = INADDR_ANY;
      port = 0;
      return TRUE;

    case OpalTransportAddress::HostOnly :
      port = 0;
      return address.GetIpAddress(ip);

    default :
      port = endpoint.GetDefaultSignalPort();
      return address.GetIpAndPort(ip, port);
  }
}


OpalListener * OpalInternalTCPTransport::CreateListener(const OpalTransportAddress & address,
                                                        OpalEndPoint & endpoint,
                                                        OpalTransportAddress::BindOptions option) const
{
  PIPSocket::Address ip;
  WORD port;
  BOOL reuseAddr;
  if (GetAdjustedIpAndPort(address, endpoint, option, ip, port, reuseAddr))
    return new OpalListenerTCP(endpoint, ip, port, reuseAddr);

  return NULL;
}


OpalTransport * OpalInternalTCPTransport::CreateTransport(const OpalTransportAddress & address,
                                                          OpalEndPoint & endpoint,
                                                          OpalTransportAddress::BindOptions option) const
{
  PIPSocket::Address ip;
  WORD port;
  BOOL reuseAddr;
  if (GetAdjustedIpAndPort(address, endpoint, option, ip, port, reuseAddr))
    return new OpalTransportTCP(endpoint, ip, port, reuseAddr);

  return NULL;
}


//////////////////////////////////////////////////////////////////////////

OpalListener * OpalInternalUDPTransport::CreateListener(const OpalTransportAddress & address,
                                                        OpalEndPoint & endpoint,
                                                        OpalTransportAddress::BindOptions option) const
{
  PIPSocket::Address ip;
  WORD port;
  BOOL reuseAddr;
  if (GetAdjustedIpAndPort(address, endpoint, option, ip, port, reuseAddr))
    return new OpalListenerUDP(endpoint, ip, port, reuseAddr);

  return NULL;
}


OpalTransport * OpalInternalUDPTransport::CreateTransport(const OpalTransportAddress & address,
                                                          OpalEndPoint & endpoint,
                                                          OpalTransportAddress::BindOptions option) const
{
  PIPSocket::Address ip;
  WORD port;
  BOOL reuseAddr;
  if (GetAdjustedIpAndPort(address, endpoint, option, ip, port, reuseAddr))
    return new OpalTransportUDP(endpoint, ip, port, reuseAddr);

  return NULL;
}


//////////////////////////////////////////////////////////////////////////

OpalListener::OpalListener(OpalEndPoint & ep)
  : endpoint(ep)
{
  thread = NULL;
  singleThread = FALSE;
}


void OpalListener::PrintOn(ostream & strm) const
{
  strm << GetLocalAddress();
}


void OpalListener::CloseWait()
{
  PTRACE(3, "Listen\tStopping listening thread on " << GetLocalAddress());
  Close();

  PAssert(PThread::Current() != thread, PLogicError);
  if (thread != NULL)
    PAssert(thread->WaitForTermination(1000), "Listener thread did not terminate");
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
      }
      else {
        transport->AttachThread(PThread::Create(acceptHandler,
                                                (INT)transport,
                                                PThread::NoAutoDeleteThread,
                                                PThread::NormalPriority,
                                                "Opal Answer:%x"));
      }
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

OpalListenerIP::OpalListenerIP(OpalEndPoint & ep,
                               PIPSocket::Address binding,
                               WORD port,
                               BOOL exclusive)
  : OpalListener(ep),
    localAddress(binding)
{
  listenerPort = port;
  exclusiveListener = exclusive;
}


OpalTransportAddress OpalListenerIP::GetLocalAddress(const OpalTransportAddress & preferredAddress) const
{
  PString addr;

  // If specifically bound to interface use that
  if (localAddress != INADDR_ANY)
    addr = localAddress.AsString();
  else {
    // If bound to all, then use '*' unless a preferred address is specified
    addr = "*";

    PIPSocket::Address ip;
    if (preferredAddress.GetIpAddress(ip)) {
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

  addr.sprintf(":%u", listenerPort);

  return GetProtoPrefix() + addr;
}


//////////////////////////////////////////////////////////////////////////

OpalListenerTCP::OpalListenerTCP(OpalEndPoint & ep,
                                 PIPSocket::Address binding,
                                 WORD port,
                                 BOOL exclusive)
  : OpalListenerIP(ep, binding, port, exclusive),
    listener(port)
{
  listenerPort = listener.GetPort();
}


OpalListenerTCP::~OpalListenerTCP()
{
  CloseWait();
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
  }
  else if (socket->GetErrorCode() != PChannel::Interrupted) {
    PTRACE(1, "Listen\tAccept error:" << socket->GetErrorText());
    listener.Close();
  }

  delete socket;
  return NULL;
}


const char * OpalListenerTCP::GetProtoPrefix() const
{
  return OpalTransportAddress::TcpPrefix;
}


//////////////////////////////////////////////////////////////////////////

OpalListenerUDP::OpalListenerUDP(OpalEndPoint & endpoint,
                                 PIPSocket::Address binding,
                                 WORD port,
                                 BOOL exclusive)
  : OpalListenerIP(endpoint, binding, port, exclusive)
{
}


OpalListenerUDP::~OpalListenerUDP()
{
  CloseWait();
}


BOOL OpalListenerUDP::OpenOneSocket(const PIPSocket::Address & address)
{
  PUDPSocket * socket = new PUDPSocket(listenerPort);
  if (socket->Listen(address)) {
    listeners.Append(socket);
    if (listenerPort == 0)
      listenerPort = socket->GetPort();
    return TRUE;
  }

  PTRACE(1, "Listen\tError in UDP listen: " << socket->GetErrorText());
  delete socket;
  return FALSE;
}


BOOL OpalListenerUDP::Open(const PNotifier & theAcceptHandler, BOOL /*isSingleThread*/)
{
  PIPSocket::InterfaceTable interfaces;
  if (!PIPSocket::GetInterfaceTable(interfaces)) {
    PTRACE(1, "Listen\tNo interfaces on system!");
    return OpenOneSocket(localAddress);
  }

  for (PINDEX i = 0; i < interfaces.GetSize(); i++) {
    PIPSocket::Address addr = interfaces[i].GetAddress();
    if (addr != 0 && (localAddress == INADDR_ANY || localAddress == addr)) {
      if (OpenOneSocket(addr)) {
        PIPSocket::Address mask = interfaces[i].GetNetMask();
        if (mask != 0 && mask != 0xffffffff)
          OpenOneSocket((addr&mask)|(0xffffffff&~mask));
      }
    }
  }

  if (listeners.GetSize() > 0)
    return StartThread(theAcceptHandler, TRUE);

  PTRACE(1, "Listen\tCould not start any UDP listeners");
  return FALSE;
}


BOOL OpalListenerUDP::IsOpen()
{
  for (PINDEX i = 0; i < listeners.GetSize(); i++) {
    if (listeners[i].IsOpen())
      return TRUE;
  }
  return FALSE;
}


void OpalListenerUDP::Close()
{
  for (PINDEX i = 0; i < listeners.GetSize(); i++)
    listeners[i].Close();
}


OpalTransport * OpalListenerUDP::Accept(const PTimeInterval & timeout)
{
  if (listeners.IsEmpty())
    return NULL;

  PSocket::SelectList selection;
  PINDEX i;
  for (i = 0; i < listeners.GetSize(); i++)
    selection += listeners[i];

  PTRACE(4, "Listen\tWaiting on UDP packet on " << GetLocalAddress());
  PChannel::Errors error = PSocket::Select(selection, timeout);

  if (error != PChannel::NoError || selection.IsEmpty()) {
    PTRACE(1, "Listen\tUDP select error: " << PSocket::GetErrorText(error));
    return NULL;
  }

  PUDPSocket & socket = (PUDPSocket &)selection[0];

  if (singleThread)
    return new OpalTransportUDP(endpoint, socket);

  PBYTEArray pdu;
  PIPSocket::Address addr;
  WORD port;
  if (socket.ReadFrom(pdu.GetPointer(2000), 2000, addr, port))
    return new OpalTransportUDP(endpoint, localAddress, pdu, addr, port);

  PTRACE(1, "Listen\tUDP read error: " << socket.GetErrorText());
  return NULL;
}


const char * OpalListenerUDP::GetProtoPrefix() const
{
  return OpalTransportAddress::UdpPrefix;
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


void OpalTransport::PrintOn(ostream & strm) const
{
  strm << GetRemoteAddress() << "<if=" << GetLocalAddress() << '>';
}


void OpalTransport::EndConnect(const OpalTransportAddress &)
{
}


BOOL OpalTransport::Close()
{
  /* Do not use PIndirectChannel::Close() as this deletes the sub-channel
     member field crashing the background thread. Just close the base
     sub-channel so breaks the threads I/O block.
   */
  if (IsOpen())
    GetBaseWriteChannel()->Close();

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


BOOL OpalTransport::WriteConnect(WriteConnectCallback function, PObject * data)
{
  return function(*this, data);
}


void OpalTransport::AttachThread(PThread * thrd)
{
  if (thread != NULL) {
    PAssert(thread->WaitForTermination(10000), "Transport not terminated when reattaching thread");
    delete thread;
  }

  thread = thrd;
}


BOOL OpalTransport::IsRunning() const
{
  if (thread == NULL)
    return FALSE;

  return !thread->IsTerminated();
}


/////////////////////////////////////////////////////////////////////////////

OpalTransportIP::OpalTransportIP(OpalEndPoint & end,
                                 PIPSocket::Address binding,
                                 WORD port)
  : OpalTransport(end),
    localAddress(binding),
    remoteAddress(0)
{
  localPort = port;
  remotePort = 0;
}


OpalTransportAddress OpalTransportIP::GetLocalAddress() const
{
  return OpalTransportAddress(localAddress, localPort, GetProtoPrefix());
}


BOOL OpalTransportIP::SetLocalAddress(const OpalTransportAddress & newLocalAddress)
{
  if (!IsCompatibleTransport(newLocalAddress))
    return FALSE;

  if (!IsOpen())
    return newLocalAddress.GetIpAndPort(localAddress, localPort);

  PIPSocket::Address address;
  WORD port;
  if (!newLocalAddress.GetIpAndPort(address, port))
    return FALSE;

  return localAddress == address && localPort == port;
}


OpalTransportAddress OpalTransportIP::GetRemoteAddress() const
{
  return OpalTransportAddress(remoteAddress, remotePort, GetProtoPrefix());
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
                                   WORD port,
                                   BOOL reuseAddr)
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
}


BOOL OpalTransportTCP::IsReliable() const
{
  return TRUE;
}


BOOL OpalTransportTCP::IsCompatibleTransport(const OpalTransportAddress & address) const
{
  return address.Left(strlen(OpalTransportAddress::TcpPrefix)) == OpalTransportAddress::TcpPrefix ||
         address.Left(sizeof(IpPrefix)-1) == IpPrefix;
}


BOOL OpalTransportTCP::Connect()
{
  if (IsOpen())
    return TRUE;

  PTCPSocket * socket = new PTCPSocket(remotePort);
  Open(socket);

  WORD portBase, portMax;
  if (localPort == 0) {
    portBase = endpoint.GetManager().GetTCPPortBase();
    portMax  = endpoint.GetManager().GetTCPPortMax();
  }
  else {
    portBase = localPort;
    portMax = localPort;
  }

  if (portBase == 0) {
    PTRACE(4, "OpalTCP\tConnecting to "
           << remoteAddress << ':' << remotePort << " (if=" << localAddress << ')');
    if (!socket->Connect(localAddress, remoteAddress)) {
      PTRACE(1, "OpalTCP\tCould not connect to "
             << remoteAddress << ':' << remotePort << ' ' << socket->GetErrorText());
      return SetErrorValues(socket->GetErrorCode(), socket->GetErrorNumber());
    }
  }
  else {
    for (;;) {
      PTRACE(4, "OpalTCP\tConnecting to "
             << remoteAddress << ':' << remotePort
             << " (if=" << localAddress << ':' << portBase << ')');
      if (socket->Connect(localAddress, portBase, remoteAddress))
        break;

      int code = socket->GetErrorNumber();
      if (((code != EADDRINUSE) && (code != EADDRNOTAVAIL)) || (portBase > portMax)) {
        PTRACE(1, "OpalTCP\tCould not connect to "
                  << remoteAddress << ':' << remotePort
                  << " (if=" << localAddress << ':' << portBase << ") "
                  << socket->GetErrorText()
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


const char * OpalTransportTCP::GetProtoPrefix() const
{
  return OpalTransportAddress::TcpPrefix;
}


/////////////////////////////////////////////////////////////////////////////

OpalTransportUDP::OpalTransportUDP(OpalEndPoint & ep,
                                   PIPSocket::Address binding,
                                   WORD port,
                                   BOOL reuseAddr)
  : OpalTransportIP(ep, binding, port)
{
  promiscuousReads = FALSE;

  PUDPSocket * udp = new PUDPSocket;
  udp->Listen(binding, 0, port,
              reuseAddr ? PSocket::CanReuseAddress : PSocket::AddressIsExclusive);

  localPort = udp->GetPort();

  Open(udp);

  PTRACE(3, "OpalUDP\tBinding to interface: " << localAddress << ':' << localPort);
}


OpalTransportUDP::OpalTransportUDP(OpalEndPoint & ep, PUDPSocket & udp)
  : OpalTransportIP(ep, INADDR_ANY, 0)
{
  promiscuousReads = TRUE;

  udp.GetLocalAddress(localAddress, localPort);

  Open(udp);

  PTRACE(3, "OpalUDP\tPre-bound to interface: " << localAddress << ':' << localPort);
}


OpalTransportUDP::OpalTransportUDP(OpalEndPoint & ep,
                                   PIPSocket::Address binding,
                                   const PBYTEArray & packet,
                                   PIPSocket::Address remAddr,
                                   WORD remPort)
  : OpalTransportIP(ep, binding, 0),
    preReadPacket(packet)
{
  promiscuousReads = TRUE;

  remoteAddress = remAddr;
  remotePort = remPort;

  PUDPSocket * udp = new PUDPSocket;
  udp->Listen(binding);

  localPort = udp->GetPort();

  Open(udp);

  PTRACE(3, "OpalUDP\tBinding to interface: " << localAddress << ':' << localPort);
}


OpalTransportUDP::~OpalTransportUDP()
{
  if (connectSockets.GetSize() > 0)
    writeChannel = readChannel = NULL;

  CloseWait();
}


BOOL OpalTransportUDP::IsReliable() const
{
  return FALSE;
}


BOOL OpalTransportUDP::IsCompatibleTransport(const OpalTransportAddress & address) const
{
  return address.Left(strlen(OpalTransportAddress::UdpPrefix)) == OpalTransportAddress::UdpPrefix ||
         address.Left(sizeof(IpPrefix)-1) == IpPrefix;
}


BOOL OpalTransportUDP::Connect()
{
  if (remotePort == 0)
    return FALSE;

  if (remoteAddress == 0) {
    remoteAddress = INADDR_BROADCAST;
    PTRACE(2, "OpalUDP\tBroadcast connect to port " << remotePort);
  }
  else {
    PTRACE(2, "OpalUDP\tStarted connect to " << remoteAddress << ':' << remotePort);
  }

  // Skip over the OpalTransportUDP::Close to make sure PUDPSocket is deleted.
  PIndirectChannel::Close();
  readAutoDelete = writeAutoDelete = FALSE;

  WORD portBase = endpoint.GetManager().GetUDPPortBase();
  WORD portMax  = endpoint.GetManager().GetUDPPortMax();

  // See if prebound to interface, only use that if so
  PIPSocket::InterfaceTable interfaces;
  if (localAddress != INADDR_ANY) {
    PTRACE(3, "OpalUDP\tConnect on pre-bound interface: " << localAddress);
    interfaces.Append(new PIPSocket::InterfaceEntry("", localAddress, PIPSocket::Address(0xffffffff), ""));
  }
  else if (!PIPSocket::GetInterfaceTable(interfaces)) {
    PTRACE(1, "OpalUDP\tNo interfaces on system!");
    interfaces.Append(new PIPSocket::InterfaceEntry("", localAddress, PIPSocket::Address(0xffffffff), ""));
  }
  else {
    PTRACE(4, "OpalUDP\tConnecting to interfaces:\n" << setfill('\n') << interfaces << setfill(' '));
  }

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

    // Not explicitly multicast
    PUDPSocket * socket = new PUDPSocket;
    connectSockets.Append(socket);

    localPort = portBase;
    while (!socket->Listen(interfaceAddress, 0, localPort)) {
      localPort++;
      if (localPort > portMax) {
        PTRACE(2, "OpalUDP\tError on connect listen: " << socket->GetErrorText());
        return FALSE;
      }
    }

#ifndef __BEOS__
    if (remoteAddress == INADDR_BROADCAST) {
      if (!socket->SetOption(SO_BROADCAST, 1)) {
        PTRACE(2, "OpalUDP\tError allowing broadcast: " << socket->GetErrorText());
        return FALSE;
      }
    }
#else
    PTRACE(3, "RAS\tBroadcast option under BeOS is not implemented yet");
#endif

    socket->SetSendAddress(remoteAddress, remotePort);
  }

  if (connectSockets.IsEmpty())
    return FALSE;

  SetPromiscuous(TRUE);

  return TRUE;
}


void OpalTransportUDP::EndConnect(const OpalTransportAddress & theLocalAddress)
{
  PAssert(theLocalAddress.GetIpAndPort(localAddress, localPort), PInvalidParameter);

  for (PINDEX i = 0; i < connectSockets.GetSize(); i++) {
    PUDPSocket * socket = (PUDPSocket *)connectSockets.GetAt(i);
    PIPSocket::Address addr;
    WORD port;
    if (socket->GetLocalAddress(addr, port) && addr == localAddress && port == localPort) {
      PTRACE(3, "OpalUDP\tEnded connect, selecting " << localAddress << ':' << localPort);
      connectSockets.DisallowDeleteObjects();
      connectSockets.RemoveAt(i);
      connectSockets.AllowDeleteObjects();
      readChannel = NULL;
      writeChannel = NULL;
      socket->SetOption(SO_BROADCAST, 0);
      PAssert(Open(socket), PLogicError);
      break;
    }
  }

  connectSockets.RemoveAll();

  OpalTransport::EndConnect(theLocalAddress);
}


BOOL OpalTransportUDP::SetLocalAddress(const OpalTransportAddress & newLocalAddress)
{
  if (connectSockets.IsEmpty())
    return OpalTransportIP::SetLocalAddress(newLocalAddress);

  if (!IsCompatibleTransport(newLocalAddress))
    return FALSE;

  if (!newLocalAddress.GetIpAndPort(localAddress, localPort))
    return FALSE;

  for (PINDEX i = 0; i < connectSockets.GetSize(); i++) {
    PUDPSocket * socket = (PUDPSocket *)connectSockets.GetAt(i);
    PIPSocket::Address ip;
    WORD port;
    if (socket->GetLocalAddress(ip, port) && ip == localAddress && port == localPort) {
      writeChannel = &connectSockets[i];
      return TRUE;
    }
  }

  return FALSE;
}


BOOL OpalTransportUDP::SetRemoteAddress(const OpalTransportAddress & address)
{
  if (!OpalTransportIP::SetRemoteAddress(address))
    return FALSE;

  PUDPSocket * socket = (PUDPSocket *)GetReadChannel();
  socket->SetSendAddress(remoteAddress, remotePort);
  return TRUE;
}


void OpalTransportUDP::SetPromiscuous(BOOL promiscuous)
{
  promiscuousReads = promiscuous;
}


BOOL OpalTransportUDP::Read(void * buffer, PINDEX length)
{
  if (!connectSockets.IsEmpty()) {
    PSocket::SelectList selection;

    PINDEX i;
    for (i = 0; i < connectSockets.GetSize(); i++)
      selection += connectSockets[i];

    if (PSocket::Select(selection, GetReadTimeout()) != PChannel::NoError) {
      PTRACE(1, "OpalUDP\tError on connection read select.");
      return FALSE;
    }

    if (selection.IsEmpty()) {
      PTRACE(2, "OpalUDP\tTimeout on connection read select.");
      return FALSE;
    }

    PUDPSocket & socket = (PUDPSocket &)selection[0];
    socket.GetLocalAddress(localAddress, localPort);
    readChannel = &socket;
  }

  for (;;) {
    if (!OpalTransportIP::Read(buffer, length))
      return FALSE;

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

    if (remoteAddress == address)
      return TRUE;

    PTRACE(1, "UDP\tReceived PDU from incorrect host: " << address << ':' << port);
  }
}


BOOL OpalTransportUDP::ReadPDU(PBYTEArray & packet)
{
  if (preReadPacket.GetSize() > 0) {
    packet = preReadPacket;
    preReadPacket.SetSize(0);
    return TRUE;
  }

  if (!Read(packet.GetPointer(10000), 10000)) {
    packet.SetSize(0);
    return FALSE;
  }

  packet.SetSize(GetLastReadCount());
  return TRUE;
}


BOOL OpalTransportUDP::WritePDU(const PBYTEArray & packet)
{
  return Write((const BYTE *)packet, packet.GetSize());
}


BOOL OpalTransportUDP::WriteConnect(WriteConnectCallback function, PObject * data)
{
  if (connectSockets.IsEmpty())
    return OpalTransport::WriteConnect(function, data);

  BOOL ok = FALSE;

  PINDEX i;
  for (i = 0; i < connectSockets.GetSize(); i++) {
    PUDPSocket & socket = (PUDPSocket &)connectSockets[i];
    socket.GetLocalAddress(localAddress, localPort);
    writeChannel = &socket;
    if (function(*this, data))
      ok = TRUE;
  }

  return ok;
}


const char * OpalTransportUDP::GetProtoPrefix() const
{
  return OpalTransportAddress::UdpPrefix;
}


//////////////////////////////////////////////////////////////////////////
