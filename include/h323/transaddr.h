/*
 * transport.h
 *
 * H.323 protocol handler
 *
 * Open H323 Library
 *
 * Copyright (c) 1998-2001 Equivalence Pty. Ltd.
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
 * $Log: transaddr.h,v $
 * Revision 1.2003  2002/07/01 04:56:30  robertj
 * Updated to OpenH323 v1.9.1
 *
 * Revision 2.1  2001/11/09 05:49:47  robertj
 * Abstracted UDP connection algorithm
 *
 * Revision 2.0  2001/07/27 15:48:24  robertj
 * Conversion of OpenH323 to Open Phone Abstraction Library (OPAL)
 *
 */

#ifndef __H323_TRANSADDR_H
#define __H323_TRANSADDR_H

#ifdef __GNUC__
#pragma interface
#endif


#include <ptlib/sockets.h>
#include <opal/transports.h>


class H225_TransportAddress;
class H245_TransportAddress;
class H225_ArrayOf_TransportAddress;


///////////////////////////////////////////////////////////////////////////////

/**Transport address for H.323.
   This adds functions to the basic OpalTransportAddress for conversions to
   and from H.225 and H.245 PDU structures.
 */
class H323TransportAddress : public OpalTransportAddress
{
    PCLASSINFO(H323TransportAddress, OpalTransportAddress);
  public:
    H323TransportAddress()
      { }
    H323TransportAddress(const char * addr, WORD port = 0, const char * proto = NULL)
      : OpalTransportAddress(addr, port, proto) { }
    H323TransportAddress(const PString & addr, WORD port = 0, const char * proto = NULL)
      : OpalTransportAddress(addr, port, proto) { }
    H323TransportAddress(const OpalTransportAddress & addr)
      : OpalTransportAddress(addr) { }
    H323TransportAddress(PIPSocket::Address ip, WORD port, const char * proto = NULL)
      : OpalTransportAddress(ip, port, proto) { }

    H323TransportAddress(
      const H225_TransportAddress & pdu,
      const char * proto = OpalTransportAddress::TcpPrefix
    );
    H323TransportAddress(
      const H245_TransportAddress & pdu,
      const char * proto = OpalTransportAddress::UdpPrefix
    );

    BOOL SetPDU(
      H225_ArrayOf_TransportAddress & pdu,  /// List of transport addresses listening on
      const OpalTransport & associatedTransport /// Associated transport for precendence and translation
    );
    BOOL SetPDU(H225_TransportAddress & pdu) const;
    BOOL SetPDU(H245_TransportAddress & pdu) const;
};


#endif // __H323_TRANSADDR_H


/////////////////////////////////////////////////////////////////////////////
