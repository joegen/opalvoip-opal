/*
 * h323rtp.h
 *
 * H.323 RTP protocol handler
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
 * $Revision$
 * $Author$
 * $Date$
 */

#ifndef OPAL_H323_H323RTP_H
#define OPAL_H323_H323RTP_H

#ifdef P_USE_PRAGMA
#pragma interface
#endif

#include <opal/buildopts.h>

#if OPAL_H323

#include <h323/channels.h>
#include <rtp/rtp_session.h>


class H225_RTPSession;

class H245_TransportAddress;
class H245_H2250LogicalChannelParameters;
class H245_H2250LogicalChannelAckParameters;
class H245_ArrayOf_GenericInformation;

class H323Connection;
class H323_RTPChannel;

class H245_TransportCapability;


///////////////////////////////////////////////////////////////////////////////

/**This class is for encpsulating the IETF Real Time Protocol interface.
 */
class H323_RTPChannel : public H323_RealTimeChannel
{
    PCLASSINFO(H323_RTPChannel, H323_RealTimeChannel);
  public:
  /**@name Construction */
  //@{
    /**Create a new channel.
     */
    H323_RTPChannel(
      H323Connection & connection,        ///<  Connection to endpoint for channel
      const H323Capability & capability,  ///<  Capability channel is using
      Directions direction,               ///< Direction of channel
      OpalMediaSession & session          ///< Session for channel
    );

    /// Destroy the channel
    ~H323_RTPChannel();
  //@}

  /**@name Overrides from class H323Channel */
  //@{
    /**Indicate the session number of the channel.
       Return session for channel. This returns the session ID of the
       H323RTPSession member variable.
     */
    virtual unsigned GetSessionID() const;

    /**Set the session number of the channel.
       During OLC negotations teh master may change the session number being
       used for the logical channel.

       Returns false if the session could not be renumbered.
      */
    virtual bool SetSessionID(
      unsigned sessionID   ///< New session ID
    );

    /**Get the media transport address for the connection.
       This is primarily used to determine if media bypass is possible for the
       call between two connections.

       The default behaviour returns false.
     */
    virtual PBoolean GetMediaTransportAddress(
      OpalTransportAddress & data,        ///<  Data channel address
      OpalTransportAddress & control      ///<  Control channel address
    ) const;
  //@}

  /**@name Overrides from class H323_RealTimeChannel */
  //@{
    /**Fill out the OpenLogicalChannel PDU for the particular channel type.
     */
    virtual PBoolean OnSendingPDU(
      H245_H2250LogicalChannelParameters & param  ///<  Open PDU to send.
    ) const;

    /**Alternate RTP port information for Same NAT
      */
    virtual PBoolean OnSendingAltPDU(
      H245_ArrayOf_GenericInformation & alternate  ///< Alternate RTP ports
    ) const;

    /**This is called when request to create a channel is received from a
       remote machine and is about to be acknowledged.
     */
    virtual void OnSendOpenAck(
      H245_H2250LogicalChannelAckParameters & param ///<  Acknowledgement PDU
    ) const;

    /**This is called after a request to create a channel occurs from the
       local machine via the H245LogicalChannelDict::Open() function, and
       the request has been acknowledged by the remote endpoint.

       The default behaviour sets the remote ports to send UDP packets to.
     */
    virtual PBoolean OnReceivedPDU(
      const H245_H2250LogicalChannelParameters & param, ///<  Acknowledgement PDU
      unsigned & errorCode                              ///<  Error on failure
    );

    /**This is called after a request to create a channel occurs from the
       local machine via the H245LogicalChannelDict::Open() function, and
       the request has been acknowledged by the remote endpoint.

       The default behaviour sets the remote ports to send UDP packets to.
     */
    virtual PBoolean OnReceivedAckPDU(
      const H245_H2250LogicalChannelAckParameters & param ///<  Acknowledgement PDU
    );

    /**Alternate RTP port information for Same NAT
      */
    virtual PBoolean OnReceivedAckAltPDU(
      const H245_ArrayOf_GenericInformation & alternate  ///< Alternate RTP ports
    );
  //@}

  /**@name Operations */
  //@{
    /**This is called when a gatekeeper wants to get status information from
       the endpoint.

       The default behaviour calls the ancestor functon and then fills in the 
       transport fields.
     */
    virtual void OnSendRasInfo(
      H225_RTPSession & info  ///<  RTP session info PDU
    );
  //@}

#if 0 // NOTE QOS? 
  /**@name GQoS Support */
  //@{
    /**Write the Transport Capability PDU to Include GQoS Support.
     */
    virtual PBoolean WriteTransportCapPDU(
       H245_TransportCapability & cap,      ///* Transport Capability PDU
       const H323_RTPChannel & channel    ///* Channel using this session.
       ) const;

    /**Read the Transport Capability PDU to detect GQoS Support.
     */
    virtual void ReadTransportCapPDU(
    const H245_TransportCapability & cap,    ///* Transport Capability PDU
    H323_RTPChannel & channel        ///* Channel using this session.
        );
  //@}
#endif
  
  protected:
    virtual bool ExtractTransport(
      const H245_TransportAddress & pdu,
      bool isDataPort,
      unsigned & errorCode
    );

  protected:
    OpalMediaSession & m_session;
};


#endif // OPAL_H323

#endif // OPAL_H323_H323RTP_H


/////////////////////////////////////////////////////////////////////////////
