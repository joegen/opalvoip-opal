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
 * Revision 1.2003  2001/11/12 05:32:12  robertj
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



/////////////////////////////////////////////////////////////////////////////

H323TransportAddress::H323TransportAddress(const H225_TransportAddress & transport,
                                           const char * proto)
{
  switch (transport.GetTag()) {
    case H225_TransportAddress::e_ipAddress :
    {
      const H225_TransportAddress_ipAddress & ip = transport;
      sprintf("%s%u.%u.%u.%u:%u",
              proto,
              ip.m_ip[0], ip.m_ip[1], ip.m_ip[2], ip.m_ip[3],
              (unsigned)ip.m_port);
      break;
    }
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
      switch (transport.GetTag()) {
        case H245_UnicastAddress::e_iPAddress :
        {
          const H245_UnicastAddress_iPAddress & ip = unicast;
          sprintf("%s%u.%u.%u.%u:%u",
                  proto,
                  ip.m_network[0], ip.m_network[1], ip.m_network[2], ip.m_network[3],
                  (unsigned)ip.m_tsapIdentifier);
          break;
        }
      }
      break;
    }
  }

  SetInternalTransport(0, NULL);
}


static void AppendTransportAddress(H225_ArrayOf_TransportAddress & pdu,
                                   const PIPSocket::Address & addr, WORD port)
{
  H225_TransportAddress_ipAddress pduAddr;
  PINDEX i;
  for (i = 0; i < 4; i++)
    pduAddr.m_ip[i] = addr[i];
  pduAddr.m_port = port;

  PINDEX lastPos = pdu.GetSize();

  // Check for already have had that IP address.
  for (i = 0; i < lastPos; i++) {
    H225_TransportAddress & taddr = pdu[i];
    if (taddr.GetTag() == H225_TransportAddress::e_ipAddress) {
      H225_TransportAddress_ipAddress & ipAddr = taddr;
      if (ipAddr == pduAddr)
        return;
    }
  }

  // Put new listener into array
  pdu.SetSize(lastPos+1);
  H225_TransportAddress & taddr = pdu[lastPos];

  taddr.SetTag(H225_TransportAddress::e_ipAddress);
  (H225_TransportAddress_ipAddress &)taddr = pduAddr;
}


BOOL H323TransportAddress::SetPDU(H225_ArrayOf_TransportAddress & pdu,
                                  const H323TransportAddress & first)
{
  PIPSocket::Address ip;
  WORD port = H323EndPoint::DefaultSignalTcpPort;
  if (!GetIpAndPort(ip, port))
    return FALSE;

  if (ip != INADDR_ANY) {
    AppendTransportAddress(pdu, ip, port);
    return TRUE;
  }

  PIPSocket::InterfaceTable interfaces;
  if (!PIPSocket::GetInterfaceTable(interfaces)) {
    PIPSocket::Address ipAddr;
    PIPSocket::GetHostAddress(ipAddr);
    AppendTransportAddress(pdu, ipAddr, port);
    return TRUE;
  }

  PINDEX i;

  PIPSocket::Address firstAddress;
  if (first.GetIpAddress(firstAddress)) {
    for (i = 0; i < interfaces.GetSize(); i++) {
      PIPSocket::Address ipAddr = interfaces[i].GetAddress();
      if (ipAddr == firstAddress)
        AppendTransportAddress(pdu, ipAddr, port);
    }
  }

  for (i = 0; i < interfaces.GetSize(); i++) {
    PIPSocket::Address ipAddr = interfaces[i].GetAddress();
    if (ipAddr != 0 && ipAddr != firstAddress && ipAddr != PIPSocket::Address()) // Ignore 127.0.0.1
      AppendTransportAddress(pdu, ipAddr, port);
  }

  return TRUE;
}


BOOL H323TransportAddress::SetPDU(H225_TransportAddress & pdu) const
{
  PIPSocket::Address ip;
  WORD port = H323EndPoint::DefaultSignalTcpPort;
  if (GetIpAndPort(ip, port)) {
    pdu.SetTag(H225_TransportAddress::e_ipAddress);
    H225_TransportAddress_ipAddress & addr = pdu;
    for (PINDEX i = 0; i < 4; i++)
      addr.m_ip[i] = ip[i];
    addr.m_port = port;
    return TRUE;
  }

  return FALSE;
}


BOOL H323TransportAddress::SetPDU(H245_TransportAddress & pdu) const
{
  PIPSocket::Address ip;
  WORD port = 0;
  if (GetIpAndPort(ip, port)) {
    pdu.SetTag(H245_TransportAddress::e_unicastAddress);

    H245_UnicastAddress & unicast = pdu;
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
