/*
 * transaddr.cxx
 *
 * H.323 transports handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2000 Equivalence Pty. Ltd.
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
 * Vovida Networks, Inc. http://www.vovida.com.
 *
 * Contributor(s): ______________________________________.
 *
 * $Log: transaddr.cxx,v $
 * Revision 2.10  2006/08/21 05:29:25  csoutheren
 * Messy but relatively simple change to add support for secure (SSL/TLS) TCP transport
 * and secure H.323 signalling via the sh323 URL scheme
 *
 * Revision 2.9  2005/01/16 23:07:34  csoutheren
 * Fixed problem with IPv6 INADDR_ANY
 *
 * Revision 2.8  2004/04/07 08:21:10  rjongbloed
 * Changes for new RTTI system.
 *
 * Revision 2.7  2004/02/24 11:37:02  rjongbloed
 * More work on NAT support, manual external address translation and STUN
 *
 * Revision 2.6  2004/02/19 10:47:04  rjongbloed
 * Merged OpenH323 version 1.13.1 changes.
 *
 * Revision 2.5  2002/11/10 11:33:19  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.4  2002/09/12 06:58:33  robertj
 * Removed protocol prefix strings as static members as has problems with
 *   use in DLL environment.
 *
 * Revision 2.3  2002/07/01 04:56:32  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 2.2  2001/11/12 05:32:12  robertj
 * Added OpalTransportAddress::GetIpAddress when don't need port number.
 *
 * Revision 2.1  2001/11/09 05:49:47  robertj
 * Abstracted UDP connection algorithm
 *
 * Revision 2.0  2001/07/27 15:48:25  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 */

#include <ptlib.h>

#ifdef __GNUC__
#pragma implementation "transaddr.h"
#endif

#include <h323/transaddr.h>

#include <h323/h323ep.h>
#include <h323/h323pdu.h>



static PString BuildIP(const PIPSocket::Address & ip, unsigned port, const char * proto)
{
  PStringStream str;

  if (proto == NULL)
    str << "ip$"; // Compatible with both tcp$ and udp$
  else {
    str << proto;
    if (str.Find('$') == P_MAX_INDEX)
      str << '$';
  }

  if (!ip.IsValid())
    str << '*';
  else
#if P_HAS_IPV6
  if (ip.GetVersion() == 6)
    str << '[' << ip << ']';
  else
#endif
    str << ip;

  if (port != 0)
    str << ':' << port;

  return str;
}


/////////////////////////////////////////////////////////////////////////////

H323TransportAddress::H323TransportAddress(const H225_TransportAddress & transport,
                                           const char * proto)
{
  switch (transport.GetTag()) {
    case H225_TransportAddress::e_ipAddress :
    {
      const H225_TransportAddress_ipAddress & ip = transport;
      *this = BuildIP(PIPSocket::Address(ip.m_ip.GetSize(), ip.m_ip.GetValue()), ip.m_port, proto);
      break;
    }
#if P_HAS_IPV6
    case H225_TransportAddress::e_ip6Address :
    {
      const H225_TransportAddress_ip6Address & ip = transport;
      *this = BuildIP(PIPSocket::Address(ip.m_ip.GetSize(), ip.m_ip.GetValue()), ip.m_port, proto);
      break;
    }
#endif
  }

  SetInternalTransport(0, NULL);
}


H323TransportAddress::H323TransportAddress(const H245_TransportAddress & transport,
                                           const char * proto)
{
  switch (transport.GetTag()) {
    case H245_TransportAddress::e_unicastAddress :
    {
      const H245_UnicastAddress & unicast = transport;
      switch (unicast.GetTag()) {
        case H245_UnicastAddress::e_iPAddress :
        {
          const H245_UnicastAddress_iPAddress & ip = unicast;
          *this = BuildIP(PIPSocket::Address(ip.m_network.GetSize(), ip.m_network.GetValue()), ip.m_tsapIdentifier, proto);
          break;
        }
#if P_HAS_IPV6
        case H245_UnicastAddress::e_iP6Address :
        {
          const H245_UnicastAddress_iP6Address & ip = unicast;
          *this = BuildIP(PIPSocket::Address(ip.m_network.GetSize(), ip.m_network.GetValue()), ip.m_tsapIdentifier, proto);
          break;
        }
#endif
      }
      break;
    }
  }

  SetInternalTransport(0, NULL);
}


