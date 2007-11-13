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
 * Revision 2.8  2006/08/21 05:29:25  csoutheren
 * Messy but relatively simple change to add support for secure (SSL/TLS) TCP transport
 * and secure H.323 signalling via the sh323 URL scheme
 *
 * Revision 2.7  2005/11/30 13:35:26  csoutheren
 * Changed tags for Doxygen
 *
 * Revision 2.6  2004/02/19 10:46:44  rjongbloed
 * Merged OpenH323 version 1.13.1 changes.
 *
 * Revision 2.5  2002/11/10 11:33:17  robertj
 * Updated to OpenH323 v1.10.3
 *
 * Revision 2.4  2002/09/16 02:52:34  robertj
 * Added #define so can select if #pragma interface/implementation is used on
 *   platform basis (eg MacOS) rather than compiler, thanks Robert Monaghan.
 *
 * Revision 2.3  2002/09/12 06:58:17  robertj
 * Removed protocol prefix strings as static members as has problems with
 *   use in DLL environment.
 *
 * Revision 2.2  2002/07/01 04:56:30  robertj
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

#ifdef P_USE_PRAGMA
#pragma interface
#endif


#include <ptlib/sockets.h>
#include <opal/transports.h>


class H225_TransportAddress;
class H245_TransportAddress;
class H225_ArrayOf_TransportAddress;


typedef OpalListener  H323Listener;
typedef PList<H323Listener> H323ListenerList;
typedef OpalTransport H323Transport;
typedef OpalTransportUDP H323TransportUDP;


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
      const char * proto = NULL ///<  Default to tcp
    );
    H323TransportAddress(
      const H245_TransportAddress & pdu,
      const char * proto = NULL ///<  default to udp
    );

    PBoolean SetPDU(
      H225_ArrayOf_TransportAddress & pdu,  ///<  List of transport addresses listening on
      const OpalTransport & associatedTransport ///<  Associated transport for precendence and translation
    );
    PBoolean SetPDU(H225_TransportAddress & pdu, WORD defPort = 0) const;
    PBoolean SetPDU(H245_TransportAddress & pdu, WORD defPort = 0) const;
};


PDECLARE_ARRAY(H323TransportAddressArray, H323TransportAddress)
  public:
    H323TransportAddressArray(
      const OpalTransportAddress & address
    ) { AppendAddress(address); }
    H323TransportAddressArray(
      const H323TransportAddress & address
    ) { AppendAddress(address); }
    H323TransportAddressArray(
      const H225_ArrayOf_TransportAddress & addresses
    );
    H323TransportAddressArray(
      const OpalTransportAddressArray & array
    ) { AppendStringCollection(array); }
    H323TransportAddressArray(
      const PStringArray & array
    ) { AppendStringCollection(array); }
    H323TransportAddressArray(
      const PStringList & list
    ) { AppendStringCollection(list); }
    H323TransportAddressArray(
      const PSortedStringList & list
    ) { AppendStringCollection(list); }

    void AppendString(
      const char * address
    );
    void AppendString(
      const PString & address
    );
    void AppendAddress(
      const H323TransportAddress & address
    );

  protected:
    void AppendStringCollection(
      const PCollection & coll
    );
};


/**Set the PDU field for the list of transport addresses
  */
void H323SetTransportAddresses(
  const H323Transport & associatedTransport,   ///<  Transport for NAT address translation
  const H323TransportAddressArray & addresses, ///<  Addresses to set
  H225_ArrayOf_TransportAddress & pdu          ///<  List of PDU transport addresses
);


#endif ///<  __H323_TRANSADDR_H


/////////////////////////////////////////////////////////////////////////////
