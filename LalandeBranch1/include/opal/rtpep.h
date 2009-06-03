/*
 * Endpoint abstraction
 *
 * Open Phone Abstraction Library (OPAL)
 *
 * Copyright (C) 2007 Post Increment
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
 * The Initial Developer of the Original Code is Post Increment
 *
 * Contributor(s): ______________________________________.
 *
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_OPAL_RTPEP_H
#define OPAL_OPAL_RTPEP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#include <opal/endpoint.h>

class OpalRTPEndPoint : public OpalEndPoint
{
  PCLASSINFO(OpalRTPEndPoint, OpalEndPoint);

  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    OpalRTPEndPoint(
      OpalManager & manager,          ///<  Manager of all endpoints.
      const PCaselessString & prefix, ///<  Prefix for URL style address strings
      unsigned attributes             ///<  Bit mask of attributes endpoint has
    );

    /**Destroy the endpoint.
     */
    ~OpalRTPEndPoint();

    /**Callback to allow interface adjustments before connecting to the remote party
       The default implementation does nothing and returns PTrue
      */
    virtual PBoolean AdjustInterfaceTable(PIPSocket::Address & remoteAddress,
                                      PIPSocket::InterfaceTable & interfaceTable);

    /**Determine if the RTP session needs to accommodate a NAT router.
       For endpoints that do not use STUN or something similar to set up all the
       correct protocol embeddded addresses correctly when a NAT router is between
       the endpoints, it is possible to still accommodate the call, with some
       restrictions. This function determines if the RTP can proceed with special
       NAT allowances.

       The special allowance is that the RTP code will ignore whatever the remote
       indicates in the protocol for the address to send RTP data and wait for
       the first packet to arrive from the remote and will then proceed to send
       all RTP data back to that address AND port.

       The default behaviour checks the values of the physical link
       (localAddr/peerAddr) against the signaling address the remote indicated in
       the protocol, eg H.323 SETUP sourceCallSignalAddress or SIP "To" or
       "Contact" fields, and makes a guess that the remote is behind a NAT router.
     */
    virtual PBoolean IsRTPNATEnabled(
      OpalConnection & connection,            ///< Connection being checked
      const PIPSocket::Address & localAddr,   ///< Local physical address of connection
      const PIPSocket::Address & peerAddr,    ///< Remote physical address of connection
      const PIPSocket::Address & signalAddr,  ///< Remotes signaling address as indicated by protocol of connection
      PBoolean incoming                       ///< Incoming/outgoing connection
    );

#ifdef OPAL_ZRTP
    virtual bool GetZRTPEnabled() const;
#endif

    /**Get the data formats this endpoint is capable of operating.
       This provides a list of media data format names that may be used by an
       OpalMediaStream may be created by a connection from this endpoint.

       Note that a specific connection may not actually support all of the
       media formats returned here, but should return no more.

       The default behaviour is pure.
      */
    virtual OpalMediaFormatList GetMediaFormats() const;
  //@}
    
  protected:
#ifdef OPAL_ZRTP
    bool zrtpEnabled;
#endif

  //@}
};


#endif // OPAL_OPAL_RTPEP_H