static void AppendTransportAddress(OpalManager & manager,
                                   const OpalTransport & associatedTransport,
                                   PIPSocket::Address addr, WORD port,
                                   H225_ArrayOf_TransportAddress & pdu)
{
  PTRACE(4, "TCP\tAppending H.225 transport " << addr << ':' << port
         << " using associated transport " << associatedTransport);

  PIPSocket::Address remoteIP;
  if (associatedTransport.GetRemoteAddress().GetIpAddress(remoteIP))
    manager.TranslateIPAddress(addr, remoteIP);

  H323TransportAddress transAddr(addr, port);
  H225_TransportAddress pduAddr;
  transAddr.SetPDU(pduAddr, associatedTransport.GetEndPoint().GetDefaultSignalPort());

  PINDEX lastPos = pdu.GetSize();

  // Check for already have had that address.
  for (PINDEX i = 0; i < lastPos; i++) {
    if (pdu[i] == pduAddr)
      return;
  }

  // Put new listener into array
  pdu.SetSize(lastPos+1);
  pdu[lastPos] = pduAddr;
}


BOOL H323TransportAddress::SetPDU(H225_ArrayOf_TransportAddress & pdu,
                                  const OpalTransport & associatedTransport)
{
  OpalManager & manager = associatedTransport.GetEndPoint().GetManager();

  PIPSocket::Address ip;
  WORD port = associatedTransport.GetEndPoint().GetDefaultSignalPort(); //H323EndPoint::DefaultTcpPort;
  if (!GetIpAndPort(ip, port))
    return FALSE;

  if (!ip.IsAny()) {
    AppendTransportAddress(manager, associatedTransport, ip, port, pdu);
    return TRUE;
  }

  PIPSocket::InterfaceTable interfaces;
  if (!PIPSocket::GetInterfaceTable(interfaces)) {
    PIPSocket::Address ipAddr;
    PIPSocket::GetHostAddress(ipAddr);
    AppendTransportAddress(manager, associatedTransport, ipAddr, port, pdu);
    return TRUE;
  }

  PINDEX i;

  PIPSocket::Address firstAddress;
  if (associatedTransport.GetLocalAddress().GetIpAddress(firstAddress)) {
    for (i = 0; i < interfaces.GetSize(); i++) {
      PIPSocket::Address ipAddr = interfaces[i].GetAddress();
      if (ipAddr == firstAddress)
        AppendTransportAddress(manager, associatedTransport, ipAddr, port, pdu);
    }
  }

  for (i = 0; i < interfaces.GetSize(); i++) {
    PIPSocket::Address ipAddr = interfaces[i].GetAddress();
    if (ipAddr != 0 && ipAddr != firstAddress && ipAddr != PIPSocket::Address()) // Ignore 127.0.0.1
      AppendTransportAddress(manager, associatedTransport, ipAddr, port, pdu);
  }

  return TRUE;
}


BOOL H323TransportAddress::SetPDU(H225_TransportAddress & pdu, WORD defPort) const
{
  PIPSocket::Address ip;
  WORD port = defPort;
  if (GetIpAndPort(ip, port)) {
#if P_HAS_IPV6
    if (ip.GetVersion() == 6) {
      pdu.SetTag(H225_TransportAddress::e_ip6Address);
      H225_TransportAddress_ip6Address & addr = pdu;
      for (PINDEX i = 0; i < ip.GetSize(); i++)
        addr.m_ip[i] = ip[i];
      addr.m_port = port;
      return TRUE;
    }
#endif

    PAssert(port != 0, "Attempt to set transport address with empty port");

    pdu.SetTag(H225_TransportAddress::e_ipAddress);
    H225_TransportAddress_ipAddress & addr = pdu;
    for (PINDEX i = 0; i < 4; i++)
      addr.m_ip[i] = ip[i];
    addr.m_port = port;
    return TRUE;
  }

  return FALSE;
}


BOOL H323TransportAddress::SetPDU(H245_TransportAddress & pdu, WORD defPort) const
{
  WORD port = defPort;
  if (defPort == 0)
    defPort = H323EndPoint::DefaultTcpSignalPort;

  PIPSocket::Address ip;
  if (GetIpAndPort(ip, port)) {
    pdu.SetTag(H245_TransportAddress::e_unicastAddress);

    H245_UnicastAddress & unicast = pdu;

#if P_HAS_IPV6
    if (ip.GetVersion() == 6) {
      unicast.SetTag(H245_UnicastAddress::e_iP6Address);
      H245_UnicastAddress_iP6Address & addr = unicast;
      for (PINDEX i = 0; i < ip.GetSize(); i++)
        addr.m_network[i] = ip[i];
      addr.m_tsapIdentifier = port;
      return TRUE;
    }
#endif

    unicast.SetTag(H245_UnicastAddress::e_iPAddress);
    H245_UnicastAddress_iPAddress & addr = unicast;
    for (PINDEX i = 0; i < 4; i++)
      addr.m_network[i] = ip[i];
    addr.m_tsapIdentifier = port;
    return TRUE;
  }

  return FALSE;
}


/////////////////////////////////////////////////////////////////////////////

H323TransportAddressArray::H323TransportAddressArray(const H225_ArrayOf_TransportAddress & addresses)
{
  for (PINDEX i = 0; i < addresses.GetSize(); i++)
    AppendAddress(H323TransportAddress(addresses[i]));
}


void H323TransportAddressArray::AppendString(const char * str)
{
  AppendAddress(H323TransportAddress(str));
}


void H323TransportAddressArray::AppendString(const PString & str)
{
  AppendAddress(H323TransportAddress(str));
}


void H323TransportAddressArray::AppendAddress(const H323TransportAddress & addr)
{
  if (!addr)
    Append(new H323TransportAddress(addr));
}


void H323TransportAddressArray::AppendStringCollection(const PCollection & coll)
{
  for (PINDEX i = 0; i < coll.GetSize(); i++) {
    PObject * obj = coll.GetAt(i);
    if (obj != NULL && PIsDescendant(obj, PString))
      AppendAddress(H323TransportAddress(*(PString *)obj));
  }
}


void H323SetTransportAddresses(const H323Transport & associatedTransport,
                               const H323TransportAddressArray & addresses,
                               H225_ArrayOf_TransportAddress & pdu)
{
  for (PINDEX i = 0; i < addresses.GetSize(); i++) {
    H323TransportAddress addr = addresses[i];

    PTRACE(4, "TCP\tAppending H.225 transport " << addr
           << " using associated transport " << associatedTransport);

    PIPSocket::Address ip;
    WORD port;
    if (addr.GetIpAndPort(ip, port)) {
      PIPSocket::Address remoteIP;
      if (associatedTransport.GetRemoteAddress().GetIpAddress(remoteIP)) {
        if (associatedTransport.GetEndPoint().GetManager().TranslateIPAddress(ip, remoteIP))
          addr = H323TransportAddress(ip, port);
      }
    }

    H225_TransportAddress pduAddr;
    addr.SetPDU(pduAddr, associatedTransport.GetEndPoint().GetDefaultSignalPort());

    PINDEX lastPos = pdu.GetSize();

    // Check for already have had that address.
    PINDEX j;
    for (j = 0; j < lastPos; j++) {
      if (pdu[j] == pduAddr)
        break;
    }

    if (j >= lastPos) {
      // Put new listener into array
      pdu.SetSize(lastPos+1);
      pdu[lastPos] = pduAddr;
    }
  }
}


/////////////////////////////////////////////////////////////////////////////

